/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "asset_manager.h"
#include "run_app.h"
#include "sky_screen.h"
#include "sky_viewmodel.h"

#include <memory>

int main() {
    zerosky::SkyViewModel view_model;
    app::AssetManager assets;
    std::unique_ptr<zerosky::SkyScreen> screen;
    return toolkit::run_app(
        view_model, assets,
        [&]() -> lv_obj_t* {
            screen = std::make_unique<zerosky::SkyScreen>(view_model, assets);
            return screen->root();
        },
        [&]() { screen.reset(); });
}
