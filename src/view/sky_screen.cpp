/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "sky_screen.h"

#include "asset_manager.h"
#include "docs.h"
#include "location.h"
#include "projection.h"
#include "raster.h"
#include "theme.h"
#include "twilight.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>

namespace zerosky {
namespace {

constexpr int kW = view::kScreenWidth;
constexpr int kH = view::kScreenHeight;
constexpr double kSkyRefreshSeconds = 10.0;
constexpr const char* kCardinals[8] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};

constexpr uint16_t rgb565(int r, int g, int b) {
    return static_cast<uint16_t>(((std::clamp(r, 0, 255) >> 3) << 11) |
                                 ((std::clamp(g, 0, 255) >> 2) << 5) | (std::clamp(b, 0, 255) >> 3));
}
constexpr uint16_t red565(int level) { return rgb565(level, 0, 0); }

// Two themes. Night draws everything in shades of red on black, so the eyes
// stay adapted to the dark. Day draws white stars and blue figures on a dark
// blue sky, easier to read in daylight.
struct SkyPalette {
    uint16_t sky;
    uint16_t ground;
    uint16_t horizon;
    uint16_t figure;
    uint16_t highlight; // Sun, Moon, planets, the search ring and arrow
    uint16_t deep_sky;  // galaxies, clusters and nebulae
    uint32_t text;      // label colour, 0xRRGGBB
    uint16_t (*star)(int level);
};

uint16_t night_star(int level) { return red565(level); }
// On the blue day sky even faint stars need a light grey to show.
uint16_t day_star(int level) {
    const int v = 120 + level / 2;
    return rgb565(v, v, std::min(255, v + 20));
}

const SkyPalette kNight{0, red565(14), red565(110), red565(70), red565(255), red565(150),
                        0xc02010, night_star};
const SkyPalette kDay{rgb565(8, 16, 40), rgb565(22, 28, 36), rgb565(110, 140, 190),
                      rgb565(50, 90, 160), rgb565(255, 220, 120), rgb565(120, 210, 170),
                      0xe8eef8, day_star};

const SkyPalette& sky_palette(bool night) { return night ? kNight : kDay; }

double now_utc() {
    using namespace std::chrono;
    return duration<double>(system_clock::now().time_since_epoch()).count();
}

// Brighter stars are larger and brighter.
void draw_star(uint16_t* buf, int x, int y, float magnitude, const SkyPalette& pal) {
    const int level = std::clamp(static_cast<int>(255.0f - (magnitude + 1.5f) * 26.0f), 50, 255);
    const uint16_t color = pal.star(level);
    if (magnitude < 1.0f) {
        view::plot_disc(buf, kW, kH, x, y, 2, color);
    } else if (magnitude < 2.5f) {
        view::plot_disc(buf, kW, kH, x, y, 1, color);
    } else if (x >= 0 && x < kW && y >= 0 && y < kH) {
        buf[y * kW + x] = color;
    }
}

// A small symbol per kind, as on printed star charts: a ring for a galaxy, a
// ring with a cross for a globular cluster, a dotted ring for an open
// cluster, a square for a nebula, a ring with a dot for a planetary nebula.
void draw_deep_sky(uint16_t* buf, int x, int y, sky::DeepSkyKind kind, uint16_t color) {
    switch (kind) {
        case sky::DeepSkyKind::Galaxy:
            view::plot_ring(buf, kW, kH, x, y, 3, color);
            break;
        case sky::DeepSkyKind::GlobularCluster:
            view::plot_ring(buf, kW, kH, x, y, 3, color);
            view::plot_line(buf, kW, kH, x - 3, y, x + 3, y, color);
            view::plot_line(buf, kW, kH, x, y - 3, x, y + 3, color);
            break;
        case sky::DeepSkyKind::OpenCluster:
            for (int i = 0; i < 8; ++i) {
                const double a = i * M_PI / 4.0;
                const int px = x + static_cast<int>(std::lround(4.0 * std::cos(a)));
                const int py = y + static_cast<int>(std::lround(4.0 * std::sin(a)));
                if (px >= 0 && px < kW && py >= 0 && py < kH) buf[py * kW + px] = color;
            }
            break;
        case sky::DeepSkyKind::PlanetaryNebula:
            view::plot_ring(buf, kW, kH, x, y, 3, color);
            view::plot_disc(buf, kW, kH, x, y, 0, color);
            break;
        default:
            view::plot_line(buf, kW, kH, x - 3, y - 3, x + 3, y - 3, color);
            view::plot_line(buf, kW, kH, x + 3, y - 3, x + 3, y + 3, color);
            view::plot_line(buf, kW, kH, x + 3, y + 3, x - 3, y + 3, color);
            view::plot_line(buf, kW, kH, x - 3, y + 3, x - 3, y - 3, color);
            break;
    }
}

// The name the search shows and the target keeps: "M42 Orion Nebula", "NGC 869".
std::string deep_sky_search_name(const sky::DeepSkyObject& obj) {
    return obj.name.empty() ? obj.label() : obj.label() + " " + obj.name;
}

int body_radius(sky::Body body) {
    switch (body) {
        case sky::Body::Sun: return 6;
        case sky::Body::Moon: return 5;
        case sky::Body::Venus:
        case sky::Body::Jupiter: return 3;
        default: return 2;
    }
}

lv_obj_t* make_label(lv_obj_t* parent, const lv_font_t* font) {
    auto* label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
    return label;
}

// Places a label next to a point, kept inside the screen.
void place_label(lv_obj_t* label, const sky::ScreenPoint& p, int dx, int dy) {
    lv_obj_remove_flag(label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_update_layout(label);
    const int32_t w = lv_obj_get_width(label);
    const int32_t h = lv_obj_get_height(label);
    const int32_t x = std::clamp(static_cast<int32_t>(p.x) + dx, int32_t{0}, std::max(int32_t{0}, kW - w));
    const int32_t y = std::clamp(static_cast<int32_t>(p.y) + dy, int32_t{0}, std::max(int32_t{0}, kH - h));
    lv_obj_set_pos(label, x, y);
}

} // namespace

SkyScreen::SkyScreen(SkyViewModel& vm, app::AssetManager& assets)
    : BaseScreen(vm, vm, assets), vm_(vm) {
    catalog_ = sky::StarCatalog::load(assets.resolve("sky/stars.tsv").string());
    lines_ = sky::ConstellationLines::load(assets.resolve("sky/constellations.tsv").string());
    deep_sky_ = sky::DeepSkyCatalog::load(assets.resolve("sky/deep_sky.tsv").string());
    init();
    vm_.start_attitude(assets.resolve("wmm/WMM.COF"));
    timer_ = lv_timer_create(tick_cb, 100, this);
}

SkyScreen::~SkyScreen() {
    if (timer_) lv_timer_delete(timer_);
}

void SkyScreen::build_content(lv_obj_t* content) {
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_set_style_bg_color(content, lv_color_black(), 0); // under the canvas only
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    pixels_.assign(static_cast<size_t>(kW) * kH, 0);
    canvas_ = lv_canvas_create(content);
    lv_canvas_set_buffer(canvas_, pixels_.data(), kW, kH, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(canvas_, 0, 0);

    auto* small = assets().load_font("inter-medium.ttf", 10);
    auto* medium = assets().load_font("inter-semibold.ttf", 12);
    const lv_font_t* small_font = small ? small : &lv_font_montserrat_12;
    const lv_font_t* medium_font = medium ? medium : &lv_font_montserrat_12;

    for (size_t i = 0; i < cardinals_.size(); ++i) {
        cardinals_[i] = make_label(content, medium_font);
        lv_label_set_text(cardinals_[i], kCardinals[i]);
    }
    for (auto*& label : body_labels_) label = make_label(content, small_font);

    target_label_ = make_label(content, medium_font);

    status_ = make_label(content, small_font);
    lv_obj_set_pos(status_, 3, 2);
    lv_obj_remove_flag(status_, LV_OBJ_FLAG_HIDDEN);
}

void SkyScreen::tick_cb(lv_timer_t* timer) {
    if (auto* self = static_cast<SkyScreen*>(lv_timer_get_user_data(timer))) self->tick();
}

void SkyScreen::tick() {
    if (location_dialog_ && !location_dialog_->is_open()) location_dialog_.reset();
    if (!location_dialog_ && (vm_.take_location_request() || vm_.take_first_location_prompt())) {
        open_location_dialog();
        return;
    }
    if (about_ && !about_->is_open()) about_.reset();
    if (!about_ && vm_.take_about_request()) open_about();
    const double utc = now_utc();
    if (search_ && !search_->is_open()) search_.reset();
    if (!search_ && vm_.take_search_request()) open_search();
    if (vm_.observer_version() != sky_observer_version_ ||
        std::abs(utc - sky_utc_) > kSkyRefreshSeconds) { // also when the clock steps back
        update_sky(utc);
    }
    vm_.update_camera();
    render();
}

void SkyScreen::update_sky(double utc) {
    const sky::Observer& observer = vm_.observer();
    star_positions_.clear();
    star_positions_.reserve(catalog_.stars().size());
    for (const auto& star : catalog_.stars()) {
        star_positions_.push_back(sky::star_horizontal(star.ra_hours, star.dec_deg, utc, observer));
    }
    line_positions_.clear();
    line_positions_.reserve(lines_.segments().size());
    for (const auto& seg : lines_.segments()) {
        line_positions_.push_back(
            {sky::star_horizontal(seg.a.ra_hours, seg.a.dec_deg, utc, observer),
             sky::star_horizontal(seg.b.ra_hours, seg.b.dec_deg, utc, observer)});
    }
    deep_sky_positions_.clear();
    deep_sky_positions_.reserve(deep_sky_.objects().size());
    for (const auto& obj : deep_sky_.objects()) {
        deep_sky_positions_.push_back(sky::star_horizontal(obj.ra_hours, obj.dec_deg, utc, observer));
    }
    bodies_ = sky::solar_system(utc, observer);
    twilight_magnitude_ = 6.0;
    for (const auto& body : bodies_) {
        if (body.body == sky::Body::Sun) {
            twilight_magnitude_ =
                sky::twilight_from_sun_altitude(body.horizontal.altitude_deg).limiting_star_magnitude;
        }
    }
    sky_utc_ = utc;
    sky_observer_version_ = vm_.observer_version();
}

void SkyScreen::render() {
    const sky::Camera& cam = vm_.camera();
    const sky::Projection projection(cam, kW, kH);
    uint16_t* buf = pixels_.data();
    const SkyPalette& pal = sky_palette(vm_.night_theme());
    if (labels_night_ != vm_.night_theme()) {
        labels_night_ = vm_.night_theme();
        auto paint = [&](lv_obj_t* label) {
            lv_obj_set_style_text_color(label, lv_color_hex(pal.text), 0);
        };
        for (auto* label : cardinals_) paint(label);
        for (auto* label : body_labels_) paint(label);
        paint(status_);
        paint(target_label_);
    }
    std::fill(pixels_.begin(), pixels_.end(), pal.sky);

    // The ground: a dim fill below the horizon, then the horizon line on top.
    for (int y = 0; y < kH; ++y) {
        for (int x = 0; x < kW; ++x) {
            if (projection.below_horizon(x + 0.5, y + 0.5)) buf[y * kW + x] = pal.ground;
        }
    }
    sky::ScreenPoint prev{};
    for (double az = 0.0; az <= 360.0; az += 1.0) {
        const auto p = projection.project(az, 0.0);
        if (p.visible && prev.visible && projection.on_screen(p, kW) &&
            projection.on_screen(prev, kW)) {
            view::plot_line(buf, kW, kH, static_cast<int>(prev.x), static_cast<int>(prev.y),
                            static_cast<int>(p.x), static_cast<int>(p.y), pal.horizon);
        }
        prev = p;
    }

    // Constellation figures, dim, under the stars.
    for (const auto& seg : line_positions_) {
        const auto a = projection.project(seg[0].azimuth_deg, seg[0].altitude_deg);
        const auto b = projection.project(seg[1].azimuth_deg, seg[1].altitude_deg);
        if (seg[0].altitude_deg < 0.0 && seg[1].altitude_deg < 0.0) continue; // below ground
        if (!projection.on_screen(a, kW) || !projection.on_screen(b, kW)) continue;
        view::plot_line(buf, kW, kH, static_cast<int>(a.x), static_cast<int>(a.y),
                        static_cast<int>(b.x), static_cast<int>(b.y), pal.figure);
    }

    const auto& stars = catalog_.stars();
    double zoom_limit = 4.0 + 2.0 * (60.0 / cam.fov_deg - 0.5);
    if (vm_.daylight()) zoom_limit = std::min(zoom_limit, twilight_magnitude_);
    const float faintest = static_cast<float>(zoom_limit);
    for (size_t i = 0; i < stars.size() && i < star_positions_.size(); ++i) {
        if (stars[i].magnitude > faintest) break; // brightest first
        const auto& h = star_positions_[i];
        if (h.altitude_deg < -1.0) continue;
        const auto p = projection.project(h.azimuth_deg, h.altitude_deg);
        if (!projection.on_screen(p, 2)) continue;
        draw_star(buf, static_cast<int>(p.x), static_cast<int>(p.y), stars[i].magnitude, pal);
    }

    // Deep-sky objects: the Messier ones at any zoom, the others as you zoom in.
    const double deep_limit = 7.0 + 2.0 * (60.0 / cam.fov_deg - 1.0);
    const bool deep_dimmed = vm_.daylight() && twilight_magnitude_ < 4.0;
    const auto& deep = deep_sky_.objects();
    for (size_t i = 0; !deep_dimmed && i < deep.size() && i < deep_sky_positions_.size(); ++i) {
        const auto& obj = deep[i];
        if (obj.messier.empty() && !(obj.magnitude <= deep_limit)) continue;
        const auto& h = deep_sky_positions_[i];
        if (h.altitude_deg < -1.0) continue;
        const auto p = projection.project(h.azimuth_deg, h.altitude_deg);
        if (!projection.on_screen(p, 4)) continue;
        draw_deep_sky(buf, static_cast<int>(p.x), static_cast<int>(p.y), obj.kind, pal.deep_sky);
    }

    for (size_t i = 0; i < body_labels_.size(); ++i) {
        lv_obj_add_flag(body_labels_[i], LV_OBJ_FLAG_HIDDEN);
        if (i >= bodies_.size()) continue;
        const auto& body = bodies_[i];
        if (body.horizontal.altitude_deg < -1.0) continue; // set: not drawn on the ground
        const auto p = projection.project(body.horizontal.azimuth_deg, body.horizontal.altitude_deg);
        if (!projection.on_screen(p, 6)) continue;
        view::plot_disc(buf, kW, kH, static_cast<int>(p.x), static_cast<int>(p.y),
                        body_radius(body.body), pal.highlight);
        lv_label_set_text(body_labels_[i], body.name);
        place_label(body_labels_[i], p, body_radius(body.body) + 2, -6);
    }

    for (size_t i = 0; i < cardinals_.size(); ++i) {
        lv_obj_add_flag(cardinals_[i], LV_OBJ_FLAG_HIDDEN);
        const auto p = projection.project(45.0 * static_cast<double>(i), 0.0);
        if (!projection.on_screen(p, -4)) continue;
        place_label(cardinals_[i], p, -4, 2);
    }

    render_guidance(buf, projection);

    // Status: the camera, the position, and why the IMU is not steering, if so.
    std::string note = vm_.location_set() ? vm_.location_label() : "no position: press 5";
    const std::string attitude = vm_.attitude_status();
    if (!attitude.empty()) note += "  " + attitude;
    else if (vm_.manual()) note += "  manual";
    char status[128];
    std::snprintf(status, sizeof status, "Az %03.0f  Alt %+03.0f  FOV %.0f  %s",
                  cam.azimuth_deg, cam.altitude_deg, cam.fov_deg, note.c_str());
    lv_label_set_text(status_, status);
    lv_obj_invalidate(canvas_);
}

bool SkyScreen::locate(const std::string& name, sky::Horizontal& out) const {
    for (const auto& body : bodies_) {
        if (name == body.name) {
            out = body.horizontal;
            return true;
        }
    }
    for (size_t i = 0; i < deep_sky_.objects().size() && i < deep_sky_positions_.size(); ++i) {
        if (deep_sky_search_name(deep_sky_.objects()[i]) == name) {
            out = deep_sky_positions_[i];
            return true;
        }
    }
    // A short form such as "M13" or "NGC 869", from ZEROSKY_TARGET.
    if (const sky::DeepSkyObject* obj = deep_sky_.find(name)) {
        const auto index = static_cast<size_t>(obj - deep_sky_.objects().data());
        if (index < deep_sky_positions_.size()) {
            out = deep_sky_positions_[index];
            return true;
        }
    }
    if (const sky::Star* star = catalog_.find_by_name(name)) {
        const auto index = static_cast<size_t>(star - catalog_.stars().data());
        if (index < star_positions_.size()) {
            out = star_positions_[index];
            return true;
        }
    }
    return false;
}

// Every object the search knows: the Sun, the Moon, the planets, the stars
// with a proper name (brightest first), then the deep-sky catalog.
void SkyScreen::open_search() {
    std::vector<SearchItem> items;
    for (const auto& body : bodies_) {
        const char* kind = body.body == sky::Body::Sun ? "Sun"
                         : body.body == sky::Body::Moon ? "Moon" : "planet";
        items.push_back({body.name, kind, body.horizontal.altitude_deg > 0.0});
    }
    const auto& stars = catalog_.stars();
    for (size_t i = 0; i < stars.size() && i < star_positions_.size(); ++i) {
        if (stars[i].name.empty()) continue;
        char detail[48];
        std::snprintf(detail, sizeof detail, "star mag %.1f %s", stars[i].magnitude,
                      stars[i].constellation.c_str());
        items.push_back({stars[i].name, detail, star_positions_[i].altitude_deg > 0.0});
    }
    static const char* const kKindNames[] = {"galaxy", "open cluster", "globular cluster",
                                             "nebula", "planetary nebula", "deep sky"};
    const auto& deep = deep_sky_.objects();
    for (size_t i = 0; i < deep.size() && i < deep_sky_positions_.size(); ++i) {
        char detail[48];
        std::snprintf(detail, sizeof detail, "%s %s", kKindNames[static_cast<int>(deep[i].kind)],
                      deep[i].constellation.c_str());
        items.push_back({deep_sky_search_name(deep[i]), detail,
                         deep_sky_positions_[i].altitude_deg > 0.0});
    }
    search_ = std::make_unique<SearchDialog>(assets(), vm_.is_dark_mode(), std::move(items),
                                             [this](const std::string& name) { vm_.set_target(name); });
}

void SkyScreen::render_guidance(uint16_t* buf, const sky::Projection& projection) {
    sky::Horizontal where;
    if (vm_.target().empty() || !locate(vm_.target(), where)) {
        // An unknown name (a typo in ZEROSKY_TARGET) is dropped once the sky is known.
        if (!vm_.target().empty() && !bodies_.empty()) {
            std::fprintf(stderr, "zerosky: unknown target \"%s\"\n", vm_.target().c_str());
            vm_.set_target({});
        }
        lv_obj_add_flag(target_label_, LV_OBJ_FLAG_HIDDEN);
        beeper_.stop();
        return;
    }
    const uint16_t highlight = sky_palette(vm_.night_theme()).highlight;
    const auto g = sky::guide(vm_.camera(), where.azimuth_deg, where.altitude_deg);
    // A target below the horizon keeps its arrow, so you know where it is,
    // but no beep: there is nothing to point at.
    const bool set = where.altitude_deg < 0.0;
    if (set) beeper_.stop();
    else beeper_.set_guidance(g.separation_deg);

    const auto p = projection.project(where.azimuth_deg, where.altitude_deg);
    char text[64];
    std::snprintf(text, sizeof text, "%s %s%.0f\xC2\xB0", vm_.target().c_str(), set ? "set " : "",
                  g.separation_deg);
    lv_label_set_text(target_label_, text);
    if (projection.on_screen(p, -8)) {
        view::plot_ring(buf, kW, kH, static_cast<int>(p.x), static_cast<int>(p.y), 9, highlight);
        place_label(target_label_, p, -20, 11);
        return;
    }
    // Off screen: an arrow near the edge, pointing the way to turn.
    const double rad = g.screen_bearing_deg * M_PI / 180.0;
    const double dx = std::sin(rad);
    const double dy = -std::cos(rad);
    const double reach = std::min((kW / 2.0 - 16.0) / std::max(std::abs(dx), 1e-6),
                                  (kH / 2.0 - 16.0) / std::max(std::abs(dy), 1e-6));
    const double tip_x = kW / 2.0 + dx * reach;
    const double tip_y = kH / 2.0 + dy * reach;
    const double base_x = tip_x - dx * 12.0;
    const double base_y = tip_y - dy * 12.0;
    view::plot_triangle(buf, kW, kH, static_cast<int>(tip_x), static_cast<int>(tip_y),
                        static_cast<int>(base_x - dy * 6.0), static_cast<int>(base_y + dx * 6.0),
                        static_cast<int>(base_x + dy * 6.0), static_cast<int>(base_y - dx * 6.0),
                        highlight);
    const sky::ScreenPoint label_at{kW / 2.0 + dx * (reach - 34.0), kH / 2.0 + dy * (reach - 30.0), true};
    place_label(target_label_, label_at, -24, -6);
}

#ifndef ZEROSKY_MANUAL_URL
#define ZEROSKY_MANUAL_URL "https://github.com/andreahaku/zerosky/blob/main/docs/manual.md"
#endif

void SkyScreen::open_about() {
    using Page = view::widgets::TextViewer::Page;
    const std::string url = ZEROSKY_MANUAL_URL;
    Page info;
    info.title = "About";
    info.markdown = "# ZeroSky " APP_VERSION "\n"
                    "A pocket planetarium for the M5Stack CardputerZero.\n"
                    "## Manual\n"
                    "Scan the QR code with your phone for the full manual.\n"
                    "## Developer\n"
                    "Andrea Salvatore (IU4APC)\n"
                    "## License\n"
                    "MIT. The sky data keep their own licenses: see Credits.";
    info.build = [url](lv_obj_t* content) {
        // Dark modules on a white quiet zone: phone cameras need the contrast.
        auto* qr = lv_qrcode_create(content);
        lv_qrcode_set_size(qr, 96);
        lv_qrcode_set_dark_color(qr, lv_color_black());
        lv_qrcode_set_light_color(qr, lv_color_white());
        lv_qrcode_update(qr, url.c_str(), static_cast<uint32_t>(url.size()));
        lv_obj_set_style_border_color(qr, lv_color_white(), 0);
        lv_obj_set_style_border_width(qr, 4, 0);
    };
    std::string credits = toolkit::read_doc("CREDITS.md");
    if (credits.empty()) credits = "CREDITS.md is not installed.";
    about_ = std::make_unique<view::widgets::TextViewer>(
        assets(), vm_.is_dark_mode(), std::vector<Page>{info, {"Credits", credits, {}}});
}

void SkyScreen::open_location_dialog() {
    location_dialog_ = std::make_unique<toolkit::LocationDialog>(
        assets(), vm_.is_dark_mode(), [this](const toolkit::location::Place& place) {
            toolkit::location::save(place);
            vm_.set_location(place.pos.lat, place.pos.lon, place.label);
        });
}

} // namespace zerosky
