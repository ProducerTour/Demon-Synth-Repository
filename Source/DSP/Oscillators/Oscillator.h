#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <array>

namespace NulyBeats {
namespace DSP {

/**
 * Base oscillator class with anti-aliased waveforms using PolyBLEP
 * Supports: Sine, Saw, Square, Triangle, Pulse with PWM
 */
class Oscillator
{
public:
    enum class Waveform
    {
        Sine,
        Saw,
        Square,
        Triangle,
        Pulse,
        Noise
    };

    Oscillator() = default;
    ~Oscillator() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        this->sampleRate = sampleRate;
        phaseIncrement = frequency / sampleRate;
    }

    void setFrequency(float freq)
    {
        frequency = freq;
        phaseIncrement = frequency / sampleRate;
    }

    void setWaveform(Waveform wf) { waveform = wf; }
    void setPulseWidth(float pw) { pulseWidth = juce::jlimit(0.01f, 0.99f, pw); }
    void setDetune(float cents) { detuneRatio = std::pow(2.0f, cents / 1200.0f); }

    float process()
    {
        float output = 0.0f;

        switch (waveform)
        {
            case Waveform::Sine:
                output = processSine();
                break;
            case Waveform::Saw:
                output = processSaw();
                break;
            case Waveform::Square:
                output = processSquare();
                break;
            case Waveform::Triangle:
                output = processTriangle();
                break;
            case Waveform::Pulse:
                output = processPulse();
                break;
            case Waveform::Noise:
                output = processNoise();
                break;
        }

        advancePhase();
        return output;
    }

    void reset()
    {
        phase = 0.0f;
    }

private:
    // Anti-aliasing using PolyBLEP
    float polyBlep(float t) const
    {
        float dt = static_cast<float>(phaseIncrement);

        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0f;
        }
        else if (t > 1.0f - dt)
        {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }

    float processSine() const
    {
        return std::sin(phase * juce::MathConstants<float>::twoPi);
    }

    float processSaw() const
    {
        float value = 2.0f * phase - 1.0f;
        value -= polyBlep(phase);
        return value;
    }

    float processSquare() const
    {
        float value = phase < 0.5f ? 1.0f : -1.0f;
        value += polyBlep(phase);
        value -= polyBlep(std::fmod(phase + 0.5f, 1.0f));
        return value;
    }

    float processTriangle() const
    {
        // Integrated square wave
        float value = processSquare();
        // Leaky integrator for triangle
        triangleIntegrator = 0.999f * triangleIntegrator + value * static_cast<float>(phaseIncrement) * 4.0f;
        return triangleIntegrator;
    }

    float processPulse() const
    {
        float value = phase < pulseWidth ? 1.0f : -1.0f;
        value += polyBlep(phase);
        value -= polyBlep(std::fmod(phase + (1.0f - pulseWidth), 1.0f));
        return value;
    }

    float processNoise() const
    {
        return random.nextFloat() * 2.0f - 1.0f;
    }

    void advancePhase()
    {
        phase += static_cast<float>(phaseIncrement * detuneRatio);
        while (phase >= 1.0f)
            phase -= 1.0f;
    }

    double sampleRate = 44100.0;
    float frequency = 440.0f;
    double phaseIncrement = 0.0;
    float phase = 0.0f;
    float pulseWidth = 0.5f;
    float detuneRatio = 1.0f;
    Waveform waveform = Waveform::Saw;

    mutable float triangleIntegrator = 0.0f;
    mutable juce::Random random;
};

} // namespace DSP
} // namespace NulyBeats
