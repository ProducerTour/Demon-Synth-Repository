#pragma once

#include <JuceHeader.h>
#include <vector>
#include <complex>
#include <cmath>

namespace NulyBeats {
namespace Engine {

/**
 * Real-time time stretching using phase vocoder
 * Allows independent pitch and time control
 */
class TimeStretch
{
public:
    static constexpr int FFT_SIZE = 2048;
    static constexpr int HOP_SIZE = FFT_SIZE / 4;
    static constexpr int OVERLAP = 4;

    TimeStretch()
        : fft(static_cast<int>(std::log2(FFT_SIZE)))
    {
        inputBuffer.resize(FFT_SIZE);
        outputBuffer.resize(FFT_SIZE);
        fftData.resize(FFT_SIZE * 2);
        window.resize(FFT_SIZE);
        lastPhase.resize(FFT_SIZE / 2 + 1);
        sumPhase.resize(FFT_SIZE / 2 + 1);
        magnitude.resize(FFT_SIZE / 2 + 1);
        frequency.resize(FFT_SIZE / 2 + 1);

        // Hann window
        for (int i = 0; i < FFT_SIZE; ++i)
        {
            window[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / FFT_SIZE));
        }
    }

    void prepare(double sampleRate)
    {
        this->sampleRate = sampleRate;
        freqPerBin = sampleRate / FFT_SIZE;
        expectedPhaseDiff = 2.0 * juce::MathConstants<double>::pi * HOP_SIZE / FFT_SIZE;
        reset();
    }

    void setStretchFactor(float factor)
    {
        // 1.0 = normal, 2.0 = half speed, 0.5 = double speed
        stretchFactor = juce::jlimit(0.25f, 4.0f, factor);
        outputHopSize = static_cast<int>(HOP_SIZE * stretchFactor);
    }

    void setPitchShift(float semitones)
    {
        // Pitch shift in semitones
        pitchFactor = std::pow(2.0f, semitones / 12.0f);
    }

    void reset()
    {
        std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0f);
        std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0f);
        std::fill(lastPhase.begin(), lastPhase.end(), 0.0f);
        std::fill(sumPhase.begin(), sumPhase.end(), 0.0f);
        inputWritePos = 0;
        outputReadPos = 0;
    }

    void process(const float* input, float* output, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            // Accumulate input
            inputBuffer[inputWritePos++] = input[i];

            if (inputWritePos >= FFT_SIZE)
            {
                processFrame();
                inputWritePos = FFT_SIZE - HOP_SIZE;

                // Shift input buffer
                std::copy(inputBuffer.begin() + HOP_SIZE, inputBuffer.end(),
                         inputBuffer.begin());
            }

            // Output from overlap-add buffer
            output[i] = outputBuffer[outputReadPos];
            outputBuffer[outputReadPos] = 0.0f;
            ++outputReadPos;

            if (outputReadPos >= outputHopSize)
                outputReadPos = 0;
        }
    }

private:
    void processFrame()
    {
        // Apply window and prepare FFT input
        for (int i = 0; i < FFT_SIZE; ++i)
        {
            fftData[i] = inputBuffer[i] * window[i];
        }
        std::fill(fftData.begin() + FFT_SIZE, fftData.end(), 0.0f);

        // Forward FFT
        fft.performRealOnlyForwardTransform(fftData.data());

        // Analysis: extract magnitude and true frequency
        for (int k = 0; k <= FFT_SIZE / 2; ++k)
        {
            float real = fftData[k * 2];
            float imag = fftData[k * 2 + 1];

            magnitude[k] = std::sqrt(real * real + imag * imag);
            float phase = std::atan2(imag, real);

            // Calculate phase difference and true frequency
            float phaseDiff = phase - lastPhase[k];
            lastPhase[k] = phase;

            // Wrap phase difference to [-pi, pi]
            phaseDiff = wrapPhase(phaseDiff - k * expectedPhaseDiff);

            // True frequency
            frequency[k] = k * freqPerBin + phaseDiff * freqPerBin / expectedPhaseDiff;
        }

        // Synthesis with pitch shift
        std::fill(fftData.begin(), fftData.end(), 0.0f);

        for (int k = 0; k <= FFT_SIZE / 2; ++k)
        {
            // Destination bin with pitch shift
            int destBin = static_cast<int>(k * pitchFactor);
            if (destBin > FFT_SIZE / 2)
                continue;

            // Accumulate phase
            float newFreq = frequency[k] * pitchFactor;
            sumPhase[destBin] += (newFreq / freqPerBin - destBin) * expectedPhaseDiff + destBin * expectedPhaseDiff;

            // Convert back to complex
            fftData[destBin * 2] += magnitude[k] * std::cos(sumPhase[destBin]);
            fftData[destBin * 2 + 1] += magnitude[k] * std::sin(sumPhase[destBin]);
        }

        // Inverse FFT
        fft.performRealOnlyInverseTransform(fftData.data());

        // Overlap-add to output
        for (int i = 0; i < FFT_SIZE; ++i)
        {
            int outIndex = (outputReadPos + i) % (outputHopSize * OVERLAP);
            outputBuffer[i] += fftData[i] * window[i] / (FFT_SIZE * OVERLAP / 2);
        }
    }

    float wrapPhase(float phase) const
    {
        while (phase > juce::MathConstants<float>::pi)
            phase -= 2.0f * juce::MathConstants<float>::pi;
        while (phase < -juce::MathConstants<float>::pi)
            phase += 2.0f * juce::MathConstants<float>::pi;
        return phase;
    }

    double sampleRate = 44100.0;
    float stretchFactor = 1.0f;
    float pitchFactor = 1.0f;
    double freqPerBin = 0.0;
    double expectedPhaseDiff = 0.0;
    int outputHopSize = HOP_SIZE;

    juce::dsp::FFT fft;

    std::vector<float> inputBuffer;
    std::vector<float> outputBuffer;
    std::vector<float> fftData;
    std::vector<float> window;
    std::vector<float> lastPhase;
    std::vector<float> sumPhase;
    std::vector<float> magnitude;
    std::vector<float> frequency;

    int inputWritePos = 0;
    int outputReadPos = 0;
};

} // namespace Engine
} // namespace NulyBeats
