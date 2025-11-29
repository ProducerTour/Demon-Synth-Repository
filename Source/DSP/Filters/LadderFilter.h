#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <array>

namespace NulyBeats {
namespace DSP {

/**
 * Moog-style Ladder Filter using Zero-Delay Feedback (ZDF)
 * 4-pole (24dB/oct) with resonance up to self-oscillation
 * Based on Välimäki/Smith improved nonlinear model
 */
class LadderFilter
{
public:
    enum class Slope
    {
        Slope6dB,   // 1-pole
        Slope12dB,  // 2-pole
        Slope18dB,  // 3-pole
        Slope24dB   // 4-pole (classic Moog)
    };

    LadderFilter() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        this->sampleRate = sampleRate;
        reset();
        updateCoefficients();
    }

    void setCutoff(float freq)
    {
        cutoffFreq = juce::jlimit(20.0f, static_cast<float>(sampleRate * 0.49), freq);
        updateCoefficients();
    }

    void setResonance(float res)
    {
        // 0-1 range, where 1 = strong self-oscillation
        resonance = juce::jlimit(0.0f, 1.0f, res);
        updateCoefficients();
    }

    void setDrive(float driveAmount)
    {
        // Soft saturation amount
        drive = juce::jlimit(1.0f, 10.0f, driveAmount);
    }

    void setSlope(Slope newSlope)
    {
        slope = newSlope;
    }

    float process(float input)
    {
        // Apply input drive/saturation
        float x = std::tanh(input * drive);

        // Feedback with resonance
        float feedback = resonance * 4.0f * (stage[3] - x * 0.5f);

        // First stage with feedback
        float u = (x - feedback) * gComp;
        stage[0] = processStage(u, state[0]);

        // Cascade through remaining stages
        for (int i = 1; i < 4; ++i)
            stage[i] = processStage(stage[i-1], state[i]);

        // Output based on slope setting
        switch (slope)
        {
            case Slope::Slope6dB:
                return stage[0];
            case Slope::Slope12dB:
                return stage[1];
            case Slope::Slope18dB:
                return stage[2];
            case Slope::Slope24dB:
            default:
                return stage[3];
        }
    }

    void processBlock(float* samples, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
            samples[i] = process(samples[i]);
    }

    void reset()
    {
        state.fill(0.0f);
        stage.fill(0.0f);
    }

private:
    float processStage(float input, float& stateVar)
    {
        // One-pole lowpass with ZDF
        float v = (input - stateVar) * g / (1.0f + g);
        float output = v + stateVar;
        stateVar = output + v;
        return output;
    }

    void updateCoefficients()
    {
        // Warped frequency for ZDF
        g = std::tan(juce::MathConstants<float>::pi * cutoffFreq / static_cast<float>(sampleRate));

        // Gain compensation for resonance
        gComp = 1.0f / (1.0f + resonance * 4.0f * g * g * g * g / std::pow(1.0f + g, 4.0f));
    }

    double sampleRate = 44100.0;
    float cutoffFreq = 1000.0f;
    float resonance = 0.0f;
    float drive = 1.0f;
    Slope slope = Slope::Slope24dB;

    float g = 0.0f;
    float gComp = 1.0f;

    std::array<float, 4> state{};
    std::array<float, 4> stage{};
};

} // namespace DSP
} // namespace NulyBeats
