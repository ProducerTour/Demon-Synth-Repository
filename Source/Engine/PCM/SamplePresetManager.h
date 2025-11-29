#pragma once

#include <JuceHeader.h>
#include "SampleZone.h"
#include <memory>
#include <map>
#include <set>

namespace NulyBeats {
namespace Engine {

/**
 * Manages sample-based presets
 * Loads samples from Resources/Samples/ and organizes them by category
 */
class SamplePresetManager
{
public:
    // Represents a single sample zone within a multisampled preset
    struct SampleZoneInfo
    {
        juce::File sampleFile;
        int rootNote = 60;      // The MIDI note this sample was recorded at
        int lowKey = 0;         // Lowest MIDI note this sample plays
        int highKey = 127;      // Highest MIDI note this sample plays
    };

    struct SamplePreset
    {
        juce::String name;
        juce::String category;
        juce::File sampleFile;          // For single-sample presets (backwards compat)
        int rootNote = 60;              // Default to C4
        bool loopEnabled = false;
        bool isMultisampled = false;    // True if this has multiple samples
        std::vector<SampleZoneInfo> zones;  // All sample zones for multisampled presets
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

            // Check if this is a multisampled category (like Pulsating)
            // These have subdirectories, each representing one preset
            auto subDirs = categoryDir.findChildFiles(juce::File::findDirectories, false);
            bool isMultisampledCategory = !subDirs.isEmpty();

            if (isMultisampledCategory)
            {
                // Each subdirectory is a preset - load ALL samples with their root notes
                for (const auto& presetDir : subDirs)
                {
                    auto wavFiles = presetDir.findChildFiles(juce::File::findFiles, false, "*.wav");
                    if (wavFiles.isEmpty())
                        continue;

                    SamplePreset preset;
                    preset.name = presetDir.getFileName();
                    preset.category = category;
                    preset.isMultisampled = true;

                    // Collect all samples with their MIDI notes
                    std::vector<std::pair<int, juce::File>> notesAndFiles;

                    juce::String presetFolderName = presetDir.getFileName();

                    // First pass: count unique base names to detect if folder has mixed presets
                    std::set<juce::String> uniqueBaseNames;
                    for (const auto& wav : wavFiles)
                    {
                        juce::String fileName = wav.getFileNameWithoutExtension();
                        int msIndex = fileName.indexOf("_ms0_");
                        juce::String baseName = (msIndex > 0) ? fileName.substring(0, msIndex) : fileName;
                        uniqueBaseNames.insert(baseName.toLowerCase());
                    }

                    // If all files share the same base name, include them all
                    // (handles cases like "First Choice" folder with "Init Patch" samples)
                    bool allSameBaseName = (uniqueBaseNames.size() == 1);

                    for (const auto& wav : wavFiles)
                    {
                        juce::String fileName = wav.getFileNameWithoutExtension();

                        // If all files have the same base name, include them all
                        // Otherwise, filter to only include files matching the folder name
                        bool belongsToPreset = allSameBaseName;

                        if (!allSameBaseName)
                        {
                            // Extract the base name from the wav file (before _ms0_XXX_)
                            int msIndex = fileName.indexOf("_ms0_");
                            juce::String baseName = (msIndex > 0) ? fileName.substring(0, msIndex) : fileName;

                            // Remove common prefixes like "A001 ", "F018 " etc
                            if (baseName.length() > 5 && baseName[4] == ' ')
                            {
                                bool hasPrefix = true;
                                for (int c = 0; c < 4; ++c)
                                {
                                    char ch = baseName[c];
                                    if (!((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')))
                                        hasPrefix = false;
                                }
                                if (hasPrefix)
                                    baseName = baseName.substring(5);
                            }

                            // Check if folder name matches the cleaned base name
                            if (presetFolderName.equalsIgnoreCase(baseName) ||
                                presetFolderName.containsIgnoreCase(baseName) ||
                                baseName.containsIgnoreCase(presetFolderName))
                            {
                                belongsToPreset = true;
                            }

                            // Skip generic "Init Patch" samples when mixed with other samples
                            if (fileName.startsWithIgnoreCase("Init Patch"))
                                belongsToPreset = false;
                        }

                        if (!belongsToPreset)
                            continue;

                        // Parse note from filename like "ShapeURMusic_ms0_060_c3.wav"
                        // or "Init Patch_ms0_060_c3.wav"
                        juce::String name = fileName;

                        // Look for _XXX_ pattern where XXX is MIDI note number
                        int lastUnderscore = name.lastIndexOf("_");
                        if (lastUnderscore > 0)
                        {
                            // Find second-to-last underscore by searching in substring
                            juce::String beforeLast = name.substring(0, lastUnderscore);
                            int secondLastUnderscore = beforeLast.lastIndexOf("_");
                            if (secondLastUnderscore >= 0)
                            {
                                juce::String noteStr = name.substring(secondLastUnderscore + 1, lastUnderscore);
                                int midiNote = noteStr.getIntValue();
                                if (midiNote >= 21 && midiNote <= 108)
                                {
                                    notesAndFiles.push_back({midiNote, wav});
                                }
                            }
                        }
                    }

                    // Sort by MIDI note
                    std::sort(notesAndFiles.begin(), notesAndFiles.end(),
                        [](const auto& a, const auto& b) { return a.first < b.first; });

                    // Calculate key ranges for each sample
                    // Each sample covers from halfway to previous sample to halfway to next sample
                    for (size_t i = 0; i < notesAndFiles.size(); ++i)
                    {
                        SampleZoneInfo zone;
                        zone.sampleFile = notesAndFiles[i].second;
                        zone.rootNote = notesAndFiles[i].first;

                        // Calculate low key (halfway between this and previous sample)
                        if (i == 0)
                            zone.lowKey = 0;  // First sample covers from bottom
                        else
                            zone.lowKey = (notesAndFiles[i-1].first + notesAndFiles[i].first) / 2 + 1;

                        // Calculate high key (halfway between this and next sample)
                        if (i == notesAndFiles.size() - 1)
                            zone.highKey = 127;  // Last sample covers to top
                        else
                            zone.highKey = (notesAndFiles[i].first + notesAndFiles[i+1].first) / 2;

                        preset.zones.push_back(zone);
                    }

                    // Also set the main sampleFile to C4 or closest for backwards compat
                    if (!notesAndFiles.empty())
                    {
                        int bestIdx = 0;
                        int bestDist = std::abs(notesAndFiles[0].first - 60);
                        for (size_t i = 1; i < notesAndFiles.size(); ++i)
                        {
                            int dist = std::abs(notesAndFiles[i].first - 60);
                            if (dist < bestDist)
                            {
                                bestDist = dist;
                                bestIdx = static_cast<int>(i);
                            }
                        }
                        preset.sampleFile = notesAndFiles[bestIdx].second;
                        preset.rootNote = notesAndFiles[bestIdx].first;
                    }

                    presets.push_back(preset);
                }
            }
            else
            {
                // Standard category with wav files directly in folder
                for (const auto& sampleFile : categoryDir.findChildFiles(juce::File::findFiles, false, "*.wav;*.mp3;*.aif;*.aiff"))
                {
                    SamplePreset preset;
                    preset.name = sampleFile.getFileNameWithoutExtension();
                    preset.category = category;
                    preset.sampleFile = sampleFile;

                    // Set default root note based on category
                    if (category == "Bass")
                        preset.rootNote = 36;  // C2 for bass sounds
                    else
                        preset.rootNote = 60;  // C4 for everything else

                    presets.push_back(preset);
                }
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
