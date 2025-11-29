#pragma once

#include <JuceHeader.h>
#include <cmath>

namespace NulyBeats {
namespace DSP {

/**
 * High-quality ADSR envelope generator using EarLevel Engineering approach
 * with proper exponential curves via targetRatio coefficients.
 *
 * Based on: https://www.earlevel.com/main/2013/06/03/envelope-generators-adsr-code/
 *
 * Curve parameter:
 *   - Small values (0.0001 to 0.01) = mostly exponential (fast attack, slow decay)
 *   - Large values (10 to 100) = nearly linear
 *   - Default is 0.0001 for natural analog-style response
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
        float decay = 0.1f;     // seconds
        float sustain = 0.7f;   // 0-1
        float release = 0.3f;   // seconds

        // Curve controls (targetRatio): small = exponential, large = linear
        // Range: 0.0001 (very exponential) to 100.0 (nearly linear)
        float attackCurve = 0.3f;    // Attack curve (0.0001 = instant punch, 100 = linear fade)
        float decayCurve = 0.0001f;  // Decay curve (0.0001 = fast drop to sustain)
        float releaseCurve = 0.0001f; // Release curve (0.0001 = natural fade out)
    };

    ADSR() = default;

    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        reset();
        calculateCoefficients();
    }

    void setParameters(const Parameters& newParams)
    {
        params = newParams;
        calculateCoefficients();
    }

    void noteOn(float vel = 1.0f)
    {
        velocity = vel;
        state = State::Attack;
    }

    void noteOff()
    {
        if (state != State::Idle)
        {
            state = State::Release;
        }
    }

    float process()
    {
        switch (state)
        {
            case State::Idle:
                break;

            case State::Attack:
                output = attackBase + output * attackCoef;
                if (output >= 1.0f)
                {
                    output = 1.0f;
                    state = State::Decay;
                }
                break;

            case State::Decay:
                output = decayBase + output * decayCoef;
                if (output <= params.sustain)
                {
                    output = params.sustain;
                    state = State::Sustain;
                }
                break;

            case State::Sustain:
                output = params.sustain;
                break;

            case State::Release:
                output = releaseBase + output * releaseCoef;
                if (output <= 0.0001f)
                {
                    output = 0.0f;
                    state = State::Idle;
                }
                break;
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
    // Calculate coefficient for exponential curve
    // targetRatio: small = more exponential, large = more linear
    float calcCoef(float rate, float targetRatio) const
    {
        if (rate <= 0.0f)
            return 0.0f;
        return std::exp(-std::log((1.0f + targetRatio) / targetRatio) / rate);
    }

    void calculateCoefficients()
    {
        // Convert curve parameters from UI range to targetRatio
        // UI sends -6 to 6, we need to map to 0.0001 to 100
        // For attack: negative curve = more exponential (small targetRatio)
        // For decay/release: positive curve = more exponential (small targetRatio)

        float attackTargetRatio = convertCurveToTargetRatio(params.attackCurve, true);
        float decayTargetRatio = convertCurveToTargetRatio(params.decayCurve, false);
        float releaseTargetRatio = convertCurveToTargetRatio(params.releaseCurve, false);

        // Attack: from current level to 1.0
        float attackRate = params.attack * static_cast<float>(sampleRate);
        attackCoef = calcCoef(attackRate, attackTargetRatio);
        attackBase = (1.0f + attackTargetRatio) * (1.0f - attackCoef);

        // Decay: from 1.0 to sustain level
        float decayRate = params.decay * static_cast<float>(sampleRate);
        decayCoef = calcCoef(decayRate, decayTargetRatio);
        decayBase = (params.sustain - decayTargetRatio) * (1.0f - decayCoef);

        // Release: from sustain to 0
        float releaseRate = params.release * static_cast<float>(sampleRate);
        releaseCoef = calcCoef(releaseRate, releaseTargetRatio);
        releaseBase = -releaseTargetRatio * (1.0f - releaseCoef);
    }

    // Convert UI curve value (-6 to 6) to targetRatio (0.0001 to 100)
    float convertCurveToTargetRatio(float curve, bool isAttack) const
    {
        // Clamp curve to valid range
        curve = juce::jlimit(-6.0f, 6.0f, curve);

        if (isAttack)
        {
            // For attack: negative = more exponential (punchy), positive = more linear
            // Map -6 to 0.0001 (very exponential), 6 to 100 (linear)
            float normalized = (curve + 6.0f) / 12.0f; // 0 to 1
            return 0.0001f * std::pow(1000000.0f, normalized); // 0.0001 to 100
        }
        else
        {
            // For decay/release: positive = more exponential, negative = more linear
            // Map 6 to 0.0001 (very exponential), -6 to 100 (linear)
            float normalized = (-curve + 6.0f) / 12.0f; // 0 to 1
            return 0.0001f * std::pow(1000000.0f, normalized); // 0.0001 to 100
        }
    }

    double sampleRate = 44100.0;
    Parameters params;

    State state = State::Idle;
    float output = 0.0f;
    float velocity = 1.0f;

    // Coefficients for exponential envelope
    float attackCoef = 0.0f;
    float attackBase = 0.0f;
    float decayCoef = 0.0f;
    float decayBase = 0.0f;
    float releaseCoef = 0.0f;
    float releaseBase = 0.0f;

    bool legato = false;
};

} // namespace DSP
} // namespace NulyBeats
