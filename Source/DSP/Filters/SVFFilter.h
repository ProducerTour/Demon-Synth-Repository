#pragma once

#include <JuceHeader.h>
#include <cmath>

namespace NulyBeats {
namespace DSP {

/**
 * State Variable Filter (SVF) using Topology-Preserving Transform (TPT)
 * Zero-delay feedback filter with simultaneous LP/BP/HP/Notch outputs
 * Based on Vadim Zavalishin's "The Art of VA Filter Design"
 */
class SVFFilter
{
public:
    enum class Type
    {
        LowPass,
        HighPass,
        BandPass,
        Notch,
        Peak,
        LowShelf,
        HighShelf
    };

    SVFFilter() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        this->sampleRate = sampleRate;
        reset();
        updateCoefficients();
    }

    void setType(Type newType)
    {
        type = newType;
    }

    void setCutoff(float freq)
    {
        cutoffFreq = juce::jlimit(20.0f, static_cast<float>(sampleRate * 0.49), freq);
        updateCoefficients();
    }

    void setResonance(float res)
    {
        // Resonance 0-1, where 1 = self-oscillation
        resonance = juce::jlimit(0.0f, 1.0f, res);
        updateCoefficients();
    }

    void setGain(float gainDb)
    {
        gain = std::pow(10.0f, gainDb / 20.0f);
        updateCoefficients();
    }

    float process(float input)
    {
        // TPT SVF implementation
        float v3 = input - ic2eq;
        float v1 = a1 * ic1eq + a2 * v3;
        float v2 = ic2eq + a2 * ic1eq + a3 * v3;

        ic1eq = 2.0f * v1 - ic1eq;
        ic2eq = 2.0f * v2 - ic2eq;

        // Output mixing based on filter type
        switch (type)
        {
            case Type::LowPass:
                return v2;
            case Type::HighPass:
                return input - k * v1 - v2;
            case Type::BandPass:
                return v1;
            case Type::Notch:
                return input - k * v1;
            case Type::Peak:
                return input - k * v1 + v2 * (gain - 1.0f);
            case Type::LowShelf:
                return input + v2 * (gain - 1.0f);
            case Type::HighShelf:
                return input + (input - k * v1 - v2) * (gain - 1.0f);
            default:
                return v2;
        }
    }

    void processBlock(float* samples, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
            samples[i] = process(samples[i]);
    }

    // Get simultaneous outputs (useful for multimode filter)
    struct FilterOutputs
    {
        float lowpass;
        float highpass;
        float bandpass;
        float notch;
    };

    FilterOutputs processMultimode(float input)
    {
        float v3 = input - ic2eq;
        float v1 = a1 * ic1eq + a2 * v3;
        float v2 = ic2eq + a2 * ic1eq + a3 * v3;

        ic1eq = 2.0f * v1 - ic1eq;
        ic2eq = 2.0f * v2 - ic2eq;

        FilterOutputs out;
        out.lowpass = v2;
        out.bandpass = v1;
        out.highpass = input - k * v1 - v2;
        out.notch = input - k * v1;

        return out;
    }

    void reset()
    {
        ic1eq = 0.0f;
        ic2eq = 0.0f;
    }

private:
    void updateCoefficients()
    {
        float g = std::tan(juce::MathConstants<float>::pi * cutoffFreq / static_cast<float>(sampleRate));

        // Q from resonance (avoiding division by zero near self-oscillation)
        float Q = 1.0f / (2.0f * (1.0f - resonance * 0.99f));
        k = 1.0f / Q;

        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    double sampleRate = 44100.0;
    float cutoffFreq = 1000.0f;
    float resonance = 0.0f;
    float gain = 1.0f;
    Type type = Type::LowPass;

    // Coefficients
    float g = 0.0f;
    float k = 1.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
    float a3 = 0.0f;

    // State
    float ic1eq = 0.0f;
    float ic2eq = 0.0f;
};

} // namespace DSP
} // namespace NulyBeats
