cmake_minimum_required(VERSION 3.22)

project(KickForge VERSION 1.1.0 LANGUAGES CXX)

include(CTest)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(KickForge
    COMPANY_NAME "KickForge"
    BUNDLE_ID "com.kickforge.instrument"
    PLUGIN_MANUFACTURER_CODE Kfge
    PLUGIN_CODE Kfki
    FORMATS VST3
    PRODUCT_NAME "KickForge"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(KickForge PRIVATE
    Source/KickSynth.cpp
    Source/PluginEditor.cpp
    Source/PluginProcessor.cpp
)
target_compile_features(KickForge PRIVATE cxx_std_17)
target_compile_definitions(KickForge PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(KickForge PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

if(BUILD_TESTING)
    add_executable(KickForgeDspTests
        Source/KickSynth.cpp
        tests/KickForgeDspTests.cpp
    )
    target_include_directories(KickForgeDspTests PRIVATE Source)
    target_compile_features(KickForgeDspTests PRIVATE cxx_std_17)
    add_test(NAME KickForgeDspTests COMMAND KickForgeDspTests)
endif()
