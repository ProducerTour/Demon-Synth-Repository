#pragma once

#include <JuceHeader.h>
#include "SampleZone.h"
#include <memory>
#include <map>

namespace NulyBeats {
namespace Engine {

/**
 * Manages sample-based presets
 * Loads samples from Resources/Samples/ and organizes them by category
 */
class SamplePresetManager
{
public:
    struct SamplePreset
    {
        juce::String name;
        juce::String category;
        juce::File sampleFile;
        int rootNote = 60;  // Default to C4
        bool loopEnabled = false;
    };

    SamplePresetManager() = default;

    void scanSampleDirectory(const juce::File& resourceDir)
    {
        presets.clear();
        categories.clear();

        auto samplesDir = resourceDir.getChildFile("Samples");
        if (!samplesDir.exists())
            return;

        // Scan each category folder
        for (const auto& categoryDir : samplesDir.findChildFiles(juce::File::findDirectories, false))
        {
            juce::String category = categoryDir.getFileName();
            categories.push_back(category);

            // Scan samples in this category
            for (const auto& sampleFile : categoryDir.findChildFiles(juce::File::findFiles, false, "*.wav;*.mp3;*.aif;*.aiff"))
            {
                SamplePreset preset;
                preset.name = sampleFile.getFileNameWithoutExtension();
                preset.category = category;
                preset.sampleFile = sampleFile;

                // Set default root note based on category
                if (category == "Arps")
                    preset.rootNote = 60;  // C4
                else if (category == "Bass")
                    preset.rootNote = 36;  // C2 for bass sounds
                else if (category == "Brass")
                    preset.rootNote = 60;
                else if (category == "Flutes")
                    preset.rootNote = 60;
                else if (category == "Keys")
                    preset.rootNote = 60;
                else if (category == "Leads")
                    preset.rootNote = 60;
                else if (category == "Pads")
                    preset.rootNote = 60;
                else if (category == "Plucks")
                    preset.rootNote = 60;
                else if (category == "SFX")
                    preset.rootNote = 60;
                else if (category == "Strings")
                    preset.rootNote = 60;
                else if (category == "Synths")
                    preset.rootNote = 60;
                else if (category == "WahWah")
                    preset.rootNote = 60;
                else
                    preset.rootNote = 60;

                presets.push_back(preset);
            }
        }
    }

    const std::vector<juce::String>& getCategories() const { return categories; }

    std::vector<SamplePreset> getPresetsInCategory(const juce::String& category) const
    {
        std::vector<SamplePreset> result;
        for (const auto& preset : presets)
        {
            if (preset.category == category)
                result.push_back(preset);
        }
        return result;
    }

    const std::vector<SamplePreset>& getAllPresets() const { return presets; }

    // Load a sample file into a SampleInstrument
    std::unique_ptr<SampleInstrument> loadPreset(const SamplePreset& preset, double sampleRate)
    {
        auto instrument = std::make_unique<SampleInstrument>();
        instrument->name = preset.name;
        instrument->addLayer();

        auto zone = std::make_unique<SampleZone>();
        zone->name = preset.name;
        zone->filePath = preset.sampleFile.getFullPathName();
        zone->rootNote = preset.rootNote;
        zone->lowKey = 0;
        zone->highKey = 127;
        zone->loopEnabled = preset.loopEnabled;

        // Load the audio file
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader(
            formatManager.createReaderFor(preset.sampleFile));

        if (reader != nullptr)
        {
            zone->originalSampleRate = reader->sampleRate;
            zone->audioData.setSize(1, static_cast<int>(reader->lengthInSamples));

            // Read as mono (mix stereo to mono if needed)
            if (reader->numChannels == 1)
            {
                reader->read(&zone->audioData, 0, static_cast<int>(reader->lengthInSamples), 0, true, false);
            }
            else
            {
                // Read stereo and mix to mono
                juce::AudioBuffer<float> stereoBuffer(2, static_cast<int>(reader->lengthInSamples));
                reader->read(&stereoBuffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);

                float* mono = zone->audioData.getWritePointer(0);
                const float* left = stereoBuffer.getReadPointer(0);
                const float* right = stereoBuffer.getReadPointer(1);

                for (int i = 0; i < stereoBuffer.getNumSamples(); ++i)
                {
                    mono[i] = (left[i] + right[i]) * 0.5f;
                }
            }

            // Set loop points if looping is enabled
            if (preset.loopEnabled)
            {
                zone->loopStart = 0;
                zone->loopEnd = zone->audioData.getNumSamples();
                zone->crossfadeLoop = true;
            }

            instrument->layers[0]->zones.push_back(std::move(zone));
        }

        return instrument;
    }

    // Find preset by name
    const SamplePreset* findPreset(const juce::String& name) const
    {
        for (const auto& preset : presets)
        {
            if (preset.name == name)
                return &preset;
        }
        return nullptr;
    }

private:
    std::vector<SamplePreset> presets;
    std::vector<juce::String> categories;
};

} // namespace Engine
} // namespace NulyBeats
