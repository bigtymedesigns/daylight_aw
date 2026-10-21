cmake_minimum_required(VERSION 3.22)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary on macOS")

project(Daylight_aw VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(DAYLIGHT_BUILD_TESTS "Build the test program" ON)
option(DAYLIGHT_MINIMAL_X11 "Linux only: build without optional X11 extensions (Xrandr/Xinerama/Xcursor headers)" OFF)
option(DAYLIGHT_SANITIZERS "Build with AddressSanitizer + UndefinedBehaviorSanitizer" OFF)
option(DAYLIGHT_TSAN "Build with ThreadSanitizer" OFF)

# ---- JUCE (pinned version; upgrades are deliberate, tested steps) ---------------------------
set(DAYLIGHT_JUCE_VERSION "8.0.15")
if(DEFINED DAYLIGHT_JUCE_DIR)
    add_subdirectory(${DAYLIGHT_JUCE_DIR} JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG ${DAYLIGHT_JUCE_VERSION}
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

if(DAYLIGHT_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()
if(DAYLIGHT_TSAN)
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
    add_link_options(-fsanitize=thread)
endif()

# ---- Airwindows effects (generated list from manifest/effects.json) -------------------------
include(cmake/Effects.cmake)

add_library(daylight_airwindows STATIC
    third_party/airwin_consolidated/airwin_consolidated_base.cpp
    ${DAYLIGHT_EFFECT_SOURCES})
target_include_directories(daylight_airwindows SYSTEM PUBLIC third_party/airwin_consolidated)
# The original Airwindows code is compiled as-is (no changes, no warnings shown).
# The original code uses M_PI etc. MSVC only provides those with _USE_MATH_DEFINES.
target_compile_definitions(daylight_airwindows PRIVATE _USE_MATH_DEFINES)
if(MSVC)
    target_compile_options(daylight_airwindows PRIVATE /w /bigobj)
else()
    target_compile_options(daylight_airwindows PRIVATE -w)
endif()
if(DAYLIGHT_SANITIZERS)
    # The original code relies on wrapping integer maths in its noise generators. That is upstream
    # behaviour we don't change, so UBSan only checks our own code (ASan still covers everything).
    target_compile_options(daylight_airwindows PRIVATE -fno-sanitize=undefined)
endif()

# ---- Embedded assets (built-in themes, fonts, layout files) ---------------------------------
# Copied under unique names first so every resource has a clear name in BinaryData.
set(ASSET_DIR ${CMAKE_CURRENT_BINARY_DIR}/assets)
set(DAYLIGHT_ASSETS)
macro(daylight_asset src name)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${src} ${ASSET_DIR}/${name} COPYONLY)
    list(APPEND DAYLIGHT_ASSETS ${ASSET_DIR}/${name})
endmacro()

daylight_asset(themes/light/theme.json   light_theme.json)
daylight_asset(themes/light/paper.png    light_paper.png)
daylight_asset(themes/light/figure.png   light_figure.png)
daylight_asset(themes/dark/theme.json    dark_theme.json)
daylight_asset(themes/dark/paper.png     dark_paper.png)
daylight_asset(themes/dark/figure.png    dark_figure.png)
daylight_asset(themes/README.md          theme_readme.md)
daylight_asset(resources/fonts/shantell_medium.ttf    shantell_medium.ttf)
daylight_asset(resources/fonts/shantell_semibold.ttf  shantell_semibold.ttf)
daylight_asset(resources/fonts/shantell_extrabold.ttf shantell_extrabold.ttf)
foreach(layout ${DAYLIGHT_LAYOUT_FILES})
    get_filename_component(layout_name ${layout} NAME)
    daylight_asset(${layout} ${layout_name})
endforeach()

juce_add_binary_data(daylight_assets HEADER_NAME BinaryData.h NAMESPACE BinaryData SOURCES ${DAYLIGHT_ASSETS})

# ---- Shared source list ------------------------------------------------------------------------
set(DAYLIGHT_SOURCES
    src/PluginProcessor.cpp
    src/dsp/EffectHost.cpp
    src/dsp/EffectRegistry.cpp
    src/generated/EffectList.gen.cpp
    src/gui/Components.cpp
    src/gui/Fonts.cpp
    src/gui/Layout.cpp
    src/gui/Panels.cpp
    src/gui/PluginEditor.cpp
    src/gui/Settings.cpp
    src/gui/Theme.cpp)

set(DAYLIGHT_DEFINITIONS
    DAYLIGHT_VERSION_STRING="${PROJECT_VERSION}"
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_MODAL_LOOPS_PERMITTED=0)
if(DAYLIGHT_MINIMAL_X11)
    list(APPEND DAYLIGHT_DEFINITIONS JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0 JUCE_USE_XSHM=1)
endif()

if(MSVC)
    add_compile_options(/utf-8)   # sources are UTF-8
endif()

set(DAYLIGHT_LIBS
    daylight_assets
    daylight_airwindows
    juce::juce_audio_processors
    juce::juce_gui_basics)

# ---- The plugin ---------------------------------------------------------------------------------
set(DAYLIGHT_FORMATS VST3)
if(APPLE)
    list(APPEND DAYLIGHT_FORMATS AU)
endif()

juce_add_plugin(Daylight_aw
    PRODUCT_NAME "Daylight_aw"
    COMPANY_NAME "big_tyme_designs"
    BUNDLE_ID "com.bigtymedesigns.daylightaw"
    PLUGIN_MANUFACTURER_CODE Btyd
    PLUGIN_CODE Dlaw
    FORMATS ${DAYLIGHT_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES Fx
    AU_MAIN_TYPE kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(Daylight_aw PRIVATE ${DAYLIGHT_SOURCES})
target_include_directories(Daylight_aw PRIVATE src)
target_compile_definitions(Daylight_aw PUBLIC ${DAYLIGHT_DEFINITIONS})
target_link_libraries(Daylight_aw
    PRIVATE ${DAYLIGHT_LIBS}
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# ---- Tests --------------------------------------------------------------------------------------
if(DAYLIGHT_BUILD_TESTS)
    juce_add_console_app(DaylightTests PRODUCT_NAME "DaylightTests")
    target_sources(DaylightTests PRIVATE tests/TestMain.cpp ${DAYLIGHT_SOURCES})
    target_include_directories(DaylightTests PRIVATE src)
    target_compile_definitions(DaylightTests PRIVATE ${DAYLIGHT_DEFINITIONS} JUCE_STANDALONE_APPLICATION=1)
    target_link_libraries(DaylightTests
        PRIVATE ${DAYLIGHT_LIBS}
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
