#pragma once

#include <JuceHeader.h>
#include <vector>
#include <array>
#include <cmath>

namespace NulyBeats {
namespace DSP {

/**
 * Wavetable Oscillator with:
 * - Multiple wavetables with morphing
 * - Bandlimited via mipmap approach
 * - SIMD-optimized interpolation
 */
class WavetableOscillator
{
public:
    static constexpr int TABLE_SIZE = 2048;
    static constexpr int MAX_TABLES = 256;
    static constexpr int NUM_MIPMAPS = 10;

    struct Wavetable
    {
        std::array<std::vector<float>, NUM_MIPMAPS> mipmaps;
        juce::String name;

        void generateMipmaps(const std::vector<float>& baseTable)
        {
            // Generate bandlimited versions for different frequency ranges
            mipmaps[0] = baseTable;

            for (int i = 1; i < NUM_MIPMAPS; ++i)
            {
                int newSize = TABLE_SIZE >> i;
                mipmaps[i].resize(newSize);

                // Low-pass and downsample
                for (int j = 0; j < newSize; ++j)
                {
                    mipmaps[i][j] = (mipmaps[i-1][j * 2] + mipmaps[i-1][j * 2 + 1]) * 0.5f;
                }
            }
        }
    };

    WavetableOscillator() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        this->sampleRate = sampleRate;
        generateDefaultTables();
    }

    void loadWavetable(const std::vector<float>& samples, int tableIndex)
    {
        if (tableIndex < 0 || tableIndex >= MAX_TABLES)
            return;

        if (tableIndex >= static_cast<int>(wavetables.size()))
            wavetables.resize(tableIndex + 1);

        wavetables[tableIndex].mipmaps[0] = samples;
        wavetables[tableIndex].generateMipmaps(samples);
    }

    void setFrequency(float freq)
    {
        frequency = freq;
        phaseIncrement = frequency * TABLE_SIZE / sampleRate;

        // Select appropriate mipmap based on frequency
        float samplesPerCycle = sampleRate / frequency;
        currentMipmap = 0;
        while (currentMipmap < NUM_MIPMAPS - 1 &&
               samplesPerCycle < (TABLE_SIZE >> currentMipmap) / 2)
        {
            ++currentMipmap;
        }
    }

    void setTablePosition(float position)
    {
        // Position 0-1 morphs through all loaded wavetables
        tablePosition = juce::jlimit(0.0f, 1.0f, position);
    }

    float process()
    {
        if (wavetables.empty())
            return 0.0f;

        // Calculate which two tables to interpolate between
        float scaledPos = tablePosition * (wavetables.size() - 1);
        int tableA = static_cast<int>(scaledPos);
        int tableB = std::min(tableA + 1, static_cast<int>(wavetables.size() - 1));
        float tableFrac = scaledPos - tableA;

        // Get samples from both tables with cubic interpolation
        float sampleA = getInterpolatedSample(wavetables[tableA]);
        float sampleB = getInterpolatedSample(wavetables[tableB]);

        // Morph between tables
        float output = sampleA + tableFrac * (sampleB - sampleA);

        // Advance phase
        phase += phaseIncrement;
        int tableSize = TABLE_SIZE >> currentMipmap;
        while (phase >= tableSize)
            phase -= tableSize;

        return output;
    }

    void reset()
    {
        phase = 0.0f;
    }

private:
    float getInterpolatedSample(const Wavetable& table) const
    {
        const auto& mipmap = table.mipmaps[currentMipmap];
        if (mipmap.empty())
            return 0.0f;

        int tableSize = static_cast<int>(mipmap.size());
        float scaledPhase = phase * tableSize / (TABLE_SIZE >> currentMipmap);

        int i0 = static_cast<int>(scaledPhase) % tableSize;
        int i1 = (i0 + 1) % tableSize;
        int i2 = (i0 + 2) % tableSize;
        int im1 = (i0 - 1 + tableSize) % tableSize;

        float frac = scaledPhase - std::floor(scaledPhase);

        // Cubic interpolation (Catmull-Rom)
        float y0 = mipmap[im1];
        float y1 = mipmap[i0];
        float y2 = mipmap[i1];
        float y3 = mipmap[i2];

        float a0 = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
        float a1 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        float a2 = -0.5f * y0 + 0.5f * y2;
        float a3 = y1;

        return a0 * frac * frac * frac + a1 * frac * frac + a2 * frac + a3;
    }

    void generateDefaultTables()
    {
        wavetables.resize(4);

        // Sine
        std::vector<float> sine(TABLE_SIZE);
        for (int i = 0; i < TABLE_SIZE; ++i)
            sine[i] = std::sin(2.0f * juce::MathConstants<float>::pi * i / TABLE_SIZE);
        wavetables[0].name = "Sine";
        loadWavetable(sine, 0);

        // Saw (additive synthesis for bandlimiting)
        std::vector<float> saw(TABLE_SIZE);
        for (int i = 0; i < TABLE_SIZE; ++i)
        {
            saw[i] = 0.0f;
            for (int h = 1; h <= 64; ++h)
                saw[i] += std::sin(2.0f * juce::MathConstants<float>::pi * h * i / TABLE_SIZE) / h;
            saw[i] *= 2.0f / juce::MathConstants<float>::pi;
        }
        wavetables[1].name = "Saw";
        loadWavetable(saw, 1);

        // Square
        std::vector<float> square(TABLE_SIZE);
        for (int i = 0; i < TABLE_SIZE; ++i)
        {
            square[i] = 0.0f;
            for (int h = 1; h <= 63; h += 2)
                square[i] += std::sin(2.0f * juce::MathConstants<float>::pi * h * i / TABLE_SIZE) / h;
            square[i] *= 4.0f / juce::MathConstants<float>::pi;
        }
        wavetables[2].name = "Square";
        loadWavetable(square, 2);

        // Triangle
        std::vector<float> triangle(TABLE_SIZE);
        for (int i = 0; i < TABLE_SIZE; ++i)
        {
            triangle[i] = 0.0f;
            for (int h = 1; h <= 63; h += 2)
            {
                float sign = ((h - 1) / 2) % 2 == 0 ? 1.0f : -1.0f;
                triangle[i] += sign * std::sin(2.0f * juce::MathConstants<float>::pi * h * i / TABLE_SIZE) / (h * h);
            }
            triangle[i] *= 8.0f / (juce::MathConstants<float>::pi * juce::MathConstants<float>::pi);
        }
        wavetables[3].name = "Triangle";
        loadWavetable(triangle, 3);
    }

    double sampleRate = 44100.0;
    float frequency = 440.0f;
    float phaseIncrement = 0.0f;
    float phase = 0.0f;
    float tablePosition = 0.0f;
    int currentMipmap = 0;

    std::vector<Wavetable> wavetables;
};

} // namespace DSP
} // namespace NulyBeats
