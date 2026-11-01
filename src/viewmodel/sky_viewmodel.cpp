/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "sky_viewmodel.h"

#include "location.h"
#include "persisted_state.h"
#include "theme.h"
#include "sky_text.h"
#include "ui_const.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>

namespace zerosky {
namespace {

constexpr const char* kHardware[] = {"none", "cap", "usb_gps"};
// The Setup page's language key, in toolkit::i18n::Lang order.
constexpr const char* kLanguageLabels[] = {"EN", "IT", "FR", "DE", "ES", "JA", "ZH"};

// The night theme of the toolkit widgets (NavBar, dialogs, help): red on
// black, so nothing white breaks the dark adaptation.
const view::ThemePalette kNightWidgets{
    lv_color_hex(0x000000), lv_color_hex(0x140202), lv_color_hex(0x0c0101),
    lv_color_hex(0x240404), lv_color_hex(0x501008), lv_color_hex(0xd02810),
    lv_color_hex(0x581008), lv_color_hex(0xff3820), lv_color_hex(0xa02010)};

std::string load_hardware() {
    if (const char* forced = std::getenv("ZEROSKY_HARDWARE"); forced && forced[0]) return forced;
    std::ifstream in(toolkit::config_file("zerosky", "hardware"));
    std::string word;
    if (in >> word) {
        for (const char* known : kHardware) {
            if (word == known) return word;
        }
    }
    return "none";
}

void save_hardware(const std::string& hardware) {
    const auto path = toolkit::config_file("zerosky", "hardware");
    if (!toolkit::ensure_parent_dir(path)) return;
    std::ofstream(path) << hardware << "\n";
}

std::filesystem::path settings_file() { return toolkit::config_file("zerosky", "settings"); }
std::filesystem::path alignment_file() { return toolkit::config_file("zerosky", "camera-alignment"); }

// Phosphor-Fill glyphs, checked by rendering the bundled font (the toolkit's
// ui_const.h has no search, camera or crosshair icon).
constexpr const char* kIconSearch = "\uE30C";      // magnifying glass
constexpr const char* kIconStopGuide = "\uE46E";   // stop-circle: guidance is on
constexpr const char* kIconCamera = "\uE10E";      // camera
constexpr const char* kIconCameraOff = "\uE110";   // camera slash
constexpr const char* kIconCrosshair = "\uE1D6";   // crosshair
constexpr const char* kIconStarHalf = "\uE70A";    // star-half: Dim, faint stars hidden by day
constexpr const char* kIconStar = "\uE46A";        // star: All
constexpr const char* kIconTranslate = "\uE4A2";   // translate: the language follows the system
constexpr const char* kIconProhibit = "\uE3DE";    // prohibit: no magnetic accessory
constexpr const char* kIconBroadcast = "\uE0F2";   // broadcast: Cap LoRa
constexpr const char* kIconUsb = "\uE956";         // usb: USB GPS
constexpr const char* kIconCompass = "\uE1C8";     // compass
constexpr const char* kIconNoLines = "\uE224";     // eye-slash
constexpr const char* kIconLines = "\uE6D4";       // line-segments
constexpr const char* kIconNamedLines = "\uEBDC";  // list-star
constexpr const char* kIconDeepSky = "\uE192";     // circles-three
constexpr const char* kIconDeepSkyOff = "\uE602";  // circle-dashed

constexpr double kAlignStepDeg = 0.5;
constexpr double kAlignFovStepDeg = 1.0;

} // namespace

SkyViewModel::SkyViewModel() {
    set_title("ZeroSky");
    set_nav_provider(this); // pages, holds and key hints go through the shell
    set_help_doc("docs/help/zerosky.md"); // H
    hardware_ = load_hardware();
    language_choice_ = toolkit::i18n::load_choice(toolkit::i18n::choice_file());
    // The choices of the last run; the ZEROSKY_* variables below override them.
    const ViewSettings settings = load_view_settings(settings_file());
    night_theme_ = settings.night_theme;
    daylight_ = settings.daylight;
    constellations_ = settings.constellations;
    deep_sky_visible_ = settings.deep_sky;
    zoom_fov_deg_ = settings.zoom_fov_deg;
    camera_.fov_deg = settings.zoom_fov_deg;
    alignment_ = load_camera_alignment(alignment_file());
    if (settings.camera_on) {
        camera_on_ = true;
        camera_.fov_deg = lens_fov_deg();
    }
    if (const auto place = toolkit::location::load()) {
        set_location(place->pos.lat, place->pos.lon, place->label);
    }
    // A starting search target, for demos and screenshots.
    if (const char* target = std::getenv("ZEROSKY_TARGET")) target_ = target;
    if (const char* theme = std::getenv("ZEROSKY_THEME")) night_theme_ = std::string(theme) != "day";
    if (const char* light = std::getenv("ZEROSKY_DAYLIGHT")) daylight_ = std::string(light) != "all";
    // The widgets follow the sky: night = dark mode with the red palette,
    // day = the light toolkit theme.
    if (const char* cam = std::getenv("ZEROSKY_CAMERA")) {
        const std::string value = cam;
        if (value == "on") {
            camera_on_ = true;
            camera_.fov_deg = lens_fov_deg();
        } else if (value == "off" && camera_on_) {
            camera_on_ = false;
            camera_.fov_deg = zoom_fov_deg_;
        }
    }
    if (const char* align = std::getenv("ZEROSKY_ALIGN"); align && std::string(align) == "on") {
        set_aligning(true); // demos and screenshots; needs the camera on
    }
    // Headless visual review: open the synthetic calibration without sensor hardware.
    if (const char* demo = std::getenv("ZEROSKY_CALIBRATION_DEMO"); demo && demo[0] &&
        std::string(demo) != "0") calibration_request_ = true;
    view::set_dark_palette_override(&kNightWidgets);
    set_dark_mode(night_theme_);
}

SkyViewModel::~SkyViewModel() {
    if (zoom_dirty_) save_settings();
}

// The horizontal field of view of the camera lens: the aligned value, else
// 62.2 degrees, the Raspberry Pi Camera v2 with the same IMX219, until it is
// measured (docs/plan-phase2.md). ZEROSKY_CAMERA_FOV overrides it.
double SkyViewModel::lens_fov_deg() const {
    // While aligning, the keys set the FOV: an override would hide them.
    if (const char* fov = std::getenv("ZEROSKY_CAMERA_FOV"); fov && !aligning_) {
        const double value = std::atof(fov);
        if (value > 10.0 && value < 170.0) return value;
    }
    return alignment_.fov_deg;
}

sky::Camera SkyViewModel::camera() const {
    if (!camera_on_) return camera_;
    return sky::offset_camera(camera_, alignment_.right_offset_deg, alignment_.up_offset_deg);
}

void SkyViewModel::save_settings() const {
    ViewSettings settings;
    settings.night_theme = night_theme_;
    settings.daylight = daylight_;
    settings.constellations = constellations_;
    settings.deep_sky = deep_sky_visible_;
    settings.camera_on = camera_on_;
    settings.zoom_fov_deg = camera_on_ ? zoom_fov_deg_ : camera_.fov_deg;
    save_view_settings(settings_file(), settings);
}

void SkyViewModel::set_camera_on(bool on) {
    if (on == camera_on_) return;
    camera_on_ = on;
    if (on) {
        zoom_fov_deg_ = camera_.fov_deg;
        camera_.fov_deg = lens_fov_deg();
    } else {
        // Leaving the camera in the middle of Align cancels it: only Done saves.
        if (aligning_) {
            aligning_ = false;
            alignment_ = load_camera_alignment(alignment_file());
        }
        camera_.fov_deg = zoom_fov_deg_;
    }
    save_settings();
    bump_nav_refresh();
}

void SkyViewModel::set_aligning(bool on) {
    if (on == aligning_) return;
    aligning_ = on && camera_on_;
    if (aligning_) {
        alignment_at_start_ = alignment_;
        camera_.fov_deg = lens_fov_deg();
    } else {
        // Done: saved, and marked measured, only when something was adjusted.
        const bool changed = alignment_.fov_deg != alignment_at_start_.fov_deg ||
                             alignment_.right_offset_deg != alignment_at_start_.right_offset_deg ||
                             alignment_.up_offset_deg != alignment_at_start_.up_offset_deg;
        if (changed) {
            alignment_.measured = true;
            save_camera_alignment(alignment_file(), alignment_);
        }
        camera_.fov_deg = lens_fov_deg();
    }
    bump_nav_refresh();
}

const char* SkyViewModel::nav_page_name(int page) const {
    using toolkit::i18n::pick;
    switch (page) {
        case 0: return pick(text::kPageSky);
        case 1: return pick(text::kPageView);
        case 2: return pick(text::kPageCam);
        default: return pick(text::kPageSetup);
    }
}

void SkyViewModel::nav_fill(int page, NavSlot out[5]) const {
    using toolkit::i18n::pick;
    if (page == 3) {
        // A 32 px key: two-letter codes and icons, never words that get cut.
        if (language_choice_) {
            out[1] = NavSlot{kLanguageLabels[static_cast<int>(*language_choice_)], true, true};
        } else {
            out[1] = NavSlot{kIconTranslate, false, true};
        }
        out[2] = NavSlot{hardware_ == "usb_gps" ? kIconUsb
                         : hardware_ == "cap"   ? kIconBroadcast
                                                : kIconProhibit,
                         false, true};
        out[3] = NavSlot{view::ICON_INFO, false, true};
        out[4] = NavSlot{kIconCompass, false, true};
        return;
    }
    if (page == 2) {
        out[1] = NavSlot{camera_on_ ? kIconCamera : kIconCameraOff, false, true};
        out[2] = NavSlot{aligning_ ? view::ICON_CHECK : kIconCrosshair, false, camera_on_};
        out[3] = NavSlot{view::ICON_MINUS, false, aligning_};
        out[4] = NavSlot{view::ICON_PLUS, false, aligning_};
        return;
    }
    if (page == 1) {
        out[1] = NavSlot{night_theme_ ? view::ICON_MOON : view::ICON_SUN, false, true};
        out[2] = NavSlot{daylight_ ? kIconStarHalf : kIconStar, false, true};
        out[3] = NavSlot{constellations_ == ConstellationDisplay::Off ? kIconNoLines
                         : constellations_ == ConstellationDisplay::Lines ? kIconLines
                                                                          : kIconNamedLines,
                         false, true};
        out[4] = NavSlot{deep_sky_visible_ ? kIconDeepSky : kIconDeepSkyOff, false, true};
        return;
    }
    out[1] = NavSlot{view::ICON_MAP_TOGGLE, false, true};
    // With the camera image on, the lens sets the field of view: no zoom.
    out[2] = NavSlot{view::ICON_MINUS, false, !camera_on_ && camera_.fov_deg < 120.0};
    out[3] = NavSlot{view::ICON_PLUS, false, !camera_on_ && camera_.fov_deg > 15.0};
    // While a target is guided, the key shows a stop sign and stops it.
    out[4] = NavSlot{target_.empty() ? kIconSearch : kIconStopGuide, false, true};
}

void SkyViewModel::nav_activate(int page, int slot) {
    if (page == 3) {
        if (slot == 1) cycle_language();
        else if (slot == 2) cycle_hardware();
        else if (slot == 3) about_request_ = true;
        else if (slot == 4) calibration_request_ = true;
        return;
    }
    if (page == 2) {
        if (slot == 1) {
            set_camera_on(!camera_on_);
        } else if (slot == 2) {
            set_aligning(!aligning_);
        } else if (aligning_ && (slot == 3 || slot == 4)) {
            const double step = slot == 3 ? -kAlignFovStepDeg : kAlignFovStepDeg;
            alignment_.fov_deg = std::clamp(alignment_.fov_deg + step, 20.0, 120.0);
            camera_.fov_deg = lens_fov_deg();
        }
        return;
    }
    if (page == 1) {
        if (slot == 1) {
            night_theme_ = !night_theme_;
            set_dark_mode(night_theme_);
            save_settings();
            bump_nav_refresh();
        } else if (slot == 2) {
            daylight_ = !daylight_;
            save_settings();
            bump_nav_refresh();
        } else if (slot == 3) {
            constellations_ = static_cast<ConstellationDisplay>(
                (static_cast<int>(constellations_) + 1) % 3);
            save_settings();
            bump_nav_refresh();
        } else if (slot == 4) {
            deep_sky_visible_ = !deep_sky_visible_;
            save_settings();
            bump_nav_refresh();
        }
        return;
    }
    switch (slot) {
        case 1: location_request_ = true; break;
        case 2: zoom(1.25); break;
        case 3: zoom(0.8); break;
        case 4:
            if (target_.empty()) {
                search_request_ = true;
            } else {
                target_.clear();
                bump_nav_refresh();
            }
            break;
        default: break;
    }
}

toolkit::i18n::Text SkyViewModel::nav_hint(int page, int slot) const {
    if (page == 3) {
        switch (slot) {
            case 1: return {"Language from the next start; the translate icon follows the system",
                            "Lingua dal prossimo avvio; l'icona traduci segue il sistema",
                            "Langue au prochain démarrage ; l'icône traduire suit le système",
                            "Sprache ab dem nächsten Start; das Übersetzen-Symbol folgt dem System",
                            "Idioma desde el próximo inicio; el icono traducir sigue el sistema",
                            "次回起動から言語を変更。翻訳アイコンはシステムに従う",
                            "下次启动时切换语言；翻译图标表示跟随系统"};
            case 2: return {"Compass magnetic profile: no sign = none, antenna = Cap LoRa, USB = USB GPS",
                            "Profilo magnetico della bussola: divieto = niente, antenna = Cap LoRa, USB = GPS USB",
                            "Profil magnétique : interdit = aucun, antenne = Cap LoRa, USB = GPS USB",
                            "Magnetprofil: Verbot = keins, Antenne = Cap LoRa, USB = USB-GPS",
                            "Perfil magnético: prohibido = ninguno, antena = Cap LoRa, USB = GPS USB",
                            "磁気プロファイル：禁止＝なし、アンテナ＝Cap LoRa、USB＝USB GPS",
                            "磁性配置：禁止＝无，天线＝Cap LoRa，USB＝USB GPS"};
            case 3: return {"About ZeroSky and the manual", "Informazioni su ZeroSky e manuale",
                            "À propos de ZeroSky et manuel", "Über ZeroSky und Handbuch",
                            "Acerca de ZeroSky y manual", "ZeroSky について・マニュアル", "关于 ZeroSky 与手册"};
            case 4: return text::kCalibrationNavHint;
            default: return {};
        }
    }
    if (page == 2) {
        switch (slot) {
            case 1: return {"Camera image behind the sky, on or off",
                            "Immagine della camera sotto il cielo, sì o no",
                            "Image de la caméra sous le ciel, oui ou non",
                            "Kamerabild hinter dem Himmel, an oder aus",
                            "Imagen de la cámara bajo el cielo, sí o no",
                            "空の背景にカメラ映像を表示（オン／オフ）", "天空背景显示相机画面（开／关）"};
            case 2: return {"Align: arrows move the drawn sky onto the photo, the check mark saves",
                            "Allinea: le frecce spostano il cielo disegnato sulla foto, la spunta salva",
                            "Aligner : les flèches déplacent le ciel dessiné sur la photo, la coche enregistre",
                            "Ausrichten: Pfeile schieben den Himmel aufs Foto, das Häkchen speichert",
                            "Alinear: las flechas mueven el cielo dibujado sobre la foto, la marca guarda",
                            "位置合わせ：矢印で描画を写真に重ね、チェックで保存",
                            "对齐：方向键把绘制的星空移到照片上，勾号保存"};
            case 3: return {"Align: narrower lens field of view", "Allinea: campo della lente più stretto",
                            "Aligner : champ de l'objectif plus étroit", "Ausrichten: engeres Bildfeld",
                            "Alinear: campo de la lente más estrecho", "位置合わせ：画角を狭く", "对齐：缩小视场"};
            case 4: return {"Align: wider lens field of view", "Allinea: campo della lente più largo",
                            "Aligner : champ de l'objectif plus large", "Ausrichten: weiteres Bildfeld",
                            "Alinear: campo de la lente más amplio", "位置合わせ：画角を広く", "对齐：扩大视场"};
            default: return {};
        }
    }
    if (page == 1) {
        switch (slot) {
            case 1: return {"Night theme (red) or day theme (white and blue)",
                            "Tema notte (rosso) o giorno (bianco e blu)",
                            "Thème nuit (rouge) ou jour (blanc et bleu)",
                            "Nachtmodus (rot) oder Tagmodus (weiß und blau)",
                            "Tema noche (rojo) o día (blanco y azul)",
                            "夜テーマ（赤）または昼テーマ（白と青）", "夜间主题（红）或白天主题（白与蓝）"};
            case 2: return {"Daylight: half star hides faint stars by day, full star shows them all",
                            "Luce del giorno: mezza stella nasconde le stelle deboli, stella piena le mostra tutte",
                            "Lumière du jour : demi-étoile masque les étoiles faibles, étoile pleine les montre toutes",
                            "Tageslicht: halber Stern blendet schwache Sterne aus, voller Stern zeigt alle",
                            "Luz del día: media estrella oculta las estrellas débiles, estrella llena las muestra todas",
                            "昼光：半分の星は暗い星を隠し、満ちた星はすべて表示",
                            "日光：半星隐藏暗星，满星全部显示"};
            case 3:
                if (constellations_ == ConstellationDisplay::Off)
                    return text::kConstellationsOffHint;
                if (constellations_ == ConstellationDisplay::Lines)
                    return text::kConstellationsLinesHint;
                return text::kConstellationsNamesHint;
            case 4: return deep_sky_visible_ ? text::kDeepSkyOnHint : text::kDeepSkyOffHint;
            default: return {};
        }
    }
    switch (slot) {
        case 1: return {"Set your position", "Imposta la posizione", "Définir la position",
                        "Position festlegen", "Fijar la posición", "位置を設定", "设置位置"};
        case 2: return {"Zoom out", "Riduci lo zoom", "Dézoomer", "Verkleinern", "Alejar",
                        "縮小", "缩小"};
        case 3: return {"Zoom in", "Aumenta lo zoom", "Zoomer", "Vergrößern", "Acercar",
                        "拡大", "放大"};
        case 4:
            if (!target_.empty())
                return {"Stop guidance and beeps", "Ferma la guida e i bip",
                        "Arrêter le guidage et les bips", "Führung und Pieptöne beenden",
                        "Detener la guía y los pitidos", "誘導とビープ音を止める", "停止引导和提示音"};
            return {"Find an object by name", "Cerca un oggetto per nome",
                    "Chercher un objet par son nom", "Objekt nach Namen suchen",
                    "Buscar un objeto por nombre", "名前で天体を探す", "按名称查找天体"};
        default: return {};
    }
}

std::string SkyViewModel::nav_hold(int page, int slot) {
    if (page != 0 || slot != 4) return {};
    target_.clear();
    bump_nav_refresh();
    return toolkit::i18n::pick(text::kSearchOff);
}

void SkyViewModel::cycle_hardware() {
    size_t next = 0;
    for (size_t i = 0; i < std::size(kHardware); ++i) {
        if (hardware_ == kHardware[i]) next = (i + 1) % std::size(kHardware);
    }
    hardware_ = kHardware[next];
    save_hardware(hardware_);
    attitude_.select_hardware(hardware_);
    bump_nav_refresh();
}

// Auto, then each language in Lang order, then Auto again.
void SkyViewModel::cycle_language() {
    using toolkit::i18n::Lang;
    const auto previous = language_choice_;
    if (!language_choice_) {
        language_choice_ = Lang::En;
    } else if (*language_choice_ == Lang::Zh) {
        language_choice_.reset();
    } else {
        language_choice_ = static_cast<Lang>(static_cast<int>(*language_choice_) + 1);
    }
    language_saved_ = toolkit::i18n::save_choice(toolkit::i18n::choice_file(), language_choice_);
    // The key shows what the next start will use, so a failed write keeps the old choice.
    if (!language_saved_) language_choice_ = previous;
    ++language_version_;
    bump_nav_refresh();
}

bool SkyViewModel::take_about_request() {
    const bool request = about_request_;
    about_request_ = false;
    return request;
}

bool SkyViewModel::take_calibration_request() {
    const bool request = calibration_request_;
    calibration_request_ = false;
    return request;
}

bool SkyViewModel::begin_calibration(std::string& error) {
    if (!location_set_) {
        error = "Set a position before calibrating the compass";
        return false;
    }
    return attitude_.begin_calibration(observer_, hardware_, error);
}

bool SkyViewModel::take_search_request() {
    const bool request = search_request_;
    search_request_ = false;
    return request;
}

void SkyViewModel::set_target(std::string name) {
    target_ = std::move(name);
    bump_nav_refresh(); // key 8 switches between the lens and the stop sign
}

bool SkyViewModel::take_location_request() {
    const bool request = location_request_;
    location_request_ = false;
    return request;
}

void SkyViewModel::set_location(double latitude_deg, double longitude_deg, std::string label) {
    observer_.latitude_deg = latitude_deg;
    observer_.longitude_deg = longitude_deg;
    location_set_ = true;
    location_label_ = std::move(label);
    ++observer_version_;
    restart_attitude();
}

void SkyViewModel::start_attitude(const std::filesystem::path& cof_file) {
    cof_file_ = cof_file;
    cof_missing_ = cof_file.empty() || !std::filesystem::exists(cof_file);
    restart_attitude();
}

void SkyViewModel::restart_attitude() {
    // Waits for both: set_location() runs from the constructor, before the
    // screen hands over the WMM path.
    if (cof_file_.empty() || cof_missing_ || !location_set_) return;
    attitude_.start(observer_, hardware_, cof_file_);
}

void SkyViewModel::update_camera() {
    compass_usable_ = attitude_.compass_usable();
    sky::Camera previous = camera_;
    if (!compass_usable_) previous.azimuth_deg -= azimuth_offset_;
    const auto camera = attitude_.camera(previous);
    attitude_live_ = camera.has_value();
    if (!camera || manual_) return;
    camera_ = *camera;
    if (!compass_usable_) {
        camera_.azimuth_deg = std::fmod(camera_.azimuth_deg + azimuth_offset_ + 720.0, 360.0);
    }
}

void SkyViewModel::turn(double d_azimuth, double d_altitude) {
    // Align mode: the arrows move the drawn sky the way they point, in fine
    // steps, so a drawn star is pushed onto its photographed twin. Moving the
    // drawing right means turning the camera left: hence the minus signs.
    if (aligning_) {
        const double sx = d_azimuth > 0 ? -1.0 : d_azimuth < 0 ? 1.0 : 0.0;
        const double sy = d_altitude > 0 ? -1.0 : d_altitude < 0 ? 1.0 : 0.0;
        alignment_.right_offset_deg =
            std::clamp(alignment_.right_offset_deg + sx * kAlignStepDeg, -15.0, 15.0);
        alignment_.up_offset_deg =
            std::clamp(alignment_.up_offset_deg + sy * kAlignStepDeg, -15.0, 15.0);
        return;
    }
    // With the IMU steering, the arrows only matter in tilt-only mode: left and
    // right line the gyro azimuth up with the real sky.
    if (!manual()) {
        if (tilt_only()) azimuth_offset_ = std::fmod(azimuth_offset_ + d_azimuth + 360.0, 360.0);
        return;
    }
    camera_.azimuth_deg = std::fmod(camera_.azimuth_deg + d_azimuth + 360.0, 360.0);
    camera_.altitude_deg = std::clamp(camera_.altitude_deg + d_altitude, -90.0, 90.0);
}

void SkyViewModel::zoom(double factor) {
    if (camera_on_) return; // the lens sets the field of view
    camera_.fov_deg = std::clamp(camera_.fov_deg * factor, 15.0, 120.0);
    zoom_dirty_ = true; // saved on exit: no SD write on every key repeat
    bump_nav_refresh();
}

} // namespace zerosky
