#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace NulyBeats {

PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Scan for sample presets - first try config file, then fallback paths
    juce::File samplesDir = getSamplesDirectory();

    // Create a fake resource dir with Samples subfolder for the manager
    juce::File resourceDir = samplesDir.getParentDirectory();

    samplePresetManager.scanSampleDirectory(resourceDir);

    // Debug: log what we found
    DBG("=== Sample Preset Manager Initialized ===");
    DBG("Samples dir: " + samplesDir.getFullPathName());
    DBG("Samples dir exists: " + juce::String(samplesDir.exists() ? "YES" : "NO"));
    DBG("Categories found: " + juce::String(samplePresetManager.getCategories().size()));
    for (const auto& cat : samplePresetManager.getCategories())
    {
        auto presets = samplePresetManager.getPresetsInCategory(cat);
        DBG("  " + cat + ": " + juce::String(presets.size()) + " presets");
    }
    DBG("=========================================");
}

PluginProcessor::~PluginProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // ===== Oscillator 1 =====
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"osc1_enabled", 1}, "Osc 1 Enabled", true));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"osc1_wave", 1}, "Osc 1 Wave",
        juce::StringArray{"Sine", "Saw", "Square", "Triangle", "Pulse", "Noise"}, 1));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"osc1_level", 1}, "Osc 1 Level",
        juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"osc1_octave", 1}, "Osc 1 Octave",
        juce::NormalisableRange<float>(-3.0f, 3.0f, 1.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"osc1_semi", 1}, "Osc 1 Semi",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 1.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"osc1_fine", 1}, "Osc 1 Fine",
        juce::NormalisableRange<float>(-100.0f, 100.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"osc1_pw", 1}, "Osc 1 Pulse Width",
        juce::NormalisableRange<float>(0.01f, 0.99f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"osc1_pan", 1}, "Osc 1 Pan",
        juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));

    // ===== Oscillator 2 =====
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"osc2_enabled", 1}, "Osc 2 Enabled", false));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"osc2_wave", 1}, "Osc 2 Wave",
        juce::StringArray{"Sine", "Saw", "Square", "Triangle", "Pulse", "Noise"}, 2));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"osc2_level", 1}, "Osc 2 Level",
        juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"osc2_octave", 1}, "Osc 2 Octave",
        juce::NormalisableRange<float>(-3.0f, 3.0f, 1.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"osc2_semi", 1}, "Osc 2 Semi",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 1.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"osc2_fine", 1}, "Osc 2 Fine",
        juce::NormalisableRange<float>(-100.0f, 100.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"osc2_pw", 1}, "Osc 2 Pulse Width",
        juce::NormalisableRange<float>(0.01f, 0.99f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"osc2_pan", 1}, "Osc 2 Pan",
        juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));

    // ===== Noise =====
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"noise_level", 1}, "Noise Level",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // ===== Filter =====
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"filter_type", 1}, "Filter Type",
        juce::StringArray{"Low Pass", "High Pass", "Band Pass", "Notch"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filter_cutoff", 1}, "Filter Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.0f, 0.25f), 20000.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filter_reso", 1}, "Filter Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filter_env_amt", 1}, "Filter Env Amount",
        juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filter_keytrack", 1}, "Filter Key Track",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // ===== Amp Envelope =====
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"amp_attack", 1}, "Amp Attack",
        juce::NormalisableRange<float>(0.001f, 10.0f, 0.0f, 0.3f), 0.01f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"amp_decay", 1}, "Amp Decay",
        juce::NormalisableRange<float>(0.001f, 10.0f, 0.0f, 0.3f), 0.1f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"amp_sustain", 1}, "Amp Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"amp_release", 1}, "Amp Release",
        juce::NormalisableRange<float>(0.001f, 10.0f, 0.0f, 0.3f), 0.3f));

    // ===== Filter Envelope =====
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filter_attack", 1}, "Filter Attack",
        juce::NormalisableRange<float>(0.001f, 10.0f, 0.0f, 0.3f), 0.01f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filter_decay", 1}, "Filter Decay",
        juce::NormalisableRange<float>(0.001f, 10.0f, 0.0f, 0.3f), 0.2f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filter_sustain", 1}, "Filter Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filter_release", 1}, "Filter Release",
        juce::NormalisableRange<float>(0.001f, 10.0f, 0.0f, 0.3f), 0.3f));

    // ===== LFO 1 =====
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"lfo1_wave", 1}, "LFO 1 Wave",
        juce::StringArray{"Sine", "Triangle", "Saw", "Reverse Saw", "Square", "S&H", "Smooth Random"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo1_rate", 1}, "LFO 1 Rate",
        juce::NormalisableRange<float>(0.01f, 50.0f, 0.0f, 0.3f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"lfo1_sync", 1}, "LFO 1 Sync", false));

    // ===== LFO 2 =====
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"lfo2_wave", 1}, "LFO 2 Wave",
        juce::StringArray{"Sine", "Triangle", "Saw", "Reverse Saw", "Square", "S&H", "Smooth Random"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo2_rate", 1}, "LFO 2 Rate",
        juce::NormalisableRange<float>(0.01f, 50.0f, 0.0f, 0.3f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"lfo2_sync", 1}, "LFO 2 Sync", false));

    // ===== Unison =====
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"unison_voices", 1}, "Unison Voices", 1, 8, 1));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"unison_detune", 1}, "Unison Detune",
        juce::NormalisableRange<float>(0.0f, 100.0f), 10.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"unison_spread", 1}, "Unison Spread",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

    // ===== Glide =====
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"glide_time", 1}, "Glide Time",
        juce::NormalisableRange<float>(0.0f, 2.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"glide_always", 1}, "Glide Always", false));

    // ===== Master =====
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"master_level", 1}, "Master Level",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));

    // ===== FX =====
    // Reverb
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"reverb_enabled", 1}, "Reverb Enabled", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"reverb_mix", 1}, "Reverb Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"reverb_size", 1}, "Reverb Size",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"reverb_damping", 1}, "Reverb Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

    // Delay
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"delay_enabled", 1}, "Delay Enabled", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"delay_mix", 1}, "Delay Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"delay_time", 1}, "Delay Time",
        juce::NormalisableRange<float>(0.01f, 2.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"delay_feedback", 1}, "Delay Feedback",
        juce::NormalisableRange<float>(0.0f, 0.99f), 0.5f));

    // Chorus
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"chorus_enabled", 1}, "Chorus Enabled", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"chorus_mix", 1}, "Chorus Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"chorus_rate", 1}, "Chorus Rate",
        juce::NormalisableRange<float>(0.1f, 10.0f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"chorus_depth", 1}, "Chorus Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.25f));

    // ===== Macro Controls =====
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"macro_boost", 1}, "Macro Boost",
        juce::NormalisableRange<float>(0.0f, 100.0f), 50.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"macro_air", 1}, "Macro Air",
        juce::NormalisableRange<float>(0.0f, 100.0f), 50.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"macro_body", 1}, "Macro Body",
        juce::NormalisableRange<float>(0.0f, 100.0f), 50.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"macro_warp", 1}, "Macro Warp",
        juce::NormalisableRange<float>(0.0f, 100.0f), 50.0f));

    return { params.begin(), params.end() };
}

void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    voiceManager.prepare(sampleRate, samplesPerBlock);
    sampleSynth.prepare(sampleRate, samplesPerBlock);
    fxRack.prepare(sampleRate, samplesPerBlock);
    globalModMatrix.prepare(sampleRate, samplesPerBlock);
    globalLFO1.prepare(sampleRate);
    globalLFO2.prepare(sampleRate);

    // Initialize parameter smoothing (20ms ramp time)
    double smoothingTime = 0.02; // 20ms
    smoothedFilterCutoff.reset(sampleRate, smoothingTime);
    smoothedMasterLevel.reset(sampleRate, smoothingTime);
    smoothedReverbMix.reset(sampleRate, smoothingTime);
    smoothedDelayMix.reset(sampleRate, smoothingTime);
    smoothedChorusMix.reset(sampleRate, smoothingTime);
}

void PluginProcessor::releaseResources()
{
    voiceManager.reset();
    fxRack.reset();
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Get tempo from host and sync to sample player
    bool gotBPMFromHost = false;
    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (auto bpm = position->getBpm())
            {
                if (std::abs(currentBPM - *bpm) > 0.1) // Only log when BPM changes
                {
                    DBG("Host BPM changed: " + juce::String(*bpm));
                }
                currentBPM = *bpm;
                sampleSynth.setHostBPM(currentBPM);
                gotBPMFromHost = true;
            }
        }
    }

    if (!gotBPMFromHost)
    {
        // No playhead or no BPM info - use default 140 BPM for standalone mode
        // This ensures samples play at a reasonable tempo
        currentBPM = 140.0;
        sampleSynth.setHostBPM(140.0);
    }

    // Update voice parameters from APVTS
    Engine::SynthVoice::Parameters voiceParams;

    voiceParams.osc1Enabled = apvts.getRawParameterValue("osc1_enabled")->load() > 0.5f;
    voiceParams.osc1Wave = static_cast<DSP::Oscillator::Waveform>(
        static_cast<int>(apvts.getRawParameterValue("osc1_wave")->load()));
    voiceParams.osc1Level = apvts.getRawParameterValue("osc1_level")->load();
    voiceParams.osc1Octave = apvts.getRawParameterValue("osc1_octave")->load();
    voiceParams.osc1Semi = apvts.getRawParameterValue("osc1_semi")->load();
    voiceParams.osc1Fine = apvts.getRawParameterValue("osc1_fine")->load();
    voiceParams.osc1PulseWidth = apvts.getRawParameterValue("osc1_pw")->load();
    voiceParams.osc1Pan = apvts.getRawParameterValue("osc1_pan")->load();

    voiceParams.osc2Enabled = apvts.getRawParameterValue("osc2_enabled")->load() > 0.5f;
    voiceParams.osc2Wave = static_cast<DSP::Oscillator::Waveform>(
        static_cast<int>(apvts.getRawParameterValue("osc2_wave")->load()));
    voiceParams.osc2Level = apvts.getRawParameterValue("osc2_level")->load();
    voiceParams.osc2Octave = apvts.getRawParameterValue("osc2_octave")->load();
    voiceParams.osc2Semi = apvts.getRawParameterValue("osc2_semi")->load();
    voiceParams.osc2Fine = apvts.getRawParameterValue("osc2_fine")->load();
    voiceParams.osc2PulseWidth = apvts.getRawParameterValue("osc2_pw")->load();
    voiceParams.osc2Pan = apvts.getRawParameterValue("osc2_pan")->load();

    voiceParams.noiseLevel = apvts.getRawParameterValue("noise_level")->load();

    voiceParams.filterType = static_cast<DSP::SVFFilter::Type>(
        static_cast<int>(apvts.getRawParameterValue("filter_type")->load()));

    // Use smoothed filter cutoff to avoid zipper noise
    smoothedFilterCutoff.setTargetValue(apvts.getRawParameterValue("filter_cutoff")->load());
    voiceParams.filterCutoff = smoothedFilterCutoff.getNextValue();

    voiceParams.filterResonance = apvts.getRawParameterValue("filter_reso")->load();
    voiceParams.filterEnvAmount = apvts.getRawParameterValue("filter_env_amt")->load();
    voiceParams.filterKeyTrack = apvts.getRawParameterValue("filter_keytrack")->load();

    voiceParams.ampAttack = apvts.getRawParameterValue("amp_attack")->load();
    voiceParams.ampDecay = apvts.getRawParameterValue("amp_decay")->load();
    voiceParams.ampSustain = apvts.getRawParameterValue("amp_sustain")->load();
    voiceParams.ampRelease = apvts.getRawParameterValue("amp_release")->load();

    voiceParams.filterAttack = apvts.getRawParameterValue("filter_attack")->load();
    voiceParams.filterDecay = apvts.getRawParameterValue("filter_decay")->load();
    voiceParams.filterSustain = apvts.getRawParameterValue("filter_sustain")->load();
    voiceParams.filterRelease = apvts.getRawParameterValue("filter_release")->load();

    voiceParams.glideTime = apvts.getRawParameterValue("glide_time")->load();
    voiceParams.glideAlways = apvts.getRawParameterValue("glide_always")->load() > 0.5f;

    // Use smoothed master level to avoid clicks
    smoothedMasterLevel.setTargetValue(apvts.getRawParameterValue("master_level")->load());
    voiceParams.masterLevel = smoothedMasterLevel.getNextValue();

    voiceManager.setVoiceParameters(voiceParams);

    // Unison
    int unisonVoices = static_cast<int>(apvts.getRawParameterValue("unison_voices")->load());
    float unisonDetune = apvts.getRawParameterValue("unison_detune")->load();
    float unisonSpread = apvts.getRawParameterValue("unison_spread")->load();
    voiceManager.setUnison(unisonVoices, unisonDetune, unisonSpread);

    // Process MIDI for synth voices
    for (const auto metadata : midiMessages)
    {
        voiceManager.handleMidiMessage(metadata.getMessage());
    }

    // Clear buffer once at the start
    buffer.clear();

    // Synth oscillators disabled - only samples play
    // voiceManager.processBlock(buffer, false);

    // Process sample synth - uses JUCE's Synthesiser which reads MIDI directly
    sampleSynth.processBlock(buffer, midiMessages);

    // Update FX parameters with smoothed mix values
    if (auto* reverb = fxRack.getEffect<DSP::ReverbEffect>())
    {
        reverb->setEnabled(apvts.getRawParameterValue("reverb_enabled")->load() > 0.5f);
        smoothedReverbMix.setTargetValue(apvts.getRawParameterValue("reverb_mix")->load());
        reverb->setMix(smoothedReverbMix.getNextValue());
        reverb->setRoomSize(apvts.getRawParameterValue("reverb_size")->load());
        reverb->setDamping(apvts.getRawParameterValue("reverb_damping")->load());
    }

    if (auto* delay = fxRack.getEffect<DSP::DelayEffect>())
    {
        delay->setEnabled(apvts.getRawParameterValue("delay_enabled")->load() > 0.5f);
        smoothedDelayMix.setTargetValue(apvts.getRawParameterValue("delay_mix")->load());
        delay->setMix(smoothedDelayMix.getNextValue());
        delay->setDelayTime(apvts.getRawParameterValue("delay_time")->load());
        delay->setFeedback(apvts.getRawParameterValue("delay_feedback")->load());
    }

    if (auto* chorus = fxRack.getEffect<DSP::ChorusEffect>())
    {
        chorus->setEnabled(apvts.getRawParameterValue("chorus_enabled")->load() > 0.5f);
        smoothedChorusMix.setTargetValue(apvts.getRawParameterValue("chorus_mix")->load());
        chorus->setMix(smoothedChorusMix.getNextValue());
        chorus->setRate(apvts.getRawParameterValue("chorus_rate")->load());
        chorus->setDepth(apvts.getRawParameterValue("chorus_depth")->load());
    }

    // Process FX
    fxRack.process(buffer);
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor(*this);
}

void PluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    DBG("=== getStateInformation called ===");
    DBG("  currentSamplePresetName: " + (currentSamplePresetName.isEmpty() ? "(EMPTY)" : currentSamplePresetName));
    DBG("  Sample loaded in synth: " + juce::String(sampleSynth.hasSampleLoaded() ? "YES" : "NO"));
    DBG("  stateHasBeenRestored: " + juce::String(stateHasBeenRestored ? "YES" : "NO"));

    auto state = apvts.copyState();

    // Only save preset name if we have one OR if state was properly restored
    // This protects against FL Studio calling getState before setState
    if (currentSamplePresetName.isNotEmpty() || stateHasBeenRestored)
    {
        state.setProperty("samplePresetName", currentSamplePresetName, nullptr);
    }
    else
    {
        // Check if there's already a preset name in the state (from previous save)
        // Don't overwrite it with empty
        juce::String existingName = state.getProperty("samplePresetName", "").toString();
        if (existingName.isNotEmpty())
        {
            DBG("  Preserving existing preset name in state: " + existingName);
            // Keep it as-is
        }
    }

    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    DBG("  XML output: " + xml->toString().substring(0, 500));

    copyXmlToBinary(*xml, destData);
    DBG("  State saved, size: " + juce::String(destData.getSize()) + " bytes");
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    DBG("=== setStateInformation called ===");
    DBG("  Data size: " + juce::String(sizeInBytes) + " bytes");

    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr)
    {
        DBG("  XML tag: " + xml->getTagName());
        DBG("  Expected tag: " + apvts.state.getType().toString());
        DBG("  XML preview: " + xml->toString().substring(0, 500));

        if (xml->hasTagName(apvts.state.getType()))
        {
            auto newState = juce::ValueTree::fromXml(*xml);

            // Restore sample preset if saved
            juce::String savedPresetName = newState.getProperty("samplePresetName", "").toString();
            DBG("  Saved preset name from state: '" + savedPresetName + "'");
            DBG("  Number of preset categories: " + juce::String(samplePresetManager.getCategories().size()));

            if (savedPresetName.isNotEmpty())
            {
                DBG("  Calling loadSamplePreset...");
                loadSamplePreset(savedPresetName);
            }
            else
            {
                DBG("  No preset name in state - skipping load");
            }

            apvts.replaceState(newState);
            stateHasBeenRestored = true;
            DBG("  State restored. Final currentSamplePresetName = '" + currentSamplePresetName + "'");
        }
        else
        {
            DBG("  ERROR: Tag mismatch, state not restored");
        }
    }
    else
    {
        DBG("  ERROR: Could not parse XML from binary data");
    }
}

void PluginProcessor::loadPreset(const juce::File& file)
{
    if (file.existsAsFile())
    {
        std::unique_ptr<juce::XmlElement> xml = juce::XmlDocument::parse(file);
        if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        {
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
        }
    }
}

void PluginProcessor::savePreset(const juce::File& file)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    xml->writeTo(file);
}

void PluginProcessor::loadSamplePreset(const juce::String& presetName)
{
    DBG("=== loadSamplePreset called ===");
    DBG("  Preset name: " + presetName);
    DBG("  Current preset before: " + (currentSamplePresetName.isEmpty() ? "(empty)" : currentSamplePresetName));

    // Write to a log file for debugging
    juce::File logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("DemonSynth_debug.log");
    logFile.appendText("loadSamplePreset: " + presetName + " at " + juce::Time::getCurrentTime().toString(true, true) + "\n");

    const auto* preset = samplePresetManager.findPreset(presetName);
    if (preset != nullptr)
    {
        DBG("  Found preset, loading file: " + preset->sampleFile.getFullPathName());
        logFile.appendText("  Found preset, file: " + preset->sampleFile.getFullPathName() + "\n");

        bool success = sampleSynth.loadSample(preset->sampleFile);
        DBG("  Sample load " + juce::String(success ? "SUCCESS" : "FAILED"));
        logFile.appendText("  Sample load: " + juce::String(success ? "SUCCESS" : "FAILED") + "\n");

        // Store preset name for state persistence
        if (success)
        {
            currentSamplePresetName = presetName;
            DBG("  Set currentSamplePresetName to: " + currentSamplePresetName);
            logFile.appendText("  Set currentSamplePresetName to: " + currentSamplePresetName + "\n");
        }
    }
    else
    {
        DBG("  Preset NOT found: " + presetName);
        logFile.appendText("  Preset NOT found: " + presetName + "\n");

        // Log available presets for debugging
        logFile.appendText("  Available categories: " + juce::String(samplePresetManager.getCategories().size()) + "\n");
        for (const auto& cat : samplePresetManager.getCategories())
        {
            auto presets = samplePresetManager.getPresetsInCategory(cat);
            logFile.appendText("    " + cat + ": " + juce::String(presets.size()) + " presets\n");
        }
    }
}

void PluginProcessor::clearSampleInstrument()
{
    sampleSynth.clearSample();
}

juce::File PluginProcessor::readSamplesPathFromConfig()
{
    // Platform-specific config location
#if JUCE_MAC
    juce::File configDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("NullyBeats/Demon Synth");
#elif JUCE_WINDOWS
    juce::File configDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("NullyBeats/Demon Synth");
#else
    juce::File configDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
        .getChildFile(".config/NullyBeats/Demon Synth");
#endif

    juce::File configFile = configDir.getChildFile("config.json");

    if (configFile.existsAsFile())
    {
        juce::String content = configFile.loadFileAsString();

        // Simple JSON parsing for samplesPath
        int pathStart = content.indexOf(juce::String("\"samplesPath\""));
        if (pathStart >= 0)
        {
            int valueStart = content.indexOfChar(pathStart, ':') + 1;
            int quoteStart = content.indexOfChar(valueStart, '"') + 1;
            int quoteEnd = content.indexOfChar(quoteStart, '"');

            if (quoteStart > 0 && quoteEnd > quoteStart)
            {
                juce::String path = content.substring(quoteStart, quoteEnd);
                // Unescape backslashes (from JSON)
                path = path.replace("\\\\", "\\");

                DBG("Config file found, samples path: " + path);
                return juce::File(path);
            }
        }
    }

    DBG("No config file found at: " + configFile.getFullPathName());
    return juce::File();
}

juce::File PluginProcessor::getSamplesDirectory()
{
    // First, try to read from config file (set by installer)
    juce::File configPath = readSamplesPathFromConfig();
    if (configPath.exists() && configPath.isDirectory())
    {
        DBG("Using samples path from config: " + configPath.getFullPathName());
        return configPath;
    }

    // Fallback paths for development and different installation scenarios
    std::vector<juce::File> searchPaths;

    // macOS: User Library location (installer default)
#if JUCE_MAC
    searchPaths.push_back(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("NullyBeats/Demon Synth/Samples"));
#endif

    // Windows: AppData location (installer default)
#if JUCE_WINDOWS
    searchPaths.push_back(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("NullyBeats/Demon Synth/Samples"));
#endif

    // Development path
    searchPaths.push_back(juce::File("/Users/nolangriffis/Documents/NullyBeatsPlugin/Resources/Samples"));

    // Relative to executable (for standalone app)
    searchPaths.push_back(juce::File::getSpecialLocation(juce::File::currentExecutableFile)
        .getParentDirectory().getParentDirectory().getChildFile("Resources/Samples"));

    // Relative to app bundle (macOS)
    searchPaths.push_back(juce::File::getSpecialLocation(juce::File::currentApplicationFile)
        .getParentDirectory().getParentDirectory().getChildFile("Resources/Samples"));

    // Try __FILE__ based path
    searchPaths.push_back(juce::File(__FILE__).getParentDirectory().getParentDirectory()
        .getParentDirectory().getChildFile("Resources/Samples"));

    // Find first existing path
    for (const auto& path : searchPaths)
    {
        if (path.exists() && path.isDirectory())
        {
            DBG("Found samples at: " + path.getFullPathName());
            return path;
        }
    }

    // Return default even if it doesn't exist
    DBG("No samples directory found, using default");
#if JUCE_MAC
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("NullyBeats/Demon Synth/Samples");
#else
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("NullyBeats/Demon Synth/Samples");
#endif
}

} // namespace NulyBeats

// Plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NulyBeats::PluginProcessor();
}
