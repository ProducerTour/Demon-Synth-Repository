#pragma once

#include <JuceHeader.h>
#include <cmath>

namespace NulyBeats {
namespace DSP {

/**
 * High-quality ADSR envelope generator with:
 * - Exponential curves with adjustable curvature
 * - Retrigger and legato modes
 * - Sample-accurate timing
 */
class ADSR
{
public:
    enum class State
    {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };

    struct Parameters
    {
        float attack = 0.01f;   // seconds
        float decay = 0.1f;    // seconds
        float sustain = 0.7f;  // 0-1
        float release = 0.3f;  // seconds

        float attackCurve = -3.0f;  // Negative = exponential, 0 = linear, positive = log
        float decayCurve = -3.0f;
        float releaseCurve = -3.0f;
    };

    ADSR() = default;

    void prepare(double sampleRate)
    {
        this->sampleRate = sampleRate;
        calculateCoefficients();
    }

    void setParameters(const Parameters& newParams)
    {
        params = newParams;
        calculateCoefficients();
    }

    void noteOn(float velocity = 1.0f)
    {
        this->velocity = velocity;

        if (state == State::Idle || !legato)
        {
            // Fresh attack from current output level
            attackBase = output;
        }

        state = State::Attack;
    }

    void noteOff()
    {
        if (state != State::Idle)
        {
            releaseStart = output;
            state = State::Release;
        }
    }

    float process()
    {
        switch (state)
        {
            case State::Idle:
                output = 0.0f;
                break;

            case State::Attack:
            {
                output += attackRate;
                if (output >= 1.0f)
                {
                    output = 1.0f;
                    state = State::Decay;
                }
                // Apply curve
                float curved = applyCurve(output, params.attackCurve);
                return curved * velocity;
            }

            case State::Decay:
            {
                output -= decayRate;
                if (output <= params.sustain)
                {
                    output = params.sustain;
                    state = State::Sustain;
                }
                // Curved decay
                float decayProgress = (1.0f - output) / (1.0f - params.sustain);
                float curved = 1.0f - applyCurve(decayProgress, params.decayCurve) * (1.0f - params.sustain);
                return curved * velocity;
            }

            case State::Sustain:
                return params.sustain * velocity;

            case State::Release:
            {
                output -= releaseRate;
                if (output <= 0.0f)
                {
                    output = 0.0f;
                    state = State::Idle;
                    return 0.0f;
                }
                // Curved release from wherever we started
                float releaseProgress = 1.0f - (output / releaseStart);
                float curved = releaseStart * (1.0f - applyCurve(releaseProgress, params.releaseCurve));
                return curved * velocity;
            }
        }

        return output * velocity;
    }

    void processBlock(float* buffer, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
            buffer[i] = process();
    }

    void reset()
    {
        state = State::Idle;
        output = 0.0f;
    }

    bool isActive() const { return state != State::Idle; }
    State getState() const { return state; }
    float getOutput() const { return output * velocity; }

    void setLegato(bool enabled) { legato = enabled; }

private:
    float applyCurve(float x, float curve) const
    {
        if (std::abs(curve) < 0.01f)
            return x; // Linear

        // Attempt to create exponential/logarithmic curve
        if (curve < 0)
        {
            // Exponential (fast start, slow end)
            return (1.0f - std::exp(curve * x)) / (1.0f - std::exp(curve));
        }
        else
        {
            // Logarithmic (slow start, fast end)
            return std::log(1.0f + (std::exp(curve) - 1.0f) * x) / curve;
        }
    }

    void calculateCoefficients()
    {
        // Rate = 1 / (time * sampleRate)
        attackRate = params.attack > 0.0f ? 1.0f / (params.attack * static_cast<float>(sampleRate)) : 1.0f;
        decayRate = params.decay > 0.0f ? (1.0f - params.sustain) / (params.decay * static_cast<float>(sampleRate)) : 1.0f;
        releaseRate = params.release > 0.0f ? 1.0f / (params.release * static_cast<float>(sampleRate)) : 1.0f;
    }

    double sampleRate = 44100.0;
    Parameters params;

    State state = State::Idle;
    float output = 0.0f;
    float velocity = 1.0f;

    float attackRate = 0.0f;
    float decayRate = 0.0f;
    float releaseRate = 0.0f;

    float attackBase = 0.0f;
    float releaseStart = 0.0f;

    bool legato = false;
};

} // namespace DSP
} // namespace NulyBeats
