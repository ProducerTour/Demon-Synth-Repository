#pragma once

#include <JuceHeader.h>
#include <cmath>

namespace NulyBeats {
namespace DSP {

/**
 * Low Frequency Oscillator with:
 * - Multiple waveforms
 * - Tempo sync
 * - Phase offset
 * - Fade-in
 * - Bipolar and unipolar output
 */
class LFO
{
public:
    enum class Waveform
    {
        Sine,
        Triangle,
        Saw,
        ReverseSaw,
        Square,
        SampleAndHold,
        SmoothRandom
    };

    enum class SyncMode
    {
        Free,       // Hz
        Tempo       // Beat divisions
    };

    LFO() = default;

    void prepare(double sampleRate)
    {
        this->sampleRate = sampleRate;
        random.setSeed(juce::Time::currentTimeMillis());
    }

    void setRate(float rate)
    {
        if (syncMode == SyncMode::Free)
        {
            frequency = rate;
            phaseIncrement = frequency / sampleRate;
        }
    }

    void setTempoSync(double bpm, float beatDivision)
    {
        // beatDivision: 1 = quarter note, 0.5 = eighth, 2 = half, etc.
        syncMode = SyncMode::Tempo;
        frequency = static_cast<float>(bpm / 60.0 / beatDivision);
        phaseIncrement = frequency / sampleRate;
    }

    void setWaveform(Waveform wf) { waveform = wf; }
    void setPhaseOffset(float offset) { phaseOffset = offset; }
    void setBipolar(bool bp) { bipolar = bp; }

    void setFadeIn(float fadeTimeSeconds)
    {
        if (fadeTimeSeconds > 0.0f)
            fadeInRate = 1.0f / (fadeTimeSeconds * static_cast<float>(sampleRate));
        else
            fadeInRate = 1.0f;
        fadeLevel = 0.0f;
    }

    float process()
    {
        float output = 0.0f;
        float effectivePhase = std::fmod(phase + phaseOffset, 1.0f);
        if (effectivePhase < 0.0f)
            effectivePhase += 1.0f;

        switch (waveform)
        {
            case Waveform::Sine:
                output = std::sin(effectivePhase * juce::MathConstants<float>::twoPi);
                break;

            case Waveform::Triangle:
                output = 4.0f * std::abs(effectivePhase - 0.5f) - 1.0f;
                break;

            case Waveform::Saw:
                output = 2.0f * effectivePhase - 1.0f;
                break;

            case Waveform::ReverseSaw:
                output = 1.0f - 2.0f * effectivePhase;
                break;

            case Waveform::Square:
                output = effectivePhase < 0.5f ? 1.0f : -1.0f;
                break;

            case Waveform::SampleAndHold:
                // New random value at start of each cycle
                if (effectivePhase < lastPhase)
                    holdValue = random.nextFloat() * 2.0f - 1.0f;
                output = holdValue;
                break;

            case Waveform::SmoothRandom:
                // Interpolated random
                if (effectivePhase < lastPhase)
                {
                    prevRandomValue = nextRandomValue;
                    nextRandomValue = random.nextFloat() * 2.0f - 1.0f;
                }
                // Smooth interpolation
                output = prevRandomValue + effectivePhase * (nextRandomValue - prevRandomValue);
                break;
        }

        lastPhase = effectivePhase;

        // Apply fade-in
        if (fadeLevel < 1.0f)
        {
            fadeLevel = std::min(1.0f, fadeLevel + fadeInRate);
        }
        output *= fadeLevel;

        // Advance phase
        phase += static_cast<float>(phaseIncrement);
        while (phase >= 1.0f)
            phase -= 1.0f;

        // Convert to unipolar if needed
        if (!bipolar)
            output = (output + 1.0f) * 0.5f;

        return output;
    }

    void reset()
    {
        phase = 0.0f;
        fadeLevel = 0.0f;
        holdValue = 0.0f;
        prevRandomValue = 0.0f;
        nextRandomValue = random.nextFloat() * 2.0f - 1.0f;
    }

    void retrigger()
    {
        phase = 0.0f;
        fadeLevel = 0.0f;
    }

    float getPhase() const { return phase; }

private:
    double sampleRate = 44100.0;
    float frequency = 1.0f;
    double phaseIncrement = 0.0;
    float phase = 0.0f;
    float phaseOffset = 0.0f;
    float lastPhase = 0.0f;

    Waveform waveform = Waveform::Sine;
    SyncMode syncMode = SyncMode::Free;
    bool bipolar = true;

    float fadeLevel = 1.0f;
    float fadeInRate = 1.0f;

    // For S&H and smooth random
    float holdValue = 0.0f;
    float prevRandomValue = 0.0f;
    float nextRandomValue = 0.0f;
    juce::Random random;
};

} // namespace DSP
} // namespace NulyBeats
