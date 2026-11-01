/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "sky_screen.h"

#include "asset_manager.h"
#include "docs.h"
#include "i18n.h"
#include "sky_text.h"
#include "location.h"
#include "projection.h"
#include "raster.h"
#include "theme.h"
#include "twilight.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace zerosky {
namespace {

constexpr int kW = view::kScreenWidth;
constexpr int kH = view::kScreenHeight;
constexpr double kSkyRefreshSeconds = 10.0;
using toolkit::i18n::pick;

const char* body_name(sky::Body body) { return pick(text::kBodies[static_cast<int>(body)]); }

// The attitude controller reports in English; the screen shows the user's language.
std::string localized_attitude(const std::string& status) {
    if (status == "manual") return pick(text::kManual);
    if (status == "tilt only") return pick(text::kTiltOnly);
    if (status == "compass disturbed") return pick(text::kCompassDisturbed);
    if (status == "axes not measured") return pick(text::kAxesNotMeasured);
    if (status == "no IMU") return pick(text::kNoImu);
    if (status == "bad axes file") return pick(text::kBadAxes);
    if (status == "IMU did not start") return pick(text::kImuFailed);
    if (status == "no WMM data") return pick(text::kNoWmm);
    return status;
}

// Japanese and Chinese: Inter draws the Latin text and a system CJK font the
// rest, as the toolkit does for its tooltips. Debian paths, then Arch.
void attach_cjk_fallback(app::AssetManager& assets, lv_font_t* font, uint32_t size) {
    const auto lang = toolkit::i18n::current();
    if (!font || !toolkit::i18n::needs_cjk_font(lang)) return;
    static constexpr const char* kJapanese[] = {
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf"};
    static constexpr const char* kChinese[] = {
        "/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc"};
    for (const char* path : lang == toolkit::i18n::Lang::Ja ? kJapanese : kChinese) {
        if (lv_font_t* cjk = assets.load_font(path, size)) {
            font->fallback = cjk;
            return;
        }
    }
}

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
    bool outlined{false}; // over a camera image: dark outlines, label backdrops
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

// Over the camera image the marks must read on bright and dark areas alike:
// brighter colours, drawn on a dark outline (Codex's proposal).
uint16_t photo_night_star(int level) { return red565(std::max(level, 180)); }
uint16_t photo_day_star(int) { return rgb565(255, 255, 255); }
const SkyPalette kPhotoNight{0, 0, red565(255), rgb565(255, 60, 40), red565(255),
                             rgb565(255, 90, 60), 0xff4030, photo_night_star, true};
const SkyPalette kPhotoDay{0, 0, rgb565(255, 220, 60), rgb565(60, 220, 255), rgb565(255, 220, 120),
                           rgb565(120, 255, 170), 0xffffff, photo_day_star, true};

const SkyPalette& sky_palette(bool night, bool photo) {
    if (photo) return night ? kPhotoNight : kPhotoDay;
    return night ? kNight : kDay;
}

// A line, with a 3 px dark outline under it when the palette asks for one.
void mark_line(uint16_t* buf, int x0, int y0, int x1, int y1, uint16_t color, bool outlined) {
    if (outlined) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx != 0 || dy != 0) view::plot_line(buf, kW, kH, x0 + dx, y0 + dy, x1 + dx, y1 + dy, 0);
            }
        }
    }
    view::plot_line(buf, kW, kH, x0, y0, x1, y1, color);
}

double now_utc() {
    using namespace std::chrono;
    return duration<double>(system_clock::now().time_since_epoch()).count();
}

// Brighter stars are larger and brighter.
void draw_star(uint16_t* buf, int x, int y, float magnitude, const SkyPalette& pal) {
    const int level = std::clamp(static_cast<int>(255.0f - (magnitude + 1.5f) * 26.0f), 50, 255);
    const uint16_t color = pal.star(level);
    if (pal.outlined) view::plot_disc(buf, kW, kH, x, y, magnitude < 1.0f ? 3 : 2, 0);
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

// The Moon with its phase: lit where x' >= (1 - 2k) * sqrt(r^2 - y'^2), with
// x' along the screen direction to the Sun, y' across it, and k the lit
// fraction. The unlit part is drawn dim, so the whole disc stays findable.
void draw_moon(uint16_t* buf, int cx, int cy, int r, double sun_dx, double sun_dy,
               double lit_fraction, uint16_t lit, uint16_t dark) {
    const double len = std::hypot(sun_dx, sun_dy);
    const double ux = len > 1e-9 ? sun_dx / len : 1.0;
    const double uy = len > 1e-9 ? sun_dy / len : 0.0;
    const double k = std::clamp(lit_fraction, 0.0, 1.0);
    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            const double rr = static_cast<double>(dx * dx + dy * dy);
            if (rr > r * r + 0.5) continue;
            const int x = cx + dx;
            const int y = cy + dy;
            if (x < 0 || x >= kW || y < 0 || y >= kH) continue;
            const double along = dx * ux + dy * uy;
            const double across = -dx * uy + dy * ux;
            const double half_chord = std::sqrt(std::max(0.0, r * r - across * across));
            buf[y * kW + x] = along >= (1.0 - 2.0 * k) * half_chord ? lit : dark;
        }
    }
}

int body_radius(sky::Body body) {
    switch (body) {
        case sky::Body::Sun: return 6;
        case sky::Body::Moon: return 6;
        case sky::Body::Venus:
        case sky::Body::Jupiter: return 3;
        default: return 2;
    }
}

// Upper case for the status lines, which read better that way on 320x170.
// Handles ASCII and the Latin-1 letters of the European languages; CJK text
// and other characters pass through unchanged.
std::string upper(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        const auto c = static_cast<unsigned char>(in[i]);
        if (c >= 'a' && c <= 'z') {
            out += static_cast<char>(c - 32);
        } else if (c == 0xC3 && i + 1 < in.size()) {
            // U+00E0..U+00FE (except the division sign) map to U+00C0..U+00DE.
            const auto next = static_cast<unsigned char>(in[i + 1]);
            out += static_cast<char>(c);
            out += static_cast<char>(next >= 0xA0 && next <= 0xBE && next != 0xB7 ? next - 0x20 : next);
            ++i;
        } else {
            out += static_cast<char>(c);
        }
    }
    return out;
}

lv_obj_t* make_label(lv_obj_t* parent, const lv_font_t* font) {
    auto* label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
    return label;
}

struct LabelBox { int x, y, w, h; };

bool overlaps(const LabelBox& a, const LabelBox& b) {
    constexpr int gap = 3;
    return a.x < b.x + b.w + gap && b.x < a.x + a.w + gap &&
           a.y < b.y + b.h + gap && b.y < a.y + a.h + gap;
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
    constellation_names_ = sky::ConstellationNames::load(
        assets.resolve("sky/constellation_names.tsv").string());
    deep_sky_ = sky::DeepSkyCatalog::load(assets.resolve("sky/deep_sky.tsv").string());
    init();
    vm_.start_attitude(assets.resolve("wmm/WMM.COF"));
    timer_ = lv_timer_create(tick_cb, 100, this);
}

SkyScreen::~SkyScreen() {
    if (timer_) lv_timer_delete(timer_);
    if (camera_) camera_->stop();
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
    attach_cjk_fallback(assets(), small, 10);
    attach_cjk_fallback(assets(), medium, 12);
    // The NavBar's text and page-tab fonts: the asset manager hands out the
    // same font objects, so the fallback reaches the bar too.
    attach_cjk_fallback(assets(), assets().load_font("inter-semibold.ttf", 16), 16);
    attach_cjk_fallback(assets(), assets().load_font("inter-bold.ttf", 13), 13);
    // The dialogs' and text viewer's fonts: help, About, Location and Find.
    attach_cjk_fallback(assets(), assets().load_font("inter-semibold.ttf", 14), 14);
    attach_cjk_fallback(assets(), assets().load_font("inter-regular.ttf", 12), 12);
    const lv_font_t* small_font = small ? small : &lv_font_montserrat_12;
    const lv_font_t* medium_font = medium ? medium : &lv_font_montserrat_12;

    for (size_t i = 0; i < cardinals_.size(); ++i) {
        cardinals_[i] = make_label(content, medium_font);
        lv_label_set_text(cardinals_[i], pick(text::kCardinals[i]));
    }
    for (auto*& label : body_labels_) label = make_label(content, small_font);
    for (auto*& label : constellation_labels_) label = make_label(content, small_font);
    for (auto*& label : deep_labels_) label = make_label(content, small_font);

    target_label_ = make_label(content, medium_font);

    // Two short lines instead of one long one, so nothing is cut off: the
    // camera numbers and the position on top, mode and warnings below.
    status_ = make_label(content, small_font);
    lv_obj_set_pos(status_, 3, 2);
    lv_obj_remove_flag(status_, LV_OBJ_FLAG_HIDDEN);
    // The labels are as wide as their text, so a backdrop over the photo
    // covers only the words; long text is cut with dots at the maximum width.
    place_ = make_label(content, small_font);
    lv_obj_set_width(place_, LV_SIZE_CONTENT);
    lv_obj_set_style_max_width(place_, 130, 0);
    lv_label_set_long_mode(place_, LV_LABEL_LONG_DOT);
    lv_obj_align(place_, LV_ALIGN_TOP_RIGHT, -3, 2);
    lv_obj_remove_flag(place_, LV_OBJ_FLAG_HIDDEN);
    notice_ = make_label(content, small_font);
    lv_obj_set_width(notice_, LV_SIZE_CONTENT);
    lv_obj_set_style_max_width(notice_, kW - 6, 0);
    lv_label_set_long_mode(notice_, LV_LABEL_LONG_DOT);
    lv_obj_set_pos(notice_, 3, 18);

    // The box for "starting camera" and the Align steps.
    panel_ = lv_obj_create(content);
    lv_obj_remove_style_all(panel_);
    lv_obj_set_style_bg_color(panel_, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(panel_, LV_OPA_80, 0);
    lv_obj_set_style_radius(panel_, 4, 0);
    lv_obj_set_style_border_width(panel_, 1, 0);
    lv_obj_set_style_pad_all(panel_, 5, 0);
    lv_obj_set_width(panel_, kW - 40);
    lv_obj_set_height(panel_, LV_SIZE_CONTENT);
    lv_obj_clear_flag(panel_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(panel_, LV_OBJ_FLAG_HIDDEN);
    panel_text_ = lv_label_create(panel_);
    lv_obj_set_width(panel_text_, LV_PCT(100));
    lv_label_set_long_mode(panel_text_, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(panel_text_, small_font, 0);
}

void SkyScreen::tick_cb(lv_timer_t* timer) {
    if (auto* self = static_cast<SkyScreen*>(lv_timer_get_user_data(timer))) self->tick();
}

void SkyScreen::tick() {
    if (calibration_ && !calibration_->is_open()) calibration_.reset();
    if (!calibration_ && vm_.take_calibration_request()) {
        beeper_.stop();
        calibration_ = std::make_unique<CalibrationDialog>(vm_, assets(), beeper_,
                                                            vm_.is_dark_mode());
        return;
    }
    if (calibration_) {
        calibration_->tick();
        return; // The full-screen modal owns the display during capture.
    }
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
    update_camera_source();
    render();
}

// Starts the camera when its mode is on and releases it when off, so another
// app can use the IMX219.
void SkyScreen::update_camera_source() {
    if (vm_.camera_on() && !camera_) {
#if USE_DESKTOP
        camera_ = std::make_unique<camera::TestPatternSource>();
#else
        camera_ = std::make_unique<camera::RpicamSource>();
#endif
        std::string error;
        if (!camera_->start(error)) std::fprintf(stderr, "zerosky: camera: %s\n", error.c_str());
        camera_started_at_ = std::chrono::steady_clock::now();
    } else if (!vm_.camera_on() && camera_) {
        camera_->stop();
        camera_.reset();
    }
}

bool SkyScreen::draw_camera_background() {
    if (!camera_) return false;
    // Log the camera state when it changes: the first frame, or a new error.
    const std::string error = camera_->last_error();
    if (error != camera_logged_error_) {
        std::fprintf(stderr, "zerosky: camera: %s\n", error.empty() ? "ok" : error.c_str());
        camera_logged_error_ = error;
    }
    if (!camera_->copy_latest(camera_frame_)) return false;
    if (std::getenv("ZEROSKY_CAMERA_DEBUG")) {
        std::fprintf(stderr, "zerosky: camera frame %llu, %zu px, age %lld ms\n",
                     static_cast<unsigned long long>(camera_frame_.sequence), camera_frame_.rgb565.size(),
                     static_cast<long long>(std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::steady_clock::now() - camera_frame_.captured_at).count()));
    }
    // An old frame is worse than none: fall back to the drawn sky.
    if (std::chrono::steady_clock::now() - camera_frame_.captured_at > std::chrono::seconds(1)) {
        return false;
    }
    if (camera_frame_.rgb565.size() != pixels_.size()) return false;
    std::copy(camera_frame_.rgb565.begin(), camera_frame_.rgb565.end(), pixels_.begin());
    return true;
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
    constellation_name_positions_.clear();
    constellation_name_positions_.reserve(constellation_names_.names().size());
    for (const auto& name : constellation_names_.names()) {
        constellation_name_positions_.push_back(
            sky::star_horizontal(name.ra_hours, name.dec_deg, utc, observer));
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
    const sky::Camera cam = vm_.camera();
    const sky::Projection projection(cam, kW, kH);
    uint16_t* buf = pixels_.data();
    // The camera image, when it is on and fresh; otherwise the drawn sky and
    // ground. The palette follows the frame actually drawn, not the Cam switch.
    const bool photo = draw_camera_background();
    const SkyPalette& pal = sky_palette(vm_.night_theme(), photo);
    const int label_style = (vm_.night_theme() ? 1 : 0) + (photo ? 2 : 0);
    if (label_style_ != label_style) {
        label_style_ = label_style;
        auto paint = [&](lv_obj_t* label) {
            lv_obj_set_style_text_color(label, lv_color_hex(pal.text), 0);
            // Over a photograph a translucent dark backdrop keeps the text readable.
            lv_obj_set_style_bg_color(label, lv_color_black(), 0);
            lv_obj_set_style_bg_opa(label, photo ? LV_OPA_60 : LV_OPA_TRANSP, 0);
            lv_obj_set_style_pad_all(label, photo ? 2 : 0, 0);
            lv_obj_set_style_radius(label, 4, 0);
        };
        for (auto* label : cardinals_) paint(label);
        for (auto* label : body_labels_) paint(label);
        for (auto* label : constellation_labels_) {
            paint(label);
            lv_obj_set_style_text_color(label, lv_color_hex(
                photo ? pal.text : vm_.night_theme() ? 0xa84938 : 0xaac7f0), 0);
        }
        for (auto* label : deep_labels_) paint(label);
        paint(status_);
        paint(place_);
        paint(notice_);
        paint(target_label_);
        lv_obj_set_style_text_color(panel_text_, lv_color_hex(pal.text), 0);
        lv_obj_set_style_border_color(panel_, lv_color_hex(pal.text), 0);
    }
    if (!photo) {
        std::fill(pixels_.begin(), pixels_.end(), pal.sky);
        // The ground: a dim fill below the horizon, then the horizon line on top.
        for (int y = 0; y < kH; ++y) {
            for (int x = 0; x < kW; ++x) {
                if (projection.below_horizon(x + 0.5, y + 0.5)) buf[y * kW + x] = pal.ground;
            }
        }
    }
    sky::ScreenPoint prev{};
    for (double az = 0.0; az <= 360.0; az += 1.0) {
        const auto p = projection.project(az, 0.0);
        if (p.visible && prev.visible && projection.on_screen(p, kW) &&
            projection.on_screen(prev, kW)) {
            mark_line(buf, static_cast<int>(prev.x), static_cast<int>(prev.y),
                      static_cast<int>(p.x), static_cast<int>(p.y), pal.horizon, pal.outlined);
        }
        prev = p;
    }

    // Constellation figures, dim, under the stars.
    for (const auto& seg : line_positions_) {
        if (vm_.constellation_display() == ConstellationDisplay::Off) break;
        const auto a = projection.project(seg[0].azimuth_deg, seg[0].altitude_deg);
        const auto b = projection.project(seg[1].azimuth_deg, seg[1].altitude_deg);
        if (seg[0].altitude_deg < 0.0 && seg[1].altitude_deg < 0.0) continue; // below ground
        if (!projection.on_screen(a, kW) || !projection.on_screen(b, kW)) continue;
        mark_line(buf, static_cast<int>(a.x), static_cast<int>(a.y), static_cast<int>(b.x),
                  static_cast<int>(b.y), pal.figure, pal.outlined);
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
    for (size_t i = 0; vm_.deep_sky_visible() && !deep_dimmed && i < deep.size() &&
                       i < deep_sky_positions_.size(); ++i) {
        const auto& obj = deep[i];
        if (obj.messier.empty() && !(obj.magnitude <= deep_limit)) continue;
        const auto& h = deep_sky_positions_[i];
        if (h.altitude_deg < -1.0) continue;
        const auto p = projection.project(h.azimuth_deg, h.altitude_deg);
        if (!projection.on_screen(p, 4)) continue;
        if (pal.outlined) { // thin dark rings around the symbol, not a blot
            view::plot_ring(buf, kW, kH, static_cast<int>(p.x), static_cast<int>(p.y), 5, 0);
            view::plot_ring(buf, kW, kH, static_cast<int>(p.x), static_cast<int>(p.y), 2, 0);
        }
        draw_deep_sky(buf, static_cast<int>(p.x), static_cast<int>(p.y), obj.kind, pal.deep_sky);
    }

    for (size_t i = 0; i < body_labels_.size(); ++i) {
        lv_obj_add_flag(body_labels_[i], LV_OBJ_FLAG_HIDDEN);
        if (i >= bodies_.size()) continue;
        const auto& body = bodies_[i];
        if (body.horizontal.altitude_deg < -1.0) continue; // set: not drawn on the ground
        const auto p = projection.project(body.horizontal.azimuth_deg, body.horizontal.altitude_deg);
        if (!projection.on_screen(p, 6)) continue;
        if (pal.outlined) {
            view::plot_disc(buf, kW, kH, static_cast<int>(p.x), static_cast<int>(p.y),
                            body_radius(body.body) + 1, 0);
        }
        if (body.body == sky::Body::Moon) {
            // The screen direction to the Sun: project a point a little way
            // from the Moon towards the Sun.
            double m[3], sun[3] = {0, 0, 0};
            sky::enu_from_horizontal(body.horizontal.azimuth_deg, body.horizontal.altitude_deg, m);
            for (const auto& other : bodies_) {
                if (other.body == sky::Body::Sun) {
                    sky::enu_from_horizontal(other.horizontal.azimuth_deg,
                                             other.horizontal.altitude_deg, sun);
                }
            }
            double t[3];
            for (int k = 0; k < 3; ++k) t[k] = m[k] + 0.02 * sun[k];
            const double t_alt = std::atan2(t[2], std::hypot(t[0], t[1])) * 180.0 / M_PI;
            const double t_az = std::atan2(t[0], t[1]) * 180.0 / M_PI;
            const auto toward = projection.project(t_az, t_alt);
            const double sdx = toward.visible ? toward.x - p.x : 1.0;
            const double sdy = toward.visible ? toward.y - p.y : 0.0;
            const uint16_t dim = vm_.night_theme() ? red565(40) : rgb565(60, 60, 70);
            draw_moon(buf, static_cast<int>(p.x), static_cast<int>(p.y), body_radius(body.body),
                      sdx, sdy, body.phase_fraction, pal.highlight, dim);
        } else {
            view::plot_disc(buf, kW, kH, static_cast<int>(p.x), static_cast<int>(p.y),
                            body_radius(body.body), pal.highlight);
        }
        lv_label_set_text(body_labels_[i], body_name(body.body));
        place_label(body_labels_[i], p, body_radius(body.body) + 2, -6);
    }

    for (size_t i = 0; i < cardinals_.size(); ++i) {
        lv_obj_add_flag(cardinals_[i], LV_OBJ_FLAG_HIDDEN);
        const auto p = projection.project(45.0 * static_cast<double>(i), 0.0);
        if (!projection.on_screen(p, -4)) continue;
        place_label(cardinals_[i], p, -4, 2);
    }

    render_guidance(buf, projection);

    // Keep the new labels away from the status, NavBar and existing object
    // names. A small fixed pool avoids creating hundreds of LVGL objects for
    // the 605 deep-sky entries, and the projected points still track the sky.
    for (auto* label : constellation_labels_) lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
    for (auto* label : deep_labels_) lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
    std::vector<LabelBox> occupied{{0, 0, kW, 34}, {0, 143, kW, kH - 143}};
    const auto reserve_visible = [&](lv_obj_t* label) {
        if (!lv_obj_has_flag(label, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_update_layout(label);
            occupied.push_back({static_cast<int>(lv_obj_get_x(label)),
                                static_cast<int>(lv_obj_get_y(label)),
                                static_cast<int>(lv_obj_get_width(label)),
                                static_cast<int>(lv_obj_get_height(label))});
        }
    };
    for (auto* label : body_labels_) reserve_visible(label);
    for (auto* label : cardinals_) reserve_visible(label);
    reserve_visible(target_label_);
    const auto place_new_label = [&](lv_obj_t* label, const std::string& name,
                                     const sky::ScreenPoint& p, bool beside_symbol) {
        lv_label_set_text(label, name.c_str());
        lv_obj_remove_flag(label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_update_layout(label);
        const int w = static_cast<int>(lv_obj_get_width(label));
        const int h = static_cast<int>(lv_obj_get_height(label));
        const int y = static_cast<int>(std::lround(p.y)) - h / 2;
        int x = static_cast<int>(std::lround(p.x)) + (beside_symbol ? 7 : -w / 2);
        if (beside_symbol && x + w > kW - 2)
            x = static_cast<int>(std::lround(p.x)) - w - 7;
        const LabelBox box{x, y, w, h};
        if (x < 2 || x + w > kW - 2 || y < 34 || y + h > 142 ||
            std::any_of(occupied.begin(), occupied.end(),
                        [&](const LabelBox& other) { return overlaps(box, other); })) {
            lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
            return false;
        }
        lv_obj_set_pos(label, x, y);
        occupied.push_back(box);
        return true;
    };

    if (vm_.deep_sky_visible() && !deep_dimmed) {
        std::vector<size_t> order;
        order.reserve(deep.size());
        for (size_t i = 0; i < deep.size() && i < deep_sky_positions_.size(); ++i) {
            if (!deep[i].messier.empty() || deep[i].magnitude <= deep_limit) order.push_back(i);
        }
        std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
            const bool ma = !deep[a].messier.empty(), mb = !deep[b].messier.empty();
            if (ma != mb) return ma;
            const double aa = std::isfinite(deep[a].magnitude) ? deep[a].magnitude : 99.0;
            const double bb = std::isfinite(deep[b].magnitude) ? deep[b].magnitude : 99.0;
            return aa < bb;
        });
        size_t used = 0;
        for (const size_t i : order) {
            if (used == deep_labels_.size()) break;
            const auto& h = deep_sky_positions_[i];
            if (h.altitude_deg < -1.0) continue;
            const auto p = projection.project(h.azimuth_deg, h.altitude_deg);
            if (!projection.on_screen(p, 8)) continue;
            if (place_new_label(deep_labels_[used], deep[i].label(), p, true)) ++used;
        }
    }
    if (vm_.constellation_display() == ConstellationDisplay::Names) {
        std::vector<size_t> order;
        const auto& names = constellation_names_.names();
        order.reserve(names.size());
        for (size_t i = 0; i < names.size() && i < constellation_name_positions_.size(); ++i)
            order.push_back(i);
        std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
            return names[a].rank < names[b].rank;
        });
        size_t used = 0;
        for (const size_t i : order) {
            if (used == constellation_labels_.size()) break;
            const auto& h = constellation_name_positions_[i];
            if (h.altitude_deg < 0.0) continue;
            const auto p = projection.project(h.azimuth_deg, h.altitude_deg);
            if (!projection.on_screen(p, 8)) continue;
            if (place_new_label(constellation_labels_[used], names[i].latin_name, p, false)) ++used;
        }
    }

    // Status: the camera numbers, the position, and on the second line the
    // mode or why the IMU is not steering.
    char status[64];
    std::snprintf(status, sizeof status, "AZ %03.0f  ALT %+03.0f  FOV %.0f", cam.azimuth_deg,
                  cam.altitude_deg, cam.fov_deg);
    lv_label_set_text(status_, status);
    lv_label_set_text(place_, upper(vm_.location_set() ? vm_.location_label()
                                                       : std::string(pick(text::kNoPosition))).c_str());

    std::string notice;
    const std::string attitude = vm_.attitude_status();
    if (vm_.aligning()) {
        char align[96];
        std::snprintf(align, sizeof align, pick(text::kAlignStatus), vm_.alignment().fov_deg,
                      vm_.alignment().right_offset_deg, vm_.alignment().up_offset_deg);
        notice = align;
    } else {
        if (!attitude.empty()) notice = localized_attitude(attitude);
        else if (vm_.manual()) notice = pick(text::kManual);
        // The lens FOV is an estimate until an alignment is saved.
        if (vm_.camera_on() && !vm_.alignment().measured) {
            if (!notice.empty()) notice += "  ";
            notice += pick(text::kFovEstimate);
        }
    }
    notice = upper(notice);
    lv_label_set_text(notice_, notice.c_str());
    if (notice.empty()) lv_obj_add_flag(notice_, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(notice_, LV_OBJ_FLAG_HIDDEN);

    // Align: a crosshair on the screen centre, the point the offsets move.
    if (vm_.aligning()) {
        const uint16_t c = pal.highlight;
        mark_line(buf, kW / 2 - 8, kH / 2, kW / 2 - 3, kH / 2, c, true);
        mark_line(buf, kW / 2 + 3, kH / 2, kW / 2 + 8, kH / 2, c, true);
        mark_line(buf, kW / 2, kH / 2 - 8, kW / 2, kH / 2 - 3, c, true);
        mark_line(buf, kW / 2, kH / 2 + 3, kW / 2, kH / 2 + 8, c, true);
    }
    update_panel(photo);
    lv_obj_invalidate(canvas_);
}

// The panel tells what the user cannot see: the camera still starting (or
// failing), and the Align steps while aligning.
void SkyScreen::update_panel(bool photo) {
    const char* text = nullptr;
    bool centred = true;
    if (vm_.language_version() != seen_language_version_) {
        seen_language_version_ = vm_.language_version();
        language_note_until_ = std::chrono::steady_clock::now() + std::chrono::seconds(4);
    }
    if (vm_.camera_on() && camera_ && !photo) {
        const bool late = std::chrono::steady_clock::now() - camera_started_at_ > std::chrono::seconds(10);
        text = pick(late ? text::kCameraFailed : text::kCameraStarting);
    } else if (language_note_until_ > std::chrono::steady_clock::now()) {
        text = pick(vm_.language_saved() ? text::kLanguageRestart : text::kLanguageNotSaved);
    } else if (vm_.aligning()) {
        // The steps for 10 s from when the panel is first free in Align (after
        // the camera start-up or a language note); then the sky stays clear.
        const auto now = std::chrono::steady_clock::now();
        if (!align_steps_started_) {
            align_started_at_ = now;
            align_steps_started_ = true;
        }
        if (now - align_started_at_ < std::chrono::seconds(10)) text = pick(text::kAlignSteps);
        centred = false; // keep the centre free for the crosshair and the star
    }
    if (!vm_.aligning()) align_steps_started_ = false;
    if (!text) {
        lv_obj_add_flag(panel_, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_label_set_text(panel_text_, text);
    if (centred) lv_obj_align(panel_, LV_ALIGN_CENTER, 0, -10);
    else lv_obj_align(panel_, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_remove_flag(panel_, LV_OBJ_FLAG_HIDDEN);
}

bool SkyScreen::locate(const std::string& name, sky::Horizontal& out) const {
    for (const auto& body : bodies_) {
        if (name == body.name || name == body_name(body.body)) {
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
        const char* kind = body.body == sky::Body::Sun ? pick(text::kKindSun)
                         : body.body == sky::Body::Moon ? pick(text::kKindMoon) : pick(text::kKindPlanet);
        items.push_back({body_name(body.body), kind, body.horizontal.altitude_deg > 0.0});
    }
    const auto& stars = catalog_.stars();
    for (size_t i = 0; i < stars.size() && i < star_positions_.size(); ++i) {
        if (stars[i].name.empty()) continue;
        char detail[48];
        std::snprintf(detail, sizeof detail, "%s mag %.1f %s", pick(text::kKindStar),
                      stars[i].magnitude, stars[i].constellation.c_str());
        items.push_back({stars[i].name, detail, star_positions_[i].altitude_deg > 0.0});
    }
    const auto& deep = deep_sky_.objects();
    for (size_t i = 0; i < deep.size() && i < deep_sky_positions_.size(); ++i) {
        char detail[48];
        std::snprintf(detail, sizeof detail, "%s %s", pick(text::kDeepSkyKinds[static_cast<int>(deep[i].kind)]),
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
    const uint16_t highlight = sky_palette(vm_.night_theme(), label_style_ >= 2).highlight;
    const auto g = sky::guide(vm_.camera(), where.azimuth_deg, where.altitude_deg);
    // A target below the horizon keeps its arrow, so you know where it is,
    // but no beep: there is nothing to point at.
    const bool set = where.altitude_deg < 0.0;
    if (set) beeper_.stop();
    else beeper_.set_guidance(g.separation_deg);

    const auto p = projection.project(where.azimuth_deg, where.altitude_deg);
    char text[64];
    std::snprintf(text, sizeof text, "%s %s%s%.0f\xC2\xB0", vm_.target().c_str(),
                  set ? pick(text::kSet) : "", set ? " " : "", g.separation_deg);
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
    // The manual in the user's language: docs/manual.it.md and so on.
    const std::string url = toolkit::localized_doc_path(
        ZEROSKY_MANUAL_URL, toolkit::i18n::code(toolkit::i18n::current()));
    Page info;
    info.title = pick(text::kAboutTitle);
    info.markdown = std::string("# ZeroSky " APP_VERSION "\n") + pick(text::kAboutBody);
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
        assets(), vm_.is_dark_mode(), std::vector<Page>{info, {pick(text::kCredits), credits, {}}});
}

void SkyScreen::open_location_dialog() {
    location_dialog_ = std::make_unique<toolkit::LocationDialog>(
        assets(), vm_.is_dark_mode(), [this](const toolkit::location::Place& place) {
            toolkit::location::save(place);
            vm_.set_location(place.pos.lat, place.pos.lon, place.label);
        });
}

} // namespace zerosky
