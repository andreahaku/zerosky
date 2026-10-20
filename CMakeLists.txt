#
# SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
#
# SPDX-License-Identifier: MIT
#

cmake_minimum_required(VERSION 3.31)

project(zerosky VERSION 0.1.0 LANGUAGES C CXX)

# Public source repository: the package homepage.
set(ZEROSKY_REPO_URL "https://github.com/andreahaku/zerosky")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# CMake configure options
# USE_DESKTOP: whether to build the desktop emulator (LVGL SDL simulator).
# For the device build, configure with -DUSE_DESKTOP=OFF (the cp0-cross preset).
set(USE_DESKTOP ON CACHE BOOL "Build the desktop emulator target with LVGL SDL support")
set(APP_KEY_INPUT_DEVICE "" CACHE STRING "Optional Linux evdev keyboard/keypad device path, e.g. /dev/input/event0")
set(APP_FRAMEBUFFER_DEVICE "/dev/fb0" CACHE STRING "Linux framebuffer device path used by embedded builds")
set(APP_DRM_DEVICE "/dev/dri/card0" CACHE STRING "Linux DRM device path used when APP_USE_DRM=ON")
set(APP_DRM_CONNECTOR_ID "-1" CACHE STRING "DRM connector id used when APP_USE_DRM=ON; -1 auto-selects")
set(APP_USE_DRM OFF CACHE BOOL "Enable LVGL Linux DRM/KMS backend for embedded Linux builds")
set(CM0_ALLOW_FETCH_DEPS ON CACHE BOOL "Allow downloading missing CM0 BSP/dependencies during cross builds")
# Export compile commands for IDE IntelliSense.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile_commands.json" FORCE)

# Shared assets root (fonts) the toolkit's AssetManager resolves at runtime.
set(ZEROSKY_ASSETS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/assets")
set(TOOLKIT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/radio_toolkit")

include(FetchContent)

# ====================================================
# lvgl - main ui framework
# ====================================================
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS    OFF CACHE BOOL "" FORCE)
set(LV_BUILD_CONF_PATH "${PROJECT_SOURCE_DIR}/third_party/radio_toolkit/src/config/lv_conf.h" CACHE PATH "" FORCE)

FetchContent_Declare(
    lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG        v9.5.0
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(lvgl)
if(NOT lvgl_POPULATED)
    if(POLICY CMP0169)
        cmake_policy(PUSH)
        cmake_policy(SET CMP0169 OLD)
    endif()
    FetchContent_Populate(lvgl)
    if(POLICY CMP0169)
        cmake_policy(POP)
    endif()
endif()
add_subdirectory("${lvgl_SOURCE_DIR}" "${lvgl_BINARY_DIR}" EXCLUDE_FROM_ALL)
set(LVGL_SOURCE_DIR "${lvgl_SOURCE_DIR}")
get_filename_component(LVGL_INCLUDE_PARENT "${LVGL_SOURCE_DIR}" DIRECTORY)

# ====================================================
# other misc dependencies
# ====================================================
if(USE_DESKTOP)
    message(STATUS "Configuring DESKTOP build")

    find_package(PNG REQUIRED)
    find_package(JPEG REQUIRED)

    if(TARGET libjpeg-turbo::libjpeg-turbo)
        set(JPEG_TARGET libjpeg-turbo::libjpeg-turbo)
    elseif(TARGET JPEG::JPEG)
        set(JPEG_TARGET JPEG::JPEG)
    else()
        message(FATAL_ERROR "No JPEG provider found")
    endif()

    find_package(Freetype REQUIRED)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(fmt CONFIG REQUIRED)
    find_package(ZLIB REQUIRED)
    find_package(Threads REQUIRED)

    set(LVGL_DESKTOP_DEPS
        ZLIB::ZLIB
        PNG::PNG
        ${JPEG_TARGET}
        Freetype::Freetype
        SDL2::SDL2
    )

    foreach(dep IN LISTS LVGL_DESKTOP_DEPS)
        if(TARGET ${dep})
            target_link_libraries(lvgl PUBLIC ${dep})
        endif()
    endforeach()

    if(SDL2_FOUND AND NOT TARGET SDL2::SDL2)
        target_include_directories(lvgl PUBLIC ${SDL2_INCLUDE_DIRS})
        target_link_libraries(lvgl PUBLIC ${SDL2_LIBRARIES})
    endif()

    target_compile_definitions(lvgl PUBLIC USE_DESKTOP=1)

    if(MSVC)
        target_compile_options(lvgl PUBLIC /utf-8)
    endif()

# Cross builds: search individual dependencies from the BSP sysroot.
else()
    message(STATUS "Configuring CM0/aarch64 Linux build")

    if(NOT CM0_SDK_ROOT OR NOT EXISTS "${CM0_SDK_ROOT}")
        message(FATAL_ERROR
            "CM0 SDK sysroot not found or invalid.\n"
            "The cp0 toolchain should download the BSP before project() runs, "
            "or configure with -DCM0_SDK_ROOT=<path_to_sdk_sysroot>."
        )
    endif()

    if(CMAKE_SYSROOT)
        set(CM0_SYSROOT "${CMAKE_SYSROOT}")
    else()
        set(CM0_SYSROOT "${CM0_SDK_ROOT}")
    endif()
    if(NOT CM0_MULTIARCH)
        set(CM0_MULTIARCH "aarch64-linux-gnu")
    endif()
    message(STATUS "Using CM0 SDK root: ${CM0_SDK_ROOT}")
    message(STATUS "Using CM0 dependency root: ${CM0_SYSROOT}")

    set(CM0_INCLUDE_HINTS
        "${CM0_SYSROOT}/usr/include/${CM0_MULTIARCH}"
        "${CM0_SYSROOT}/usr/include"
        "${CM0_SYSROOT}/usr/local/include"
        "${CM0_SYSROOT}/include/${CM0_MULTIARCH}"
        "${CM0_SYSROOT}/include"
    )
    set(CM0_LIBRARY_HINTS
        "${CM0_SYSROOT}/usr/lib/${CM0_MULTIARCH}"
        "${CM0_SYSROOT}/usr/lib"
        "${CM0_SYSROOT}/usr/local/lib/${CM0_MULTIARCH}"
        "${CM0_SYSROOT}/usr/local/lib"
        "${CM0_SYSROOT}/lib/${CM0_MULTIARCH}"
        "${CM0_SYSROOT}/lib"
    )

    function(cm0_find_header out_var header_name)
        find_path(${out_var}
            NAMES ${header_name}
            HINTS ${ARGN}
            NO_DEFAULT_PATH
        )
        set(${out_var} "${${out_var}}" PARENT_SCOPE)
    endfunction()

    function(cm0_find_library out_var)
        find_library(${out_var}
            NAMES ${ARGN}
            HINTS ${CM0_LIBRARY_HINTS}
            NO_DEFAULT_PATH
        )
        set(${out_var} "${${out_var}}" PARENT_SCOPE)
    endfunction()

    function(cm0_add_imported_library target_name library_path include_dirs)
        if(NOT TARGET ${target_name})
            add_library(${target_name} UNKNOWN IMPORTED)
            set_target_properties(${target_name} PROPERTIES
                IMPORTED_LOCATION "${library_path}"
                INTERFACE_INCLUDE_DIRECTORIES "${include_dirs}"
            )
        endif()
    endfunction()

    if(APP_USE_DRM)
        cm0_find_header(DRM_XF86_INCLUDE_DIR xf86drm.h
            "${CM0_SYSROOT}/usr/include"
            "${CM0_SYSROOT}/usr/include/libdrm"
        )
        cm0_find_header(DRM_XF86_MODE_INCLUDE_DIR xf86drmMode.h
            "${CM0_SYSROOT}/usr/include"
            "${CM0_SYSROOT}/usr/include/libdrm"
        )
        cm0_find_header(DRM_MODE_INCLUDE_DIR drm.h
            "${CM0_SYSROOT}/usr/include/drm"
            "${CM0_SYSROOT}/usr/include/libdrm"
            "${CM0_SYSROOT}/usr/include"
        )
        cm0_find_library(DRM_LIBRARY drm drm.so.2)

        if(NOT DRM_XF86_INCLUDE_DIR
           OR NOT DRM_XF86_MODE_INCLUDE_DIR
           OR NOT DRM_MODE_INCLUDE_DIR
           OR NOT DRM_LIBRARY)
            message(FATAL_ERROR
                "APP_USE_DRM=ON but libdrm headers/library were not found in BSP sysroot.\n"
                "Provide them in ${CM0_SYSROOT} or disable with -DAPP_USE_DRM=OFF."
            )
        endif()
        cm0_add_imported_library(CM0::DRM "${DRM_LIBRARY}"
            "${DRM_XF86_INCLUDE_DIR};${DRM_XF86_MODE_INCLUDE_DIR};${DRM_MODE_INCLUDE_DIR}"
        )
        message(STATUS "CM0 DRM display support enabled")
    endif()

    cm0_find_header(FREETYPE_INCLUDE_DIR ft2build.h
        "${CM0_SYSROOT}/usr/include/freetype2"
        "${CM0_SYSROOT}/usr/include"
    )
    cm0_find_library(FREETYPE_LIBRARY freetype)
    if(NOT FREETYPE_INCLUDE_DIR OR NOT FREETYPE_LIBRARY)
        message(FATAL_ERROR "Missing Freetype header/library in BSP sysroot: ${CM0_SYSROOT}")
    endif()
    cm0_add_imported_library(CM0::Freetype "${FREETYPE_LIBRARY}" "${FREETYPE_INCLUDE_DIR}")

    cm0_find_header(PNG_INCLUDE_DIR png.h
        "${CM0_SYSROOT}/usr/include/libpng16"
        "${CM0_SYSROOT}/usr/include/libpng"
        "${CM0_SYSROOT}/usr/include"
    )
    cm0_find_library(PNG_LIBRARY png16 png)
    if(NOT PNG_INCLUDE_DIR OR NOT PNG_LIBRARY)
        message(FATAL_ERROR "Missing PNG header/library in BSP sysroot: ${CM0_SYSROOT}")
    endif()
    cm0_add_imported_library(CM0::PNG "${PNG_LIBRARY}" "${PNG_INCLUDE_DIR}")

    cm0_find_header(JPEG_INCLUDE_DIR jpeglib.h
        "${CM0_SYSROOT}/usr/include"
    )
    cm0_find_library(JPEG_LIBRARY jpeg)
    if(NOT JPEG_INCLUDE_DIR OR NOT JPEG_LIBRARY)
        message(FATAL_ERROR "Missing JPEG header/library in BSP sysroot: ${CM0_SYSROOT}")
    endif()
    cm0_add_imported_library(CM0::JPEG "${JPEG_LIBRARY}" "${JPEG_INCLUDE_DIR}")

    cm0_find_header(ZLIB_INCLUDE_DIR zlib.h
        "${CM0_SYSROOT}/usr/include"
    )
    cm0_find_library(ZLIB_LIBRARY z)
    if(NOT ZLIB_INCLUDE_DIR OR NOT ZLIB_LIBRARY)
        message(FATAL_ERROR "Missing ZLIB header/library in BSP sysroot: ${CM0_SYSROOT}")
    endif()
    cm0_add_imported_library(ZLIB::ZLIB "${ZLIB_LIBRARY}" "${ZLIB_INCLUDE_DIR}")

    set(CM0_MULTIARCH_INCLUDE_DIRS)
    foreach(CM0_INCLUDE_DIR IN ITEMS
            "${CM0_SYSROOT}/usr/include/${CM0_MULTIARCH}"
            "${CM0_SYSROOT}/include/${CM0_MULTIARCH}")
        if(EXISTS "${CM0_INCLUDE_DIR}/bits/timesize.h" OR EXISTS "${CM0_INCLUDE_DIR}/bits/wordsize.h")
            list(APPEND CM0_MULTIARCH_INCLUDE_DIRS "${CM0_INCLUDE_DIR}")
            message(STATUS "Found multiarch include: ${CM0_INCLUDE_DIR}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES CM0_MULTIARCH_INCLUDE_DIRS)

    cm0_find_header(FMT_INCLUDE_DIR fmt/format.h
        "${CM0_SYSROOT}/usr/include"
    )
    cm0_find_library(FMT_LIBRARY fmt libfmt.so.10 libfmt.so.10.1.0)
    if(FMT_INCLUDE_DIR AND FMT_LIBRARY)
        cm0_add_imported_library(fmt::fmt "${FMT_LIBRARY}" "${FMT_INCLUDE_DIR}")
    elseif(CM0_ALLOW_FETCH_DEPS)
        message(STATUS "Missing fmt in BSP sysroot, fetching from GitHub because CM0_ALLOW_FETCH_DEPS=ON")
        FetchContent_Declare(
            fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt.git
            GIT_TAG        10.1.1
            GIT_SHALLOW    TRUE
        )
        set(FMT_INSTALL OFF CACHE BOOL "Disable fmt install rules for app packages" FORCE)
        set(FMT_TEST OFF CACHE BOOL "Disable fmt tests" FORCE)
        set(FMT_DOC OFF CACHE BOOL "Disable fmt docs" FORCE)
        FetchContent_MakeAvailable(fmt)
        if(TARGET fmt)
            target_include_directories(fmt SYSTEM BEFORE PRIVATE ${CM0_MULTIARCH_INCLUDE_DIRS})
        endif()
        if(TARGET fmt AND NOT TARGET fmt::fmt)
            add_library(fmt::fmt ALIAS fmt)
        endif()
    else()
        message(FATAL_ERROR
            "Missing fmt header/library in BSP sysroot: ${CM0_SYSROOT}\n"
            "Install/copy fmt into the BSP, or reconfigure with -DCM0_ALLOW_FETCH_DEPS=ON."
        )
    endif()

    set_target_properties(CM0::PNG PROPERTIES INTERFACE_LINK_LIBRARIES ZLIB::ZLIB)
    set_target_properties(CM0::Freetype PROPERTIES INTERFACE_LINK_LIBRARIES "CM0::PNG;ZLIB::ZLIB")

    cm0_find_library(PTHREAD_LIBRARY pthread libpthread.so.0 libpthread.a)
    cm0_find_library(M_LIBRARY m libm.so libm.so.6 libm.a)
    if(NOT PTHREAD_LIBRARY)
        message(FATAL_ERROR "Missing pthread library in BSP sysroot: ${CM0_SYSROOT}")
    endif()
    if(NOT M_LIBRARY)
        message(FATAL_ERROR "Missing math library in BSP sysroot: ${CM0_SYSROOT}")
    endif()

    set(PLATFORM_INCLUDE_DIR ${CM0_MULTIARCH_INCLUDE_DIRS})
    set(PLATFORM_LIBS ${PTHREAD_LIBRARY} ${M_LIBRARY})

    set(LVGL_INCLUDE_DIR
        ${FREETYPE_INCLUDE_DIR}
        ${PNG_INCLUDE_DIR}
        ${JPEG_INCLUDE_DIR}
        ${ZLIB_INCLUDE_DIR}
    )
    set(LVGL_LIBS
        CM0::Freetype
        CM0::PNG
        CM0::JPEG
        ZLIB::ZLIB
    )
    if(APP_USE_DRM)
        list(APPEND LVGL_INCLUDE_DIR
            ${DRM_XF86_INCLUDE_DIR}
            ${DRM_XF86_MODE_INCLUDE_DIR}
            ${DRM_MODE_INCLUDE_DIR}
        )
        list(APPEND LVGL_LIBS CM0::DRM)
    endif()

    list(REMOVE_DUPLICATES PLATFORM_INCLUDE_DIR)
    list(REMOVE_DUPLICATES PLATFORM_LIBS)
    list(REMOVE_DUPLICATES LVGL_INCLUDE_DIR)
    list(REMOVE_DUPLICATES LVGL_LIBS)

    target_compile_definitions(lvgl
        PUBLIC
            USE_DESKTOP=0
            APP_USE_DRM=$<BOOL:${APP_USE_DRM}>
    )

    # fix: issue of compiling lvgl on gcc >= 15.0
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        if (CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 15.0)
            target_compile_options(lvgl PRIVATE -Wno-stringop-overflow)
        endif()
    endif()

    target_include_directories(lvgl
        PUBLIC
            ${LVGL_INCLUDE_DIR}
            ${PLATFORM_INCLUDE_DIR}
    )

    target_link_libraries(lvgl
        PUBLIC
            ${LVGL_LIBS}
            ${PLATFORM_LIBS}
    )

endif()

# ====================================================
# logger (fmt-based)
# ====================================================
file(GLOB_RECURSE LOGGER_SOURCES CONFIGURE_DEPENDS
    "${TOOLKIT_DIR}/src/logger/*.c"
    "${TOOLKIT_DIR}/src/logger/*.cpp"
)

add_library(logger STATIC ${LOGGER_SOURCES})
target_include_directories(logger PUBLIC
    ${TOOLKIT_DIR}/src/logger
    ${PLATFORM_INCLUDE_DIR}
)
target_link_libraries(logger
    PUBLIC
        fmt::fmt
    PRIVATE
        ${PLATFORM_LIBS}
)

# ====================================================
# radio_toolkit: the app shell vendored from ZeroRadio (third_party/radio_toolkit/VENDORED.md)
# ====================================================
file(GLOB_RECURSE TOOLKIT_SOURCES CONFIGURE_DEPENDS
    "${TOOLKIT_DIR}/src/app/*.cpp"
    "${TOOLKIT_DIR}/src/reactive/*.cpp"
    "${TOOLKIT_DIR}/src/platform/*.c"
    "${TOOLKIT_DIR}/src/platform/*.cpp"
    "${TOOLKIT_DIR}/src/view/*.cpp"
    "${TOOLKIT_DIR}/src/geo/*.cpp"
)

find_package(Threads REQUIRED)

add_library(radio_toolkit STATIC ${TOOLKIT_SOURCES})

target_include_directories(radio_toolkit
    PUBLIC
        ${LVGL_INCLUDE_DIR}
        ${PLATFORM_INCLUDE_DIR}
        ${TOOLKIT_DIR}/src
        ${TOOLKIT_DIR}/src/app
        ${TOOLKIT_DIR}/src/config
        ${TOOLKIT_DIR}/src/logger
        ${TOOLKIT_DIR}/src/reactive
        ${TOOLKIT_DIR}/src/platform
        ${TOOLKIT_DIR}/src/view
        ${TOOLKIT_DIR}/src/view/widgets
        ${TOOLKIT_DIR}/src/view/screens
        ${TOOLKIT_DIR}/src/geo
)

# The device build resolves assets and docs from the installed package only, so
# no build-machine path ends up in the shipped binary.
if(CMAKE_CROSSCOMPILING)
    set(ZEROSKY_RUNTIME_ROOT "/usr/share/zerosky")
else()
    set(ZEROSKY_RUNTIME_ROOT "${ZEROSKY_ASSETS_ROOT}")
endif()
target_compile_definitions(radio_toolkit
    PUBLIC
        APP_NAME="zerosky"
        APP_DISPLAY_NAME="ZeroSky"
        APP_VERSION="${PROJECT_VERSION}"
        ZERORADIO_DOCS_ROOT="$<IF:$<BOOL:${CMAKE_CROSSCOMPILING}>,/usr/share/zerosky,${CMAKE_CURRENT_SOURCE_DIR}>"
        APP_CMAKE_ASSETS_ROOT="${ZEROSKY_RUNTIME_ROOT}"
        APP_SOURCE_ASSETS_ROOT="${ZEROSKY_RUNTIME_ROOT}"
        APP_FRAMEBUFFER_DEVICE="${APP_FRAMEBUFFER_DEVICE}"
        APP_DRM_DEVICE="${APP_DRM_DEVICE}"
        APP_DRM_CONNECTOR_ID=${APP_DRM_CONNECTOR_ID}
        APP_KEY_INPUT_DEVICE="${APP_KEY_INPUT_DEVICE}"
)

target_link_libraries(radio_toolkit
    PUBLIC
        lvgl
        logger
        Threads::Threads
)

# ====================================================
# tests (host only)
# ====================================================
enable_testing()

if(USE_DESKTOP)
    add_executable(location_test ${TOOLKIT_DIR}/test/location_test.cpp)
    target_link_libraries(location_test PRIVATE radio_toolkit)
    target_compile_definitions(location_test PRIVATE
        CITIES_TSV="${ZEROSKY_ASSETS_ROOT}/geodata/cities.tsv")
    add_test(NAME location_test COMMAND location_test)

    add_executable(i18n_test ${TOOLKIT_DIR}/test/i18n_test.cpp)
    target_link_libraries(i18n_test PRIVATE radio_toolkit)
    add_test(NAME i18n_test COMMAND i18n_test)
endif()

# ====================================================
# sensors: BMI270 and BMM150 over IIO, plus the desktop simulator (no LVGL)
# ====================================================
add_library(zerosky_sensors STATIC src/sensors/sensor_source.cpp)
target_include_directories(zerosky_sensors PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/sensors)

if(USE_DESKTOP)
    add_executable(sensor_source_test src/sensors/sensor_source_test.cpp)
    target_link_libraries(sensor_source_test PRIVATE zerosky_sensors)
    add_test(NAME sensor_source_test COMMAND sensor_source_test)
endif()

# ====================================================
# wmm: NOAA World Magnetic Model 2025 (third_party/wmm, public domain)
# ====================================================
add_library(wmm STATIC third_party/wmm/GeomagnetismLibrary.c)
target_include_directories(wmm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/wmm)
target_link_libraries(wmm PUBLIC ${PLATFORM_LIBS})
if(NOT WIN32)
    target_link_libraries(wmm PUBLIC m)
endif()

# ====================================================
# orientation: Mahony filter, magnetometer calibration, disturbed-compass gate
# ====================================================
add_library(zerosky_orientation STATIC
    src/orientation/mahony.cpp
    src/orientation/calibration.cpp
    src/orientation/magnetic_gate.cpp
    src/orientation/attitude_service.cpp
    src/orientation/geomagnetic.cpp
    src/orientation/calibration_fit.cpp
    src/orientation/camera_from_attitude.cpp
    src/orientation/frame_mapping_store.cpp
)
target_include_directories(zerosky_orientation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/orientation)
target_link_libraries(zerosky_orientation PUBLIC zerosky_sensors zerosky_sky wmm Threads::Threads)

if(USE_DESKTOP)
    add_executable(orientation_test src/orientation/orientation_test.cpp)
    target_link_libraries(orientation_test PRIVATE zerosky_orientation)
    add_test(NAME orientation_test COMMAND orientation_test)

    add_executable(attitude_service_test src/orientation/attitude_service_test.cpp)
    target_link_libraries(attitude_service_test PRIVATE zerosky_orientation)
    add_test(NAME attitude_service_test COMMAND attitude_service_test)

    add_executable(geomagnetic_test src/orientation/geomagnetic_test.cpp)
    target_link_libraries(geomagnetic_test PRIVATE zerosky_orientation)
    add_test(NAME geomagnetic_test
             COMMAND geomagnetic_test ${CMAKE_CURRENT_SOURCE_DIR}/third_party/wmm/WMM.COF)

    add_executable(calibration_fit_test src/orientation/calibration_fit_test.cpp)
    target_link_libraries(calibration_fit_test PRIVATE zerosky_orientation)
    add_test(NAME calibration_fit_test COMMAND calibration_fit_test)

    add_executable(camera_from_attitude_test src/orientation/camera_from_attitude_test.cpp)
    target_link_libraries(camera_from_attitude_test PRIVATE zerosky_orientation)
    add_test(NAME camera_from_attitude_test COMMAND camera_from_attitude_test)

    add_executable(frame_mapping_store_test src/orientation/frame_mapping_store_test.cpp)
    target_link_libraries(frame_mapping_store_test PRIVATE zerosky_orientation)
    add_test(NAME frame_mapping_store_test COMMAND frame_mapping_store_test)
endif()

# ====================================================
# sky: Astronomy Engine (third_party/astronomy, MIT) for the Sun, Moon and planets
# ====================================================
add_library(astronomy STATIC third_party/astronomy/astronomy.c)
target_include_directories(astronomy PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/astronomy)
target_link_libraries(astronomy PUBLIC ${PLATFORM_LIBS})
if(NOT WIN32)
    target_link_libraries(astronomy PUBLIC m)
endif()

add_library(zerosky_sky STATIC
    src/sky/sky_engine.cpp
    src/sky/star_catalog.cpp
    src/sky/projection.cpp
    src/sky/constellations.cpp
    src/sky/twilight.cpp
    src/sky/deep_sky.cpp
)
target_include_directories(zerosky_sky PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/sky)
target_link_libraries(zerosky_sky PUBLIC astronomy)

if(USE_DESKTOP)
    add_executable(sky_engine_test src/sky/sky_engine_test.cpp)
    target_link_libraries(sky_engine_test PRIVATE zerosky_sky)
    add_test(NAME sky_engine_test COMMAND sky_engine_test)

    add_executable(star_catalog_test src/sky/star_catalog_test.cpp)
    target_link_libraries(star_catalog_test PRIVATE zerosky_sky)
    target_compile_definitions(star_catalog_test PRIVATE
        STARS_TSV="${ZEROSKY_ASSETS_ROOT}/sky/stars.tsv")
    add_test(NAME star_catalog_test COMMAND star_catalog_test)

    add_executable(projection_test src/sky/projection_test.cpp)
    target_link_libraries(projection_test PRIVATE zerosky_sky)
    add_test(NAME projection_test COMMAND projection_test)

    add_executable(constellations_test src/sky/constellations_test.cpp)
    target_link_libraries(constellations_test PRIVATE zerosky_sky)
    target_compile_definitions(constellations_test PRIVATE
        LINES_TSV="${ZEROSKY_ASSETS_ROOT}/sky/constellations.tsv"
        STARS_TSV="${ZEROSKY_ASSETS_ROOT}/sky/stars.tsv")
    add_test(NAME constellations_test COMMAND constellations_test)

    add_executable(twilight_test src/sky/twilight_test.cpp)
    target_link_libraries(twilight_test PRIVATE zerosky_sky)
    add_test(NAME twilight_test COMMAND twilight_test)

    add_executable(deep_sky_test src/sky/deep_sky_test.cpp)
    target_link_libraries(deep_sky_test PRIVATE zerosky_sky)
    target_compile_definitions(deep_sky_test PRIVATE
        DEEP_SKY_TSV="${ZEROSKY_ASSETS_ROOT}/sky/deep_sky.tsv")
    add_test(NAME deep_sky_test COMMAND deep_sky_test)
endif()

# ====================================================
# audio: the search beep (SDL on the desktop, ALSA on the device, else silent)
# ====================================================
add_library(zerosky_audio STATIC src/audio/beeper.cpp)
target_include_directories(zerosky_audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/audio)
target_link_libraries(zerosky_audio PUBLIC Threads::Threads)
if(USE_DESKTOP)
    target_compile_definitions(zerosky_audio PRIVATE ZEROSKY_BEEPER_SDL=1)
    target_link_libraries(zerosky_audio PRIVATE SDL2::SDL2)
else()
    cm0_find_library(ASOUND_LIBRARY asound)
    if(ASOUND_LIBRARY)
        target_compile_definitions(zerosky_audio PRIVATE ZEROSKY_BEEPER_ALSA=1)
        target_link_libraries(zerosky_audio PRIVATE ${ASOUND_LIBRARY})
    else()
        message(STATUS "libasound not found: the search beep is silent")
    endif()
endif()

if(USE_DESKTOP)
    add_executable(beeper_test src/audio/beeper_test.cpp)
    target_link_libraries(beeper_test PRIVATE zerosky_audio)
    add_test(NAME beeper_test COMMAND beeper_test)
endif()

# ====================================================
# zerosky_probe: command-line sensor and calibration probe (docs/probe.md)
# ====================================================
add_executable(zerosky_probe src/tools/probe_main.cpp)
target_link_libraries(zerosky_probe PRIVATE zerosky_orientation)

# ====================================================
# the app
# ====================================================
add_executable(zerosky
    src/main.cpp
    src/viewmodel/sky_viewmodel.cpp
    src/viewmodel/attitude_controller.cpp
    src/view/sky_screen.cpp
    src/view/search_dialog.cpp
    src/view/search_filter.cpp
)
target_include_directories(zerosky PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/viewmodel
    ${CMAKE_CURRENT_SOURCE_DIR}/src/view
)
target_link_libraries(zerosky PRIVATE radio_toolkit zerosky_sensors zerosky_orientation zerosky_sky zerosky_audio)

if(USE_DESKTOP)
    add_executable(search_filter_test src/view/search_dialog_test.cpp src/view/search_filter.cpp)
    target_include_directories(search_filter_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/view)
    add_test(NAME search_filter_test COMMAND search_filter_test)
endif()

# ====================================================
# Packaging: the `zerosky` Debian package (aarch64 cross build only)
# ====================================================
if(CMAKE_CROSSCOMPILING)
    set(ZEROSKY_PKG_DATADIR "share/zerosky")
    set(ZEROSKY_APPLAUNCH   "share/APPLaunch")

    install(TARGETS zerosky RUNTIME DESTINATION "${ZEROSKY_APPLAUNCH}/bin")
    install(DIRECTORY "${ZEROSKY_ASSETS_ROOT}/fonts" "${ZEROSKY_ASSETS_ROOT}/geodata"
                      "${ZEROSKY_ASSETS_ROOT}/sky"
            DESTINATION "${ZEROSKY_PKG_DATADIR}")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/wmm/WMM.COF"
            DESTINATION "${ZEROSKY_PKG_DATADIR}/wmm")
    # In-app help and credits (toolkit::read_doc), same relative paths as in the repository.
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/CREDITS.md" DESTINATION "${ZEROSKY_PKG_DATADIR}")
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/docs/help" DESTINATION "${ZEROSKY_PKG_DATADIR}/docs")
    # The field probe for the axes and the outdoor calibration (docs/probe.md).
    install(TARGETS zerosky_probe RUNTIME DESTINATION "${ZEROSKY_PKG_DATADIR}/bin")

    set(APP_DISPLAY_NAME "ZeroSky")
    set(APP_PACKAGE_DESCRIPTION "Pocket AR planetarium for CardputerZero")
    configure_file(cmake/templates/app.desktop.in "${CMAKE_CURRENT_BINARY_DIR}/zerosky.desktop" @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/zerosky.desktop"
            DESTINATION "${ZEROSKY_APPLAUNCH}/applications")
    install(FILES "${ZEROSKY_ASSETS_ROOT}/images/zerosky.png"
            DESTINATION "${ZEROSKY_APPLAUNCH}/share/images")

    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGE_NAME "zerosky")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_CONTACT "Andrea Salvatore <andreahaku@gmail.com>")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${APP_PACKAGE_DESCRIPTION}")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${ZEROSKY_REPO_URL}")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
    set(CPACK_DEBIAN_PACKAGE_SECTION "science")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
    include(CPack)
endif()
