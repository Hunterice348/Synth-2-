cmake_minimum_required(VERSION 3.15)
project(HUNTERICE_ABYSS VERSION 0.1.0)

# Загружаем JUCE из официального репозитория
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# Создаём VST3 target
juce_add_plugin(HUNTERICE_ABYSS
    COMPANY_NAME              "HUNTERICE"
    IS_SYNTH                  TRUE
    NEEDS_MIDI_INPUT          TRUE
    NEEDS_MIDI_OUTPUT         FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD   FALSE
    PLUGIN_MANUFACTURER_CODE  Hunt
    PLUGIN_CODE               Abys
    FORMATS                   VST3
    PRODUCT_NAME              "HUNTERICE ABYSS"
)

# Исходники (Убедитесь, что эти файлы физически существуют по этим путям в GitHub)
target_sources(HUNTERICE_ABYSS
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Заголовки
target_include_directories(HUNTERICE_ABYSS
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

# Подключаем модули JUCE и флаги
target_compile_definitions(HUNTERICE_ABYSS
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(HUNTERICE_ABYSS
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
