#pragma once

#include <JuceHeader.h>
#include "../Engine/Voice/VoiceManager.h"
#include "../Engine/PCM/SampleSynth.h"
#include "../Engine/PCM/SamplePresetManager.h"
#include "../DSP/Effects/FXRack.h"
#include "../Modulation/ModMatrix.h"
#include "../Modulation/MidiLearn.h"
#include <atomic>

namespace NulyBeats {

/**
 * Main audio processor for NulyBeats Synth
 * Hybrid ROMpler + VA/Wavetable synth engine
 */
class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    // AudioProcessor interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Engine access
    Engine::VoiceManager& getVoiceManager() { return voiceManager; }
    Engine::SampleSynth& getSampleSynth() { return sampleSynth; }
    DSP::FXRack& getFXRack() { return fxRack; }
    Modulation::ModMatrix& getGlobalModMatrix() { return globalModMatrix; }

    // Parameter access
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // Preset management
    void loadPreset(const juce::File& file);
    void savePreset(const juce::File& file);

    // Sample preset management
    Engine::SamplePresetManager& getSamplePresetManager() { return samplePresetManager; }
    void loadSamplePreset(const juce::String& presetName);
    void clearSampleInstrument();

    // Get samples directory from config
    juce::File getSamplesDirectory();

    // Get current sample preset name (for UI state restoration)
    const juce::String& getCurrentSamplePresetName() const { return currentSamplePresetName; }

    // Audio level metering (thread-safe)
    float getRmsLevel() const { return currentRmsLevel.load(std::memory_order_relaxed); }

    // Oscilloscope scope buffer (lock-free audio→GUI)
    static constexpr int SCOPE_SIZE = 512;
    void pushScopeData(const float* data, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            scopeBuffer[static_cast<size_t>(scopeWritePos)] = data[i];
            scopeWritePos = (scopeWritePos + 1) % SCOPE_SIZE;
        }
        scopeReady.store(true, std::memory_order_release);
    }
    const std::array<float, SCOPE_SIZE>& getScopeBuffer() const { return scopeBuffer; }
    bool isScopeReady() const { return scopeReady.load(std::memory_order_acquire); }
    void clearScopeReady() { scopeReady.store(false, std::memory_order_release); }

    // MIDI learn
    Modulation::MidiLearn& getMidiLearn() { return midiLearn; }

    // Mod matrix routing (5 rows: srcId, dstId, amount)
    struct ModRowData { int srcId = 1, dstId = 1; float amount = 0.0f; };
    void setModMatrixRow(int row, int srcId, int dstId, float amount);
    const std::array<ModRowData, 5>& getModMatrixRows() const { return modMatrixRows; }

private:
    // Read config file for samples path
    juce::File readSamplesPathFromConfig();
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Synth engine
    Engine::VoiceManager voiceManager;
    Engine::SampleSynth sampleSynth;
    Engine::SamplePresetManager samplePresetManager;

    // Effects
    DSP::FXRack fxRack;

    // Global modulation
    Modulation::ModMatrix globalModMatrix;

    // LFOs for global modulation
    DSP::LFO globalLFO1;
    DSP::LFO globalLFO2;

    // Parameters
    juce::AudioProcessorValueTreeState apvts;

    // Smoothed parameters (to avoid zipper noise during automation)
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothedFilterCutoff{20000.0f};
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedMasterLevel{0.7f};
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedReverbMix{0.3f};
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDelayMix{0.3f};
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedChorusMix{0.5f};
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedFlangerMix{0.5f};

    // Tempo sync
    double currentBPM = 120.0;

    // Currently loaded sample preset name (for state persistence)
    juce::String currentSamplePresetName;

    // Flag to track if setStateInformation has been called
    // This protects against FL Studio's bug where getState is called before setState
    bool stateHasBeenRestored = false;

    // Audio level metering
    std::atomic<float> currentRmsLevel{0.0f};

    // Oscilloscope buffer
    std::array<float, SCOPE_SIZE> scopeBuffer{};
    int scopeWritePos = 0;
    std::atomic<bool> scopeReady{false};
    std::vector<float> scopeMonoBuffer; // Pre-allocated in prepareToPlay

    // MIDI learn
    Modulation::MidiLearn midiLearn;

    // Mod matrix UI routing storage
    std::array<ModRowData, 5> modMatrixRows{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};

} // namespace NulyBeats
