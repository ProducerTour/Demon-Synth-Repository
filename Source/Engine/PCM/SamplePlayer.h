#pragma once

#include <JuceHeader.h>
#include "SampleZone.h"
#include "../../DSP/Modulators/ADSR.h"
#include "../../DSP/Filters/SVFFilter.h"
#include <memory>
#include <array>

namespace NulyBeats {
namespace Engine {

/**
 * Sample player voice with:
 * - High-quality interpolation
 * - Loop with crossfade
 * - Per-voice filter and envelope
 */
class SamplePlayerVoice
{
public:
    SamplePlayerVoice() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        this->sampleRate = sampleRate;
        ampEnv.prepare(sampleRate);
        filter.prepare(sampleRate, samplesPerBlock);
    }

    void startNote(SampleZone* zone, int midiNote, float velocity)
    {
        if (zone == nullptr || zone->audioData.getNumSamples() == 0)
            return;

        currentZone = zone;
        this->velocity = velocity;

        // Calculate playback rate
        pitchRatio = zone->getPitchRatio(midiNote);
        pitchRatio *= sampleRate / zone->originalSampleRate; // Sample rate compensation

        // Reset playback position
        position = 0.0;
        isPlaying = true;

        // Configure envelope
        DSP::ADSR::Parameters envParams;
        envParams.attack = 0.005f;
        envParams.decay = 0.1f;
        envParams.sustain = 1.0f;
        envParams.release = 0.1f;
        ampEnv.setParameters(envParams);
        ampEnv.noteOn(velocity);

        // Configure filter
        filter.setCutoff(20000.0f);
        filter.setResonance(0.0f);
    }

    void stopNote(bool allowTailOff)
    {
        if (allowTailOff)
        {
            ampEnv.noteOff();
        }
        else
        {
            isPlaying = false;
            currentZone = nullptr;
        }
    }

    float process()
    {
        if (!isPlaying || currentZone == nullptr)
            return 0.0f;

        const auto& buffer = currentZone->audioData;
        int numSamples = buffer.getNumSamples();

        // Get interpolated sample
        float sample = getInterpolatedSample(buffer, position);

        // Advance position
        position += pitchRatio;

        // Handle looping
        if (currentZone->loopEnabled)
        {
            if (position >= currentZone->loopEnd)
            {
                if (currentZone->crossfadeLoop)
                {
                    // Crossfade looping for seamless loops
                    float loopLength = static_cast<float>(currentZone->loopEnd - currentZone->loopStart);
                    position = currentZone->loopStart + std::fmod(position - currentZone->loopStart, loopLength);
                }
                else
                {
                    position = currentZone->loopStart;
                }
            }
        }
        else
        {
            // Non-looping: check for end of sample
            if (position >= numSamples - 1)
            {
                isPlaying = false;
                currentZone = nullptr;
                return 0.0f;
            }
        }

        // Apply envelope
        float envValue = ampEnv.process();
        if (!ampEnv.isActive())
        {
            isPlaying = false;
            currentZone = nullptr;
            return 0.0f;
        }

        sample *= envValue * currentZone->getGain();

        // Apply filter
        sample = filter.process(sample);

        return sample;
    }

    void processBlock(float* outputL, float* outputR, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float sample = process();

            // Apply pan
            float pan = currentZone ? currentZone->pan : 0.0f;
            float leftGain = std::cos((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
            float rightGain = std::sin((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);

            outputL[i] += sample * leftGain;
            outputR[i] += sample * rightGain;
        }
    }

    bool isVoiceActive() const { return isPlaying; }
    int getCurrentNote() const { return currentNote; }

    void setFilterCutoff(float cutoff) { filter.setCutoff(cutoff); }
    void setFilterResonance(float res) { filter.setResonance(res); }

private:
    // Hermite interpolation for high-quality pitch shifting
    float getInterpolatedSample(const juce::AudioBuffer<float>& buffer, double pos) const
    {
        int numSamples = buffer.getNumSamples();
        int i0 = static_cast<int>(pos);

        if (i0 < 0 || i0 >= numSamples - 1)
            return 0.0f;

        float frac = static_cast<float>(pos - i0);

        // Get 4 samples for cubic interpolation
        int im1 = std::max(0, i0 - 1);
        int i1 = std::min(numSamples - 1, i0 + 1);
        int i2 = std::min(numSamples - 1, i0 + 2);

        const float* data = buffer.getReadPointer(0);
        float ym1 = data[im1];
        float y0 = data[i0];
        float y1 = data[i1];
        float y2 = data[i2];

        // Hermite interpolation
        float c0 = y0;
        float c1 = 0.5f * (y1 - ym1);
        float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
        float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    double sampleRate = 44100.0;
    SampleZone* currentZone = nullptr;

    double position = 0.0;
    float pitchRatio = 1.0f;
    float velocity = 1.0f;
    int currentNote = -1;
    bool isPlaying = false;

    DSP::ADSR ampEnv;
    DSP::SVFFilter filter;
};

/**
 * Polyphonic sample player managing multiple voices
 */
class SamplePlayer
{
public:
    static constexpr int MAX_VOICES = 64;

    SamplePlayer()
    {
        voices.resize(MAX_VOICES);
    }

    void prepare(double sampleRate, int samplesPerBlock)
    {
        this->sampleRate = sampleRate;
        for (auto& voice : voices)
            voice.prepare(sampleRate, samplesPerBlock);
    }

    void setInstrument(SampleInstrument* inst)
    {
        instrument = inst;
        activeVoiceCount = inst ? inst->polyphony : MAX_VOICES;
    }

    void noteOn(int midiNote, float velocity)
    {
        if (instrument == nullptr)
            return;

        // Find zone(s) to play
        for (const auto& layer : instrument->layers)
        {
            SampleZone* zone = layer->findZone(midiNote, static_cast<int>(velocity * 127), roundRobinCounter);
            if (zone != nullptr)
            {
                // Find free voice
                SamplePlayerVoice* voice = findFreeVoice();
                if (voice != nullptr)
                {
                    voice->startNote(zone, midiNote, velocity);
                }
            }
        }

        ++roundRobinCounter;
    }

    void noteOff(int midiNote, bool allowTailOff = true)
    {
        for (auto& voice : voices)
        {
            if (voice.isVoiceActive() && voice.getCurrentNote() == midiNote)
            {
                voice.stopNote(allowTailOff);
            }
        }
    }

    void allNotesOff()
    {
        for (auto& voice : voices)
            voice.stopNote(false);
    }

    void processBlock(juce::AudioBuffer<float>& buffer)
    {
        float* left = buffer.getWritePointer(0);
        float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : left;

        for (auto& voice : voices)
        {
            if (voice.isVoiceActive())
            {
                voice.processBlock(left, right, buffer.getNumSamples());
            }
        }
    }

private:
    SamplePlayerVoice* findFreeVoice()
    {
        // First, look for inactive voice
        for (int i = 0; i < activeVoiceCount; ++i)
        {
            if (!voices[i].isVoiceActive())
                return &voices[i];
        }

        // Voice stealing: find oldest voice
        // For now, just return first voice
        return &voices[0];
    }

    double sampleRate = 44100.0;
    std::vector<SamplePlayerVoice> voices;
    SampleInstrument* instrument = nullptr;
    int activeVoiceCount = MAX_VOICES;
    int roundRobinCounter = 0;
};

} // namespace Engine
} // namespace NulyBeats
