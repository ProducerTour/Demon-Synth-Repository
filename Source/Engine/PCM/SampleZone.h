#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

namespace NulyBeats {
namespace Engine {

/**
 * Represents a single sample zone in a multisample instrument
 * Handles: key range, velocity layers, round-robin, loop points
 */
struct SampleZone
{
    // Sample data
    juce::AudioBuffer<float> audioData;
    juce::String filePath;
    juce::String name;

    // Key mapping
    int rootNote = 60;          // MIDI note the sample was recorded at
    int lowKey = 0;             // Lowest key this zone responds to
    int highKey = 127;          // Highest key this zone responds to

    // Velocity mapping
    int lowVelocity = 0;
    int highVelocity = 127;

    // Round-robin
    int roundRobinGroup = 0;    // Which RR group this belongs to
    int roundRobinIndex = 0;    // Position within the group

    // Loop points
    bool loopEnabled = false;
    int loopStart = 0;
    int loopEnd = 0;
    bool crossfadeLoop = false;
    int crossfadeSamples = 512;

    // Playback
    double originalSampleRate = 44100.0;
    float gainDb = 0.0f;
    float pan = 0.0f;           // -1 to 1
    float fineTune = 0.0f;      // cents

    // Streaming
    bool streamFromDisk = false;
    int64_t fileStartPosition = 0;  // For streaming

    bool containsNote(int midiNote) const
    {
        return midiNote >= lowKey && midiNote <= highKey;
    }

    bool containsVelocity(int velocity) const
    {
        return velocity >= lowVelocity && velocity <= highVelocity;
    }

    float getPitchRatio(int midiNote) const
    {
        float semitones = static_cast<float>(midiNote - rootNote) + fineTune / 100.0f;
        return std::pow(2.0f, semitones / 12.0f);
    }

    float getGain() const
    {
        return std::pow(10.0f, gainDb / 20.0f);
    }
};

/**
 * A layer containing multiple velocity-split zones
 */
struct SampleLayer
{
    std::vector<std::unique_ptr<SampleZone>> zones;
    int layerIndex = 0;

    // Per-layer envelope overrides
    bool useLayerEnvelope = false;
    float attack = 0.01f;
    float decay = 0.1f;
    float sustain = 0.7f;
    float release = 0.3f;

    // Per-layer filter
    bool useLayerFilter = false;
    float filterCutoff = 20000.0f;
    float filterResonance = 0.0f;

    SampleZone* findZone(int midiNote, int velocity, int roundRobinCounter) const
    {
        std::vector<SampleZone*> matches;

        for (const auto& zone : zones)
        {
            if (zone->containsNote(midiNote) && zone->containsVelocity(velocity))
            {
                matches.push_back(zone.get());
            }
        }

        if (matches.empty())
            return nullptr;

        // Handle round-robin selection
        if (matches.size() > 1)
        {
            // Filter by round-robin group
            int maxRR = 0;
            for (auto* z : matches)
                maxRR = std::max(maxRR, z->roundRobinIndex + 1);

            if (maxRR > 1)
            {
                int rrIndex = roundRobinCounter % maxRR;
                for (auto* z : matches)
                {
                    if (z->roundRobinIndex == rrIndex)
                        return z;
                }
            }
        }

        return matches.front();
    }
};

/**
 * A complete multisample instrument (like one "program" or "patch")
 */
struct SampleInstrument
{
    juce::String name;
    std::vector<std::unique_ptr<SampleLayer>> layers;

    // Global settings
    float masterGain = 0.0f;
    int polyphony = 32;
    bool monoMode = false;
    bool legatoMode = false;

    // Global envelope
    float attack = 0.01f;
    float decay = 0.1f;
    float sustain = 0.7f;
    float release = 0.3f;

    void addLayer()
    {
        auto layer = std::make_unique<SampleLayer>();
        layer->layerIndex = static_cast<int>(layers.size());
        layers.push_back(std::move(layer));
    }
};

} // namespace Engine
} // namespace NulyBeats
