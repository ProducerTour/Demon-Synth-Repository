cmake_minimum_required(VERSION 3.22)

project(NulyBeatsPlugin VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE Configuration
add_subdirectory(JUCE)

juce_add_plugin(NulyBeatsPlugin
    COMPANY_NAME "NullyBeats"
    BUNDLE_ID "com.nullybeats.demonsynth"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Nuly
    PLUGIN_CODE Nubs
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Demon Synth"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Graphics/square.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Graphics/square.png"
)

# Generate JuceHeader.h
juce_generate_juce_header(NulyBeatsPlugin)

# Binary resources (fonts and graphics)
juce_add_binary_data(BinaryResources SOURCES
    Resources/Fonts/Orbitron-Bold.ttf
    Resources/Fonts/Orbitron-Black.ttf
    Resources/Fonts/Rajdhani-Bold.ttf
    Resources/Fonts/Rajdhani-SemiBold.ttf
    "Resources/Fonts/Sofachrome Rg.otf"
    "Resources/Fonts/Sofachrome Rg It.otf"
    Resources/Graphics/square.png
)

# Source files
target_sources(NulyBeatsPlugin
    PRIVATE
        # Core
        Source/Core/PluginProcessor.cpp
        Source/Core/PluginEditor.cpp
        Source/Core/Parameters.cpp

        # DSP - Oscillators
        Source/DSP/Oscillators/Oscillator.cpp
        Source/DSP/Oscillators/WavetableOscillator.cpp
        Source/DSP/Oscillators/VAOscillator.cpp

        # DSP - Filters
        Source/DSP/Filters/SVFFilter.cpp
        Source/DSP/Filters/LadderFilter.cpp

        # DSP - Effects
        Source/DSP/Effects/FXRack.cpp
        Source/DSP/Effects/Reverb.cpp
        Source/DSP/Effects/Delay.cpp
        Source/DSP/Effects/Chorus.cpp
        Source/DSP/Effects/Distortion.cpp
        Source/DSP/Effects/Compressor.cpp
        Source/DSP/Effects/EQ.cpp

        # DSP - Modulators
        Source/DSP/Modulators/ADSR.cpp
        Source/DSP/Modulators/LFO.cpp

        # Engine
        Source/Engine/PCM/SamplePlayer.cpp
        Source/Engine/PCM/SampleZone.cpp
        Source/Engine/PCM/SampleStreamer.cpp
        Source/Engine/PCM/TimeStretch.cpp
        Source/Engine/Wavetable/WavetableEngine.cpp
        Source/Engine/Voice/SynthVoice.cpp
        Source/Engine/Voice/VoiceManager.cpp

        # Modulation
        Source/Modulation/ModMatrix.cpp
        Source/Modulation/ModSource.cpp
        Source/Modulation/ModDestination.cpp

        # UI
        Source/UI/Components/OscillatorPanel.cpp
        Source/UI/Components/FilterPanel.cpp
        Source/UI/Components/ModMatrixPanel.cpp
        Source/UI/Components/FXRackPanel.cpp
        Source/UI/Components/WaveformDisplay.cpp
        Source/UI/Components/Knob.cpp
        Source/UI/LookAndFeel/NulyBeatsLookAndFeel.cpp

        # HellcatUI (header-only components)
        Source/HellcatUI/HellcatLookAndFeel.h
        Source/HellcatUI/Components/HellcatTopBar.h
        Source/HellcatUI/Components/HellcatGauge.h
        Source/HellcatUI/Components/HellcatOscillatorPanel.h
        Source/HellcatUI/Components/HellcatFilterPanel.h
        Source/HellcatUI/Components/HellcatEnvelopeDisplay.h
        Source/HellcatUI/Components/HellcatMacroKnob.h
        Source/HellcatUI/Components/HellcatXYPad.h
        Source/HellcatUI/Components/HellcatTransportButton.h
        Source/HellcatUI/Components/HellcatModMatrix.h
        Source/HellcatUI/Components/HellcatLFOPanel.h
        Source/HellcatUI/Components/HellcatTabbedPanel.h

        # Utils
        Source/Utils/SIMDUtils.cpp
        Source/Utils/AudioHelpers.cpp
)

target_compile_definitions(NulyBeatsPlugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

# SIMD optimizations
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(NulyBeatsPlugin PRIVATE
        -O3
    )
elseif(MSVC)
    target_compile_options(NulyBeatsPlugin PRIVATE
        /arch:AVX2
        /O2
    )
endif()

target_link_libraries(NulyBeatsPlugin
    PRIVATE
        BinaryResources
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
