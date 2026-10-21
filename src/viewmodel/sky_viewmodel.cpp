/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "sky_viewmodel.h"

#include "location.h"
#include "persisted_state.h"
#include "theme.h"
#include "ui_const.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>

namespace zerosky {
namespace {

constexpr const char* kHardware[] = {"none", "cap", "usb_gps"};

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

} // namespace

SkyViewModel::SkyViewModel() {
    set_title("ZeroSky");
    set_nav_provider(this); // pages, holds and key hints go through the shell
    set_help_doc("docs/help/zerosky.md"); // H
    hardware_ = load_hardware();
    if (const auto place = toolkit::location::load()) {
        set_location(place->pos.lat, place->pos.lon, place->label);
    }
    // A starting search target, for demos and screenshots.
    if (const char* target = std::getenv("ZEROSKY_TARGET")) target_ = target;
    if (const char* theme = std::getenv("ZEROSKY_THEME")) night_theme_ = std::string(theme) != "day";
    if (const char* light = std::getenv("ZEROSKY_DAYLIGHT")) daylight_ = std::string(light) != "all";
    // The widgets follow the sky: night = dark mode with the red palette,
    // day = the light toolkit theme.
    if (const char* cam = std::getenv("ZEROSKY_CAMERA"); cam && std::string(cam) == "on") {
        camera_on_ = true;
        camera_.fov_deg = lens_fov_deg();
    }
    view::set_dark_palette_override(&kNightWidgets);
    set_dark_mode(night_theme_);
}

// The horizontal field of view of the camera lens. The CardputerZero lens is
// not documented: 62.2 degrees is the Raspberry Pi Camera v2, same IMX219
// sensor, until it is measured (docs/plan-phase2.md). ZEROSKY_CAMERA_FOV
// overrides it.
double SkyViewModel::lens_fov_deg() {
    if (const char* fov = std::getenv("ZEROSKY_CAMERA_FOV")) {
        const double value = std::atof(fov);
        if (value > 10.0 && value < 170.0) return value;
    }
    return 62.2;
}

void SkyViewModel::nav_fill(int page, NavSlot out[5]) const {
    if (page == 2) {
        out[1] = NavSlot{camera_on_ ? "On" : "Off", true, true};
        for (int slot = 2; slot < 5; ++slot) out[slot] = NavSlot{"", false, false};
        return;
    }
    if (page == 1) {
        out[1] = NavSlot{night_theme_ ? view::ICON_MOON : view::ICON_SUN, false, true};
        out[2] = NavSlot{daylight_ ? "Dim" : "All", true, true};
        out[3] = NavSlot{hardware_ == "usb_gps" ? "M:USB" : hardware_ == "cap" ? "M:Cap" : "M:-",
                         true, true};
        out[4] = NavSlot{view::ICON_INFO, false, true};
        return;
    }
    out[1] = NavSlot{view::ICON_MAP_TOGGLE, false, true};
    out[2] = NavSlot{view::ICON_MINUS, false, camera_.fov_deg < 120.0};
    out[3] = NavSlot{view::ICON_PLUS, false, camera_.fov_deg > 15.0};
    out[4] = NavSlot{"Find", true, true};
}

void SkyViewModel::nav_activate(int page, int slot) {
    if (page == 2) {
        if (slot == 1) {
            camera_on_ = !camera_on_;
            if (camera_on_) {
                zoom_fov_deg_ = camera_.fov_deg;
                camera_.fov_deg = lens_fov_deg();
            } else {
                camera_.fov_deg = zoom_fov_deg_;
            }
            bump_nav_refresh();
        }
        return;
    }
    if (page == 1) {
        if (slot == 1) {
            night_theme_ = !night_theme_;
            set_dark_mode(night_theme_);
            bump_nav_refresh();
        } else if (slot == 2) {
            daylight_ = !daylight_;
            bump_nav_refresh();
        } else if (slot == 3) {
            cycle_hardware();
        } else if (slot == 4) {
            about_request_ = true;
        }
        return;
    }
    switch (slot) {
        case 1: location_request_ = true; break;
        case 2: zoom(1.25); break;
        case 3: zoom(0.8); break;
        case 4: search_request_ = true; break;
        default: break;
    }
}

toolkit::i18n::Text SkyViewModel::nav_hint(int page, int slot) const {
    if (page == 2) {
        if (slot != 1) return {};
        return {"Camera image behind the sky, on or off", "Immagine della camera sotto il cielo, sì o no",
                "Image de la caméra sous le ciel, oui ou non", "Kamerabild hinter dem Himmel, an oder aus",
                "Imagen de la cámara bajo el cielo, sí o no", "空の背景にカメラ映像を表示（オン／オフ）",
                "天空背景显示相机画面（开／关）"};
    }
    if (page == 1) {
        switch (slot) {
            case 1: return {"Night theme (red) or day theme (white and blue)",
                            "Tema notte (rosso) o giorno (bianco e blu)",
                            "Thème nuit (rouge) ou jour (blanc et bleu)",
                            "Nachtmodus (rot) oder Tagmodus (weiß und blau)",
                            "Tema noche (rojo) o día (blanco y azul)",
                            "夜テーマ（赤）または昼テーマ（白と青）", "夜间主题（红）或白天主题（白与蓝）"};
            case 2: return {"Daylight: Dim hides faint stars by day, All shows them",
                            "Luce del giorno: Dim nasconde le stelle deboli, All le mostra",
                            "Lumière du jour : Dim masque les étoiles faibles, All les montre",
                            "Tageslicht: Dim blendet schwache Sterne aus, All zeigt sie",
                            "Luz del día: Dim oculta las estrellas débiles, All las muestra",
                            "昼光：Dim は暗い星を隠し、All は表示", "日光：Dim 隐藏暗星，All 显示"};
            case 3: return {"Magnetic profile for the compass: none, Cap LoRa or USB GPS",
                            "Profilo magnetico della bussola: niente, Cap LoRa o GPS USB",
                            "Profil magnétique de la boussole : aucun, Cap LoRa ou GPS USB",
                            "Magnetprofil des Kompasses: keins, Cap LoRa oder USB-GPS",
                            "Perfil magnético de la brújula: ninguno, Cap LoRa o GPS USB",
                            "コンパスの磁気プロファイル：なし、Cap LoRa、USB GPS",
                            "指南针磁性配置：无、Cap LoRa 或 USB GPS"};
            case 4: return {"About ZeroSky and the manual", "Informazioni su ZeroSky e manuale",
                            "À propos de ZeroSky et manuel", "Über ZeroSky und Handbuch",
                            "Acerca de ZeroSky y manual", "ZeroSky について・マニュアル", "关于 ZeroSky 与手册"};
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
        case 4: return {"Find an object by name, hold to stop",
                        "Cerca un oggetto per nome, tieni premuto per smettere",
                        "Chercher un objet par son nom, maintenir pour arrêter",
                        "Objekt nach Namen suchen, halten zum Beenden",
                        "Buscar un objeto por nombre, mantener para parar",
                        "名前で天体を探す、長押しで終了", "按名称查找天体，长按停止"};
        default: return {};
    }
}

std::string SkyViewModel::nav_hold(int page, int slot) {
    if (page != 0 || slot != 4) return {};
    target_.clear();
    return "Search off";
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

bool SkyViewModel::take_about_request() {
    const bool request = about_request_;
    about_request_ = false;
    return request;
}

bool SkyViewModel::take_search_request() {
    const bool request = search_request_;
    search_request_ = false;
    return request;
}

void SkyViewModel::set_target(std::string name) { target_ = std::move(name); }

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
    bump_nav_refresh();
}

} // namespace zerosky
