cmake_minimum_required(VERSION 3.22)
project(GRIMOIRE VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(GRIMOIRE
    COMPANY_NAME                 "Underwater Palace"
    PLUGIN_MANUFACTURER_CODE     Uwpl
    PLUGIN_CODE                  Grmr
    FORMATS                      VST3 Standalone
    PRODUCT_NAME                 "GRIMOIRE"
    IS_SYNTH                     FALSE
    NEEDS_MIDI_INPUT             FALSE
    NEEDS_MIDI_OUTPUT            FALSE
    IS_MIDI_EFFECT               FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS  TRUE
    COPY_PLUGIN_AFTER_BUILD      FALSE
    NEEDS_WEBVIEW2               TRUE
    VST3_CATEGORIES              Fx Modulation)

# L'interface 3D (la même page que la preview) est embarquée dans le plugin
juce_add_binary_data(GrimoireUI SOURCES ui/index.html)

target_sources(GRIMOIRE PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(GRIMOIRE PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(GRIMOIRE
    PRIVATE
        GrimoireUI
        juce::juce_audio_utils
        juce::juce_gui_extra
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
