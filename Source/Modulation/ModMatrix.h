#pragma once

#include <JuceHeader.h>
#include <vector>
#include <array>
#include <unordered_map>
#include <functional>

namespace NulyBeats {
namespace Modulation {

/**
 * Modulation sources available in the synth
 */
enum class ModSource
{
    None = 0,

    // Envelopes
    AmpEnv,
    FilterEnv,
    ModEnv1,
    ModEnv2,

    // LFOs
    LFO1,
    LFO2,
    LFO3,
    LFO4,

    // MIDI
    Velocity,
    Aftertouch,
    ModWheel,
    PitchBend,
    KeyTrack,

    // Macros
    Macro1,
    Macro2,
    Macro3,
    Macro4,

    // Performance
    Random,
    SampleAndHold,

    COUNT
};

/**
 * Modulation destinations
 */
enum class ModDest
{
    None = 0,

    // Oscillators
    Osc1Pitch,
    Osc1Fine,
    Osc1WavePos,
    Osc1PulseWidth,
    Osc1Level,
    Osc1Pan,

    Osc2Pitch,
    Osc2Fine,
    Osc2WavePos,
    Osc2PulseWidth,
    Osc2Level,
    Osc2Pan,

    // Noise
    NoiseLevel,

    // Filter
    FilterCutoff,
    FilterResonance,
    FilterDrive,
    FilterMix,

    // Amp
    AmpLevel,
    AmpPan,

    // LFOs
    LFO1Rate,
    LFO2Rate,
    LFO3Rate,
    LFO4Rate,

    // Effects
    FXMix,
    ReverbMix,
    DelayMix,
    DelayTime,
    ChorusMix,
    ChorusRate,

    // Master
    MasterPitch,
    MasterLevel,

    COUNT
};

/**
 * A single modulation routing
 */
struct ModRouting
{
    ModSource source = ModSource::None;
    ModDest destination = ModDest::None;
    float amount = 0.0f;     // -1 to 1
    bool bipolar = true;     // false = unipolar (0 to 1)

    // Via modulation (modulate the amount)
    ModSource viaSource = ModSource::None;
    float viaAmount = 0.0f;
};

/**
 * Full modulation matrix with per-voice and global modulation
 */
class ModMatrix
{
public:
    static constexpr int MAX_ROUTINGS = 32;
    static constexpr int NUM_SOURCES = static_cast<int>(ModSource::COUNT);
    static constexpr int NUM_DESTS = static_cast<int>(ModDest::COUNT);

    ModMatrix() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        this->sampleRate = sampleRate;
        reset();
    }

    // Add a modulation routing
    bool addRouting(ModSource src, ModDest dst, float amount, bool bipolar = true)
    {
        if (numRoutings >= MAX_ROUTINGS)
            return false;

        routings[numRoutings].source = src;
        routings[numRoutings].destination = dst;
        routings[numRoutings].amount = amount;
        routings[numRoutings].bipolar = bipolar;
        ++numRoutings;

        return true;
    }

    // Add routing with via modulation
    bool addRouting(ModSource src, ModDest dst, float amount, ModSource via, float viaAmt)
    {
        if (numRoutings >= MAX_ROUTINGS)
            return false;

        routings[numRoutings].source = src;
        routings[numRoutings].destination = dst;
        routings[numRoutings].amount = amount;
        routings[numRoutings].viaSource = via;
        routings[numRoutings].viaAmount = viaAmt;
        ++numRoutings;

        return true;
    }

    void removeRouting(int index)
    {
        if (index < 0 || index >= numRoutings)
            return;

        for (int i = index; i < numRoutings - 1; ++i)
            routings[i] = routings[i + 1];

        --numRoutings;
    }

    void clearRoutings()
    {
        numRoutings = 0;
    }

    // Set source value (called by voice/global modulators)
    void setSourceValue(ModSource source, float value)
    {
        if (source != ModSource::None && source != ModSource::COUNT)
            sourceValues[static_cast<int>(source)] = value;
    }

    // Get current modulated value for a destination
    float getModulatedValue(ModDest dest, float baseValue) const
    {
        if (dest == ModDest::None || dest == ModDest::COUNT)
            return baseValue;

        float modulation = 0.0f;

        for (int i = 0; i < numRoutings; ++i)
        {
            const auto& r = routings[i];

            if (r.destination != dest)
                continue;

            float srcValue = sourceValues[static_cast<int>(r.source)];

            // Apply bipolar/unipolar
            if (!r.bipolar)
                srcValue = (srcValue + 1.0f) * 0.5f;

            float amount = r.amount;

            // Apply via modulation
            if (r.viaSource != ModSource::None)
            {
                float viaValue = sourceValues[static_cast<int>(r.viaSource)];
                amount *= viaValue * r.viaAmount + (1.0f - r.viaAmount);
            }

            modulation += srcValue * amount;
        }

        return baseValue + modulation;
    }

    // Get just the modulation amount (without base value)
    float getModulation(ModDest dest) const
    {
        return getModulatedValue(dest, 0.0f);
    }

    // Process one sample - updates all destinations
    void process()
    {
        // Reset destination values
        std::fill(destValues.begin(), destValues.end(), 0.0f);

        // Calculate all modulation
        for (int i = 0; i < numRoutings; ++i)
        {
            const auto& r = routings[i];

            if (r.destination == ModDest::None)
                continue;

            float srcValue = sourceValues[static_cast<int>(r.source)];

            if (!r.bipolar)
                srcValue = (srcValue + 1.0f) * 0.5f;

            float amount = r.amount;

            if (r.viaSource != ModSource::None)
            {
                float viaValue = sourceValues[static_cast<int>(r.viaSource)];
                amount *= viaValue * r.viaAmount + (1.0f - r.viaAmount);
            }

            destValues[static_cast<int>(r.destination)] += srcValue * amount;
        }
    }

    // Get pre-calculated destination value
    float getDestinationValue(ModDest dest) const
    {
        if (dest == ModDest::None || dest == ModDest::COUNT)
            return 0.0f;
        return destValues[static_cast<int>(dest)];
    }

    void reset()
    {
        std::fill(sourceValues.begin(), sourceValues.end(), 0.0f);
        std::fill(destValues.begin(), destValues.end(), 0.0f);
    }

    // Access routings for UI
    const ModRouting& getRouting(int index) const { return routings[index]; }
    ModRouting& getRouting(int index) { return routings[index]; }
    int getNumRoutings() const { return numRoutings; }

    // String helpers for UI
    static juce::String getSourceName(ModSource src)
    {
        switch (src)
        {
            case ModSource::None: return "None";
            case ModSource::AmpEnv: return "Amp Env";
            case ModSource::FilterEnv: return "Filter Env";
            case ModSource::ModEnv1: return "Mod Env 1";
            case ModSource::ModEnv2: return "Mod Env 2";
            case ModSource::LFO1: return "LFO 1";
            case ModSource::LFO2: return "LFO 2";
            case ModSource::LFO3: return "LFO 3";
            case ModSource::LFO4: return "LFO 4";
            case ModSource::Velocity: return "Velocity";
            case ModSource::Aftertouch: return "Aftertouch";
            case ModSource::ModWheel: return "Mod Wheel";
            case ModSource::PitchBend: return "Pitch Bend";
            case ModSource::KeyTrack: return "Key Track";
            case ModSource::Macro1: return "Macro 1";
            case ModSource::Macro2: return "Macro 2";
            case ModSource::Macro3: return "Macro 3";
            case ModSource::Macro4: return "Macro 4";
            case ModSource::Random: return "Random";
            case ModSource::SampleAndHold: return "S&H";
            default: return "Unknown";
        }
    }

    static juce::String getDestName(ModDest dst)
    {
        switch (dst)
        {
            case ModDest::None: return "None";
            case ModDest::Osc1Pitch: return "Osc 1 Pitch";
            case ModDest::Osc1Fine: return "Osc 1 Fine";
            case ModDest::Osc1WavePos: return "Osc 1 WavePos";
            case ModDest::Osc1PulseWidth: return "Osc 1 PW";
            case ModDest::Osc1Level: return "Osc 1 Level";
            case ModDest::Osc1Pan: return "Osc 1 Pan";
            case ModDest::Osc2Pitch: return "Osc 2 Pitch";
            case ModDest::Osc2Fine: return "Osc 2 Fine";
            case ModDest::Osc2WavePos: return "Osc 2 WavePos";
            case ModDest::Osc2PulseWidth: return "Osc 2 PW";
            case ModDest::Osc2Level: return "Osc 2 Level";
            case ModDest::Osc2Pan: return "Osc 2 Pan";
            case ModDest::NoiseLevel: return "Noise Level";
            case ModDest::FilterCutoff: return "Filter Cutoff";
            case ModDest::FilterResonance: return "Filter Reso";
            case ModDest::FilterDrive: return "Filter Drive";
            case ModDest::FilterMix: return "Filter Mix";
            case ModDest::AmpLevel: return "Amp Level";
            case ModDest::AmpPan: return "Amp Pan";
            case ModDest::LFO1Rate: return "LFO 1 Rate";
            case ModDest::LFO2Rate: return "LFO 2 Rate";
            case ModDest::LFO3Rate: return "LFO 3 Rate";
            case ModDest::LFO4Rate: return "LFO 4 Rate";
            case ModDest::FXMix: return "FX Mix";
            case ModDest::ReverbMix: return "Reverb Mix";
            case ModDest::DelayMix: return "Delay Mix";
            case ModDest::DelayTime: return "Delay Time";
            case ModDest::ChorusMix: return "Chorus Mix";
            case ModDest::ChorusRate: return "Chorus Rate";
            case ModDest::MasterPitch: return "Master Pitch";
            case ModDest::MasterLevel: return "Master Level";
            default: return "Unknown";
        }
    }

private:
    double sampleRate = 44100.0;

    std::array<ModRouting, MAX_ROUTINGS> routings;
    int numRoutings = 0;

    std::array<float, NUM_SOURCES> sourceValues{};
    std::array<float, NUM_DESTS> destValues{};
};

} // namespace Modulation
} // namespace NulyBeats
