#pragma once

#include <JuceHeader.h>

namespace NulyBeats {
namespace Engine {

/**
 * Extended SamplerSound that stores original BPM for tempo sync
 */
class TempoSyncSamplerSound : public juce::SynthesiserSound
{
public:
    TempoSyncSamplerSound(const juce::String& soundName,
                          juce::AudioFormatReader& source,
                          const juce::BigInteger& midiNotes,
                          int midiNoteForNormalPitch,
                          double attackTimeSecs,
                          double releaseTimeSecs,
                          double maxSampleLengthSeconds,
                          double originalBPM)
        : name(soundName),
          sourceSampleRate(source.sampleRate),
          midiNotes(midiNotes),
          midiRootNote(midiNoteForNormalPitch),
          attackTime(static_cast<float>(attackTimeSecs)),
          releaseTime(static_cast<float>(releaseTimeSecs)),
          originalBPM(originalBPM)
    {
        // Load the audio data
        if (source.lengthInSamples > 0)
        {
            length = juce::jmin((int)source.lengthInSamples,
                                (int)(maxSampleLengthSeconds * source.sampleRate));

            data.reset(new juce::AudioBuffer<float>(juce::jmin(2, (int)source.numChannels), length + 4));
            source.read(data.get(), 0, length + 4, 0, true, true);
        }
    }

    bool appliesToNote(int midiNoteNumber) override { return midiNotes[midiNoteNumber]; }
    bool appliesToChannel(int /*midiChannel*/) override { return true; }

    double getOriginalBPM() const { return originalBPM; }
    int getMidiNoteForNormalPitch() const { return midiRootNote; }
    juce::AudioBuffer<float>* getAudioData() const { return data.get(); }
    float getAttackTime() const { return attackTime; }
    float getReleaseTime() const { return releaseTime; }
    double getSourceSampleRate() const { return sourceSampleRate; }

private:
    juce::String name;
    std::unique_ptr<juce::AudioBuffer<float>> data;
    double sourceSampleRate = 44100.0;
    juce::BigInteger midiNotes;
    int length = 0;
    int midiRootNote = 60;
    float attackTime = 0.01f;
    float releaseTime = 0.1f;
    double originalBPM = 120.0;
};

/**
 * Custom SamplerVoice with tempo sync support
 * Adjusts playback rate to match DAW tempo by modifying pitch ratio
 */
class TempoSyncSamplerVoice : public juce::SynthesiserVoice
{
public:
    void setHostBPM(double bpm)
    {
        hostBPM = bpm;
        updatePitchRatio();
    }

    void setTempoSyncEnabled(bool enabled)
    {
        tempoSyncEnabled = enabled;
        updatePitchRatio();
    }

    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<TempoSyncSamplerSound*>(sound) != nullptr;
    }

    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound* s, int currentPitchWheelPosition) override
    {
        if (auto* sound = dynamic_cast<TempoSyncSamplerSound*>(s))
        {
            currentMidiNote = midiNoteNumber;
            currentPitchWheel = currentPitchWheelPosition;

            // Sample rate conversion ratio
            baseSampleRateRatio = sound->getSourceSampleRate() / getSampleRate();

            // Standard sampler pitch tracking:
            // Root note (60 = C4) plays at original pitch
            // Each semitone up/down changes pitch by 2^(1/12)
            // This is the traditional "repitch" behavior used in samplers like Kontakt
            baseFrequencyRatio = std::pow(2.0, (midiNoteNumber - sound->getMidiNoteForNormalPitch()) / 12.0);

            updatePitchRatio();

            sourceSamplePosition = 0.0;
            lgain = velocity;
            rgain = velocity;

            adsr.setSampleRate(getSampleRate());
            adsr.setParameters({ sound->getAttackTime(), 0.0f, 1.0f, sound->getReleaseTime() });
            adsr.noteOn();

            DBG("Note ON - MIDI: " + juce::String(midiNoteNumber) +
                " Root: " + juce::String(sound->getMidiNoteForNormalPitch()) +
                " Freq Ratio: " + juce::String(baseFrequencyRatio) +
                " Pitch Ratio: " + juce::String(pitchRatio));
        }
        else
        {
            jassertfalse;
        }
    }

    void stopNote(float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            adsr.noteOff();
        }
        else
        {
            clearCurrentNote();
            adsr.reset();
        }
    }

    void pitchWheelMoved(int newPitchWheelValue) override
    {
        currentPitchWheel = newPitchWheelValue;
        updatePitchRatio();
    }

    void controllerMoved(int /*controllerNumber*/, int /*newControllerValue*/) override
    {
    }

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                         int startSample, int numSamples) override
    {
        if (auto* sound = dynamic_cast<TempoSyncSamplerSound*>(getCurrentlyPlayingSound().get()))
        {
            auto& data = *sound->getAudioData();
            const float* const inL = data.getReadPointer(0);
            const float* const inR = data.getNumChannels() > 1 ? data.getReadPointer(1) : inL;

            float* outL = outputBuffer.getWritePointer(0, startSample);
            float* outR = outputBuffer.getNumChannels() > 1 ? outputBuffer.getWritePointer(1, startSample) : nullptr;

            while (--numSamples >= 0)
            {
                auto pos = (int)sourceSamplePosition;
                auto alpha = (float)(sourceSamplePosition - pos);
                auto invAlpha = 1.0f - alpha;

                // Linear interpolation
                float l = 0, r = 0;

                if (pos < data.getNumSamples() - 1)
                {
                    l = (inL[pos] * invAlpha + inL[pos + 1] * alpha);
                    r = (inR[pos] * invAlpha + inR[pos + 1] * alpha);
                }

                auto envelopeValue = adsr.getNextSample();

                l *= lgain * envelopeValue;
                r *= rgain * envelopeValue;

                *outL++ += l;
                if (outR != nullptr)
                    *outR++ += r;

                sourceSamplePosition += pitchRatio;

                if (sourceSamplePosition >= data.getNumSamples() - 1 || !adsr.isActive())
                {
                    clearCurrentNote();
                    break;
                }
            }
        }
    }

private:
    void updatePitchRatio()
    {
        // Standard repitch calculation:
        // pitchRatio = frequency ratio * sample rate ratio
        // This gives traditional sampler behavior where pitch and speed change together
        pitchRatio = baseFrequencyRatio * baseSampleRateRatio;

        // Apply pitch wheel (+/- 2 semitones)
        pitchRatio *= std::pow(2.0, (currentPitchWheel - 8192) / 8192.0 / 6.0);

        // Note: Tempo sync is disabled for one-shot samples
        // One-shots should play at their natural pitch based on MIDI note
        // If you need tempo-synced loops, that would require time-stretching (different feature)
    }

    double pitchRatio = 1.0;
    double sourceSamplePosition = 0.0;
    double baseSampleRateRatio = 1.0;
    double baseFrequencyRatio = 1.0;
    float lgain = 0, rgain = 0;
    double hostBPM = 120.0;
    bool tempoSyncEnabled = false; // Disabled for one-shot samples
    int currentMidiNote = 60;
    int currentPitchWheel = 8192;

    juce::ADSR adsr;
};

/**
 * Sample synth with tempo sync support
 * Matches sample playback to DAW tempo
 */
class SampleSynth
{
public:
    SampleSynth()
    {
        // Register audio formats we can read
        formatManager.registerBasicFormats();

        // Add tempo-sync voices to the synthesiser
        for (int i = 0; i < 16; ++i)
            synth.addVoice(new TempoSyncSamplerVoice());
    }

    void prepare(double sampleRate, int samplesPerBlock)
    {
        synth.setCurrentPlaybackSampleRate(sampleRate);
        this->sampleRate = sampleRate;
    }

    /**
     * Set the host BPM for tempo sync
     */
    void setHostBPM(double bpm)
    {
        hostBPM = bpm;
        for (int i = 0; i < synth.getNumVoices(); ++i)
        {
            if (auto* voice = dynamic_cast<TempoSyncSamplerVoice*>(synth.getVoice(i)))
                voice->setHostBPM(bpm);
        }
    }

    /**
     * Enable/disable tempo sync
     */
    void setTempoSyncEnabled(bool enabled)
    {
        tempoSyncEnabled = enabled;
        for (int i = 0; i < synth.getNumVoices(); ++i)
        {
            if (auto* voice = dynamic_cast<TempoSyncSamplerVoice*>(synth.getVoice(i)))
                voice->setTempoSyncEnabled(enabled);
        }
    }

    /**
     * Set the original BPM of the sample (for tempo sync calculations)
     */
    void setOriginalBPM(double bpm)
    {
        originalBPM = bpm;
    }

    /**
     * Load a sample file and make it playable across all MIDI notes
     */
    bool loadSample(const juce::File& file)
    {
        // Clear any existing sounds
        synth.clearSounds();

        // Create a reader for the file
        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

        if (reader == nullptr)
        {
            DBG("Failed to create reader for: " + file.getFullPathName());
            return false;
        }

        DBG("Loading sample: " + file.getFullPathName());
        DBG("  Sample rate: " + juce::String(reader->sampleRate));
        DBG("  Length: " + juce::String(reader->lengthInSamples) + " samples");
        DBG("  Channels: " + juce::String(reader->numChannels));

        // Try to detect BPM from filename (common format: "SampleName_120BPM.wav")
        double detectedBPM = detectBPMFromFilename(file.getFileNameWithoutExtension());
        if (detectedBPM <= 0)
            detectedBPM = originalBPM; // Use default if not detected

        DBG("  Original BPM: " + juce::String(detectedBPM));

        // Create a BigInteger for which MIDI notes this sample responds to (all notes)
        juce::BigInteger allNotes;
        allNotes.setRange(0, 128, true);

        // Create the TempoSyncSamplerSound with BPM info
        synth.addSound(new TempoSyncSamplerSound(
            file.getFileNameWithoutExtension(),  // name
            *reader,                              // source reader
            allNotes,                             // notes this sample plays on
            60,                                   // root note (C4 - middle C)
            0.01,                                 // attack time in seconds
            0.1,                                  // release time in seconds
            30.0,                                 // max sample length in seconds
            detectedBPM                           // original BPM
        ));

        currentSampleFile = file;
        return true;
    }

    void clearSample()
    {
        synth.clearSounds();
        currentSampleFile = juce::File();
    }

    void noteOn(int midiChannel, int midiNote, float velocity)
    {
        synth.noteOn(midiChannel, midiNote, velocity);
    }

    void noteOff(int midiChannel, int midiNote, float velocity)
    {
        synth.noteOff(midiChannel, midiNote, velocity, true);
    }

    void allNotesOff()
    {
        synth.allNotesOff(0, true);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
    {
        synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    }

    bool hasSampleLoaded() const
    {
        return synth.getNumSounds() > 0;
    }

    juce::String getCurrentSampleName() const
    {
        return currentSampleFile.getFileNameWithoutExtension();
    }

    bool isTempoSyncEnabled() const { return tempoSyncEnabled; }
    double getHostBPM() const { return hostBPM; }
    double getOriginalBPM() const { return originalBPM; }

private:
    /**
     * Try to detect BPM from filename
     * Looks for patterns like: "120BPM", "120_bpm", "120 bpm", "_120_"
     */
    double detectBPMFromFilename(const juce::String& filename)
    {
        // Common patterns: "120BPM", "120bpm", "120_bpm", "bpm120"
        juce::String lower = filename.toLowerCase();

        // Look for "XXXbpm" pattern
        int bpmIndex = lower.indexOf("bpm");
        if (bpmIndex > 0)
        {
            // Check for digits before "bpm"
            juce::String beforeBPM = filename.substring(0, bpmIndex);
            int numStart = beforeBPM.length() - 1;
            while (numStart >= 0 && juce::CharacterFunctions::isDigit(beforeBPM[numStart]))
                numStart--;
            numStart++;

            if (numStart < beforeBPM.length())
            {
                double bpm = beforeBPM.substring(numStart).getDoubleValue();
                if (bpm >= 60 && bpm <= 200)
                    return bpm;
            }
        }

        // Look for "bpmXXX" pattern
        if (bpmIndex >= 0 && bpmIndex + 3 < lower.length())
        {
            juce::String afterBPM = filename.substring(bpmIndex + 3);
            int numEnd = 0;
            while (numEnd < afterBPM.length() && juce::CharacterFunctions::isDigit(afterBPM[numEnd]))
                numEnd++;

            if (numEnd > 0)
            {
                double bpm = afterBPM.substring(0, numEnd).getDoubleValue();
                if (bpm >= 60 && bpm <= 200)
                    return bpm;
            }
        }

        // Look for standalone 3-digit number that could be BPM (60-200 range)
        for (int i = 0; i < filename.length() - 2; ++i)
        {
            if (juce::CharacterFunctions::isDigit(filename[i]))
            {
                int numEnd = i;
                while (numEnd < filename.length() && juce::CharacterFunctions::isDigit(filename[numEnd]))
                    numEnd++;

                juce::String numStr = filename.substring(i, numEnd);
                double num = numStr.getDoubleValue();
                if (num >= 60 && num <= 200)
                    return num;

                i = numEnd;
            }
        }

        return 0; // Not detected
    }

    juce::Synthesiser synth;
    juce::AudioFormatManager formatManager;
    double sampleRate = 44100.0;
    double hostBPM = 120.0;
    double originalBPM = 120.0;
    bool tempoSyncEnabled = true;
    juce::File currentSampleFile;
};

} // namespace Engine
} // namespace NulyBeats
