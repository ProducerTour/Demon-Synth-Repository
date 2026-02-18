#pragma once

#include <JuceHeader.h>
#include "../../DSP/Oscillators/Oscillator.h"
#include "../../DSP/Oscillators/WavetableOscillator.h"
#include "../../DSP/Filters/SVFFilter.h"
#include "../../DSP/Modulators/ADSR.h"
#include "../../DSP/Modulators/LFO.h"
#include "../../Modulation/ModMatrix.h"
#include "../PCM/SamplePlayer.h"

namespace NulyBeats {
namespace Engine {

/**
 * Complete synth voice combining:
 * - Dual oscillators (VA or Wavetable)
 * - PCM sample layer
 * - Noise generator
 * - Multi-mode filter
 * - ADSR envelopes (Amp, Filter, Mod)
 * - LFOs
 * - Per-voice modulation
 */
class SynthVoice
{
public:
    struct Parameters
    {
        // Oscillator 1
        bool osc1Enabled = true;
        DSP::Oscillator::Waveform osc1Wave = DSP::Oscillator::Waveform::Saw;
        float osc1Level = 1.0f;
        float osc1Octave = 0.0f;
        float osc1Semi = 0.0f;
        float osc1Fine = 0.0f;
        float osc1PulseWidth = 0.5f;
        float osc1Pan = 0.0f;

        // Oscillator 2
        bool osc2Enabled = false;
        DSP::Oscillator::Waveform osc2Wave = DSP::Oscillator::Waveform::Square;
        float osc2Level = 1.0f;
        float osc2Octave = 0.0f;
        float osc2Semi = 0.0f;
        float osc2Fine = 0.0f;
        float osc2PulseWidth = 0.5f;
        float osc2Pan = 0.0f;

        // Noise
        float noiseLevel = 0.0f;

        // Filter
        DSP::SVFFilter::Type filterType = DSP::SVFFilter::Type::LowPass;
        float filterCutoff = 20000.0f;
        float filterResonance = 0.0f;
        float filterEnvAmount = 0.0f;
        float filterKeyTrack = 0.0f;

        // Amp Envelope
        float ampAttack = 0.01f;
        float ampDecay = 0.1f;
        float ampSustain = 0.7f;
        float ampRelease = 0.3f;
        float ampAttackCurve = -3.0f;   // Negative = exponential, 0 = linear, positive = logarithmic
        float ampDecayCurve = 3.0f;
        float ampReleaseCurve = 3.0f;

        // Filter Envelope
        float filterAttack = 0.01f;
        float filterDecay = 0.2f;
        float filterSustain = 0.5f;
        float filterRelease = 0.3f;

        // Mod Envelope
        float modAttack = 0.01f;
        float modDecay = 0.5f;
        float modSustain = 0.0f;
        float modRelease = 0.5f;

        // Glide/Portamento
        float glideTime = 0.0f;
        bool glideAlways = false;

        // Master
        float masterLevel = 1.0f;
    };

    SynthVoice() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        this->sampleRate = sampleRate;

        osc1.prepare(sampleRate, samplesPerBlock);
        osc2.prepare(sampleRate, samplesPerBlock);
        wavetableOsc1.prepare(sampleRate, samplesPerBlock);
        wavetableOsc2.prepare(sampleRate, samplesPerBlock);

        filter.prepare(sampleRate, samplesPerBlock);

        ampEnv.prepare(sampleRate);
        filterEnv.prepare(sampleRate);
        modEnv.prepare(sampleRate);

        lfo1.prepare(sampleRate);
        lfo2.prepare(sampleRate);

        modMatrix.prepare(sampleRate, samplesPerBlock);

        updateEnvelopes();
    }

    void noteOn(int midiNote, float velocity, bool legato = false, float fromFreq = 0.0f)
    {
        this->midiNote = midiNote;
        this->velocity = velocity;

        // Calculate target frequency
        float targetFreq = midiNoteToFrequency(midiNote);

        // For legato, start from the previous frequency if provided
        if (legato && fromFreq > 0.0f)
        {
            currentFreq = fromFreq;
        }

        if (params.glideTime > 0.0f && (params.glideAlways || isActive || legato))
        {
            // Portamento - calculate sample-accurate glide
            glideTarget = targetFreq;
            int glideSamples = static_cast<int>(params.glideTime * sampleRate);
            if (glideSamples > 0)
            {
                // Use exponential glide for more natural pitch slide
                glideRatio = std::pow(glideTarget / currentFreq, 1.0f / glideSamples);
            }
            else
            {
                currentFreq = targetFreq;
                glideRatio = 1.0f;
            }
        }
        else
        {
            currentFreq = targetFreq;
            glideTarget = targetFreq;
            glideRatio = 1.0f;
        }

        // Reset oscillators only on non-legato notes
        if (!isActive && !legato)
        {
            osc1.reset();
            osc2.reset();
            wavetableOsc1.reset();
            wavetableOsc2.reset();
        }

        // Trigger envelopes - only on non-legato or if not active
        if (!legato || !isActive)
        {
            ampEnv.noteOn(velocity);
            filterEnv.noteOn(velocity);
            modEnv.noteOn(velocity);

            // Retrigger LFOs if configured
            lfo1.retrigger();
            lfo2.retrigger();
        }

        isActive = true;

        // Set velocity in mod matrix
        modMatrix.setSourceValue(Modulation::ModSource::Velocity, velocity);

        // Key tracking
        float keyTrack = (midiNote - 60) / 127.0f; // Center on C4
        modMatrix.setSourceValue(Modulation::ModSource::KeyTrack, keyTrack);
    }

    float getCurrentFrequency() const { return currentFreq; }

    void noteOff()
    {
        ampEnv.noteOff();
        filterEnv.noteOff();
        modEnv.noteOff();
    }

    void process(float& left, float& right)
    {
        if (!isActive)
        {
            left = 0.0f;
            right = 0.0f;
            return;
        }

        // Update glide (exponential for natural pitch perception)
        if (currentFreq != glideTarget && glideRatio != 1.0f)
        {
            currentFreq *= glideRatio;
            // Check if we've reached the target (within 0.1 cent)
            float ratio = currentFreq / glideTarget;
            if (ratio > 0.9999f && ratio < 1.0001f)
            {
                currentFreq = glideTarget;
                glideRatio = 1.0f;
            }
        }

        // Process modulators
        float ampEnvValue = ampEnv.process();
        float filterEnvValue = filterEnv.process();
        float modEnvValue = modEnv.process();
        float lfo1Value = lfo1.process();
        float lfo2Value = lfo2.process();

        // Update mod matrix sources
        modMatrix.setSourceValue(Modulation::ModSource::AmpEnv, ampEnvValue);
        modMatrix.setSourceValue(Modulation::ModSource::FilterEnv, filterEnvValue);
        modMatrix.setSourceValue(Modulation::ModSource::ModEnv1, modEnvValue);
        modMatrix.setSourceValue(Modulation::ModSource::LFO1, lfo1Value);
        modMatrix.setSourceValue(Modulation::ModSource::LFO2, lfo2Value);

        modMatrix.process();

        // Get modulated values
        float pitchMod = modMatrix.getDestinationValue(Modulation::ModDest::Osc1Pitch);
        float cutoffMod = modMatrix.getDestinationValue(Modulation::ModDest::FilterCutoff);

        // Calculate modulated frequency
        float modFreq = currentFreq * std::pow(2.0f, pitchMod / 12.0f);

        // Generate oscillators
        float osc1Sample = 0.0f;
        float osc2Sample = 0.0f;

        if (params.osc1Enabled)
        {
            float osc1Freq = modFreq * std::pow(2.0f, params.osc1Octave + params.osc1Semi / 12.0f + params.osc1Fine / 1200.0f);
            osc1.setFrequency(osc1Freq);
            osc1.setWaveform(params.osc1Wave);
            osc1.setPulseWidth(params.osc1PulseWidth);
            osc1Sample = osc1.process() * params.osc1Level;
        }

        if (params.osc2Enabled)
        {
            float osc2Freq = modFreq * std::pow(2.0f, params.osc2Octave + params.osc2Semi / 12.0f + params.osc2Fine / 1200.0f);
            osc2.setFrequency(osc2Freq);
            osc2.setWaveform(params.osc2Wave);
            osc2.setPulseWidth(params.osc2PulseWidth);
            osc2Sample = osc2.process() * params.osc2Level;
        }

        // Noise
        float noiseSample = 0.0f;
        if (params.noiseLevel > 0.0f)
        {
            noiseSample = (random.nextFloat() * 2.0f - 1.0f) * params.noiseLevel;
        }

        // Mix oscillators and filter (mono)
        float mix = osc1Sample + osc2Sample + noiseSample;

        // Filter
        float filterCutoff = params.filterCutoff;
        filterCutoff += params.filterEnvAmount * filterEnvValue * 10000.0f;
        filterCutoff += params.filterKeyTrack * (midiNote - 60) * 100.0f;
        filterCutoff += cutoffMod * 5000.0f;
        filterCutoff = juce::jlimit(20.0f, 20000.0f, filterCutoff);

        filter.setCutoff(filterCutoff);
        filter.setResonance(params.filterResonance);
        filter.setType(params.filterType);

        mix = filter.process(mix);

        // Apply amp envelope and master level
        float gain = ampEnvValue * velocity * params.masterLevel;

        // Check if voice is done
        if (!ampEnv.isActive())
        {
            isActive = false;
            left = 0.0f;
            right = 0.0f;
            return;
        }

        // Per-oscillator stereo panning (equal-power)
        // Scale each osc's contribution from the filtered mix by its relative level
        float osc1Contrib = (osc1Sample + noiseSample) * gain;
        float osc2Contrib = osc2Sample * gain;

        float pan1L = std::cos((params.osc1Pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
        float pan1R = std::sin((params.osc1Pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
        float pan2L = std::cos((params.osc2Pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
        float pan2R = std::sin((params.osc2Pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);

        // When both pans are center (0), this equals the original mono behavior
        // When pans differ, oscillators spread in the stereo field
        float totalAmp = std::abs(osc1Sample + noiseSample) + std::abs(osc2Sample);
        if (totalAmp > 0.0001f)
        {
            float w1 = std::abs(osc1Sample + noiseSample) / totalAmp;
            float w2 = std::abs(osc2Sample) / totalAmp;
            float blendedPanL = pan1L * w1 + pan2L * w2;
            float blendedPanR = pan1R * w1 + pan2R * w2;
            left  = mix * gain * blendedPanL;
            right = mix * gain * blendedPanR;
        }
        else
        {
            left  = mix * gain * pan1L;
            right = mix * gain * pan1R;
        }
    }

    void processBlock(float* left, float* right, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            process(left[i], right[i]);
        }
    }

    bool isVoiceActive() const { return isActive; }
    int getMidiNote() const { return midiNote; }
    float getVelocity() const { return velocity; }

    void setParameters(const Parameters& p)
    {
        params = p;
        updateEnvelopes();
    }

    Parameters& getParameters() { return params; }
    Modulation::ModMatrix& getModMatrix() { return modMatrix; }

    void setLFOParams(DSP::LFO::Waveform lfo1Wave, float lfo1Rate,
                      DSP::LFO::Waveform lfo2Wave, float lfo2Rate)
    {
        lfo1.setWaveform(lfo1Wave);
        lfo1.setRate(lfo1Rate);
        lfo2.setWaveform(lfo2Wave);
        lfo2.setRate(lfo2Rate);
    }

    void reset()
    {
        isActive = false;
        osc1.reset();
        osc2.reset();
        wavetableOsc1.reset();
        wavetableOsc2.reset();
        filter.reset();
        ampEnv.reset();
        filterEnv.reset();
        modEnv.reset();
        lfo1.reset();
        lfo2.reset();
        modMatrix.reset();
    }

private:
    float midiNoteToFrequency(int note) const
    {
        return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    }

    void updateEnvelopes()
    {
        DSP::ADSR::Parameters ampParams;
        ampParams.attack = params.ampAttack;
        ampParams.decay = params.ampDecay;
        ampParams.sustain = params.ampSustain;
        ampParams.release = params.ampRelease;
        ampParams.attackCurve = params.ampAttackCurve;
        ampParams.decayCurve = params.ampDecayCurve;
        ampParams.releaseCurve = params.ampReleaseCurve;
        ampEnv.setParameters(ampParams);

        DSP::ADSR::Parameters filterParams;
        filterParams.attack = params.filterAttack;
        filterParams.decay = params.filterDecay;
        filterParams.sustain = params.filterSustain;
        filterParams.release = params.filterRelease;
        filterEnv.setParameters(filterParams);

        DSP::ADSR::Parameters modParams;
        modParams.attack = params.modAttack;
        modParams.decay = params.modDecay;
        modParams.sustain = params.modSustain;
        modParams.release = params.modRelease;
        modEnv.setParameters(modParams);
    }

    double sampleRate = 44100.0;

    // State
    bool isActive = false;
    int midiNote = -1;
    float velocity = 0.0f;
    float currentFreq = 440.0f;
    float glideTarget = 440.0f;
    float glideRatio = 1.0f;  // Exponential glide multiplier per sample

    // Oscillators
    DSP::Oscillator osc1, osc2;
    DSP::WavetableOscillator wavetableOsc1, wavetableOsc2;

    // Filter
    DSP::SVFFilter filter;

    // Envelopes
    DSP::ADSR ampEnv, filterEnv, modEnv;

    // LFOs
    DSP::LFO lfo1, lfo2;

    // Modulation
    Modulation::ModMatrix modMatrix;

    // Noise
    juce::Random random;

    // Parameters
    Parameters params;
};

} // namespace Engine
} // namespace NulyBeats
