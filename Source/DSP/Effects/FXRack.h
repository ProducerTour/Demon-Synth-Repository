#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>
#include <array>

namespace NulyBeats {
namespace DSP {

/**
 * Base class for all effects
 */
class Effect
{
public:
    virtual ~Effect() = default;

    virtual void prepare(double sampleRate, int samplesPerBlock) = 0;
    virtual void process(juce::AudioBuffer<float>& buffer) = 0;
    virtual void reset() = 0;

    virtual juce::String getName() const = 0;

    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }

    void setMix(float mix) { this->mix = juce::jlimit(0.0f, 1.0f, mix); }
    float getMix() const { return mix; }

protected:
    bool enabled = true;
    float mix = 1.0f;
    double sampleRate = 44100.0;
    int samplesPerBlock = 512;
};

/**
 * Stereo Reverb (algorithmic)
 */
class ReverbEffect : public Effect
{
public:
    void prepare(double sr, int blockSize) override
    {
        sampleRate = sr;
        samplesPerBlock = blockSize;

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sr;
        spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
        spec.numChannels = 2;

        reverb.prepare(spec);

        // Pre-allocate dry buffer to avoid allocation during processing
        dryBuffer.setSize(2, blockSize);

        updateParameters();
    }

    void process(juce::AudioBuffer<float>& buffer) override
    {
        if (!enabled)
            return;

        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        // Ensure dry buffer is large enough (without reallocation if possible)
        if (dryBuffer.getNumSamples() < numSamples || dryBuffer.getNumChannels() < numChannels)
            dryBuffer.setSize(numChannels, numSamples, false, false, true);

        // Copy dry signal without allocation
        for (int ch = 0; ch < numChannels; ++ch)
            dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

        // Apply reverb
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        reverb.process(context);

        // Mix
        if (mix < 1.0f)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* wet = buffer.getWritePointer(ch);
                const float* dry = dryBuffer.getReadPointer(ch);

                for (int i = 0; i < numSamples; ++i)
                {
                    wet[i] = dry[i] * (1.0f - mix) + wet[i] * mix;
                }
            }
        }
    }

    void reset() override
    {
        reverb.reset();
    }

    juce::String getName() const override { return "Reverb"; }

    void setRoomSize(float size)
    {
        params.roomSize = juce::jlimit(0.0f, 1.0f, size);
        updateParameters();
    }

    void setDamping(float damp)
    {
        params.damping = juce::jlimit(0.0f, 1.0f, damp);
        updateParameters();
    }

    void setWidth(float width)
    {
        params.width = juce::jlimit(0.0f, 1.0f, width);
        updateParameters();
    }

private:
    void updateParameters()
    {
        reverb.setParameters(params);
    }

    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters params;
    juce::AudioBuffer<float> dryBuffer;
};

/**
 * Stereo Delay with sync and feedback
 */
class DelayEffect : public Effect
{
public:
    static constexpr int MAX_DELAY_SAMPLES = 192000 * 2; // 2 seconds at 192kHz

    void prepare(double sr, int blockSize) override
    {
        sampleRate = sr;
        samplesPerBlock = blockSize;

        delayLineL.resize(MAX_DELAY_SAMPLES);
        delayLineR.resize(MAX_DELAY_SAMPLES);
        reset();
    }

    void process(juce::AudioBuffer<float>& buffer) override
    {
        if (!enabled)
            return;

        float* left = buffer.getWritePointer(0);
        float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : left;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            // Read from delay lines
            float delayedL = readDelay(delayLineL, readPosL);
            float delayedR = readDelay(delayLineR, readPosR);

            // Cross-feedback for ping-pong
            float inputL = left[i] + delayedR * feedback * pingPong;
            float inputR = right[i] + delayedL * feedback * pingPong;

            // Normal feedback
            inputL += delayedL * feedback * (1.0f - pingPong);
            inputR += delayedR * feedback * (1.0f - pingPong);

            // Write to delay lines
            delayLineL[writePos] = inputL;
            delayLineR[writePos] = inputR;

            // Output mix
            left[i] = left[i] * (1.0f - mix) + delayedL * mix;
            right[i] = right[i] * (1.0f - mix) + delayedR * mix;

            // Advance positions
            ++writePos;
            if (writePos >= MAX_DELAY_SAMPLES)
                writePos = 0;

            ++readPosL;
            if (readPosL >= MAX_DELAY_SAMPLES)
                readPosL = 0;

            ++readPosR;
            if (readPosR >= MAX_DELAY_SAMPLES)
                readPosR = 0;
        }
    }

    void reset() override
    {
        std::fill(delayLineL.begin(), delayLineL.end(), 0.0f);
        std::fill(delayLineR.begin(), delayLineR.end(), 0.0f);
        writePos = 0;
        updateReadPositions();
    }

    juce::String getName() const override { return "Delay"; }

    void setDelayTime(float seconds)
    {
        delayTimeL = juce::jlimit(0.001f, 2.0f, seconds);
        delayTimeR = delayTimeL;
        updateReadPositions();
    }

    void setDelayTimeSync(double bpm, float beatDivision)
    {
        float seconds = static_cast<float>(60.0 / bpm * beatDivision);
        setDelayTime(seconds);
    }

    void setFeedback(float fb) { feedback = juce::jlimit(0.0f, 0.99f, fb); }
    void setPingPong(float pp) { pingPong = juce::jlimit(0.0f, 1.0f, pp); }

private:
    float readDelay(const std::vector<float>& line, int readPos) const
    {
        return line[readPos];
    }

    void updateReadPositions()
    {
        int delaySamplesL = static_cast<int>(delayTimeL * sampleRate);
        int delaySamplesR = static_cast<int>(delayTimeR * sampleRate);

        delaySamplesL = std::min(delaySamplesL, MAX_DELAY_SAMPLES - 1);
        delaySamplesR = std::min(delaySamplesR, MAX_DELAY_SAMPLES - 1);

        readPosL = writePos - delaySamplesL;
        if (readPosL < 0)
            readPosL += MAX_DELAY_SAMPLES;

        readPosR = writePos - delaySamplesR;
        if (readPosR < 0)
            readPosR += MAX_DELAY_SAMPLES;
    }

    std::vector<float> delayLineL;
    std::vector<float> delayLineR;
    int writePos = 0;
    int readPosL = 0;
    int readPosR = 0;

    float delayTimeL = 0.5f;
    float delayTimeR = 0.5f;
    float feedback = 0.5f;
    float pingPong = 0.0f;
};

/**
 * Stereo Chorus
 */
class ChorusEffect : public Effect
{
public:
    void prepare(double sr, int blockSize) override
    {
        sampleRate = sr;
        samplesPerBlock = blockSize;

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sr;
        spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
        spec.numChannels = 2;

        chorus.prepare(spec);
        chorus.setRate(1.0f);
        chorus.setDepth(0.25f);
        chorus.setCentreDelay(7.0f);
        chorus.setFeedback(-0.2f);
        chorus.setMix(0.5f);
    }

    void process(juce::AudioBuffer<float>& buffer) override
    {
        if (!enabled)
            return;

        chorus.setMix(mix);

        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        chorus.process(context);
    }

    void reset() override
    {
        chorus.reset();
    }

    juce::String getName() const override { return "Chorus"; }

    void setRate(float rate) { chorus.setRate(rate); }
    void setDepth(float depth) { chorus.setDepth(depth); }
    void setFeedback(float fb) { chorus.setFeedback(fb); }

private:
    juce::dsp::Chorus<float> chorus;
};

/**
 * Stereo Flanger - shorter delay than chorus with higher feedback for "jet" sound
 */
class FlangerEffect : public Effect
{
public:
    void prepare(double sr, int blockSize) override
    {
        sampleRate = sr;
        samplesPerBlock = blockSize;

        // Flanger uses short delay lines (max ~20ms)
        int maxDelaySamples = static_cast<int>(0.02 * sr) + 1;
        delayLineL.resize(static_cast<size_t>(maxDelaySamples));
        delayLineR.resize(static_cast<size_t>(maxDelaySamples));
        maxDelay = maxDelaySamples;

        reset();
    }

    void process(juce::AudioBuffer<float>& buffer) override
    {
        if (!enabled)
            return;

        float* left = buffer.getWritePointer(0);
        float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : left;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            // Update LFO
            float lfoValue = std::sin(lfoPhase * 2.0f * juce::MathConstants<float>::pi);
            lfoPhase += static_cast<float>(rate / sampleRate);
            if (lfoPhase >= 1.0f)
                lfoPhase -= 1.0f;

            // Calculate modulated delay time (0.1ms to 10ms range)
            float minDelayMs = 0.1f;
            float maxDelayMs = 10.0f;
            float centreDelayMs = (minDelayMs + maxDelayMs) * 0.5f;
            float delayRangeMs = (maxDelayMs - minDelayMs) * 0.5f * depth;

            float delayMs = centreDelayMs + lfoValue * delayRangeMs;
            float delaySamples = static_cast<float>(delayMs * 0.001 * sampleRate);
            delaySamples = juce::jlimit(1.0f, static_cast<float>(maxDelay - 1), delaySamples);

            // Stereo spread - offset phase for right channel
            float lfoValueR = std::sin((lfoPhase + stereoSpread) * 2.0f * juce::MathConstants<float>::pi);
            float delayMsR = centreDelayMs + lfoValueR * delayRangeMs;
            float delaySamplesR = static_cast<float>(delayMsR * 0.001 * sampleRate);
            delaySamplesR = juce::jlimit(1.0f, static_cast<float>(maxDelay - 1), delaySamplesR);

            // Read from delay lines with linear interpolation
            float delayedL = readDelayInterp(delayLineL, delaySamples);
            float delayedR = readDelayInterp(delayLineR, delaySamplesR);

            // Write to delay lines with feedback
            delayLineL[static_cast<size_t>(writePos)] = left[i] + delayedL * feedback;
            delayLineR[static_cast<size_t>(writePos)] = right[i] + delayedR * feedback;

            // Output mix
            float dryL = left[i];
            float dryR = right[i];
            left[i] = dryL * (1.0f - mix) + (dryL + delayedL) * 0.5f * mix;
            right[i] = dryR * (1.0f - mix) + (dryR + delayedR) * 0.5f * mix;

            // Advance write position
            ++writePos;
            if (writePos >= maxDelay)
                writePos = 0;
        }
    }

    void reset() override
    {
        std::fill(delayLineL.begin(), delayLineL.end(), 0.0f);
        std::fill(delayLineR.begin(), delayLineR.end(), 0.0f);
        writePos = 0;
        lfoPhase = 0.0f;
    }

    juce::String getName() const override { return "Flanger"; }

    void setRate(float r) { rate = juce::jlimit(0.05f, 10.0f, r); }
    void setDepth(float d) { depth = juce::jlimit(0.0f, 1.0f, d); }
    void setFeedback(float fb) { feedback = juce::jlimit(-0.95f, 0.95f, fb); }
    void setStereoSpread(float spread) { stereoSpread = juce::jlimit(0.0f, 0.5f, spread); }

private:
    float readDelayInterp(const std::vector<float>& line, float delaySamples) const
    {
        float readPos = static_cast<float>(writePos) - delaySamples;
        if (readPos < 0.0f)
            readPos += static_cast<float>(maxDelay);

        int pos0 = static_cast<int>(readPos);
        int pos1 = pos0 + 1;
        if (pos1 >= maxDelay)
            pos1 = 0;

        float frac = readPos - static_cast<float>(pos0);
        return line[static_cast<size_t>(pos0)] * (1.0f - frac) + line[static_cast<size_t>(pos1)] * frac;
    }

    std::vector<float> delayLineL;
    std::vector<float> delayLineR;
    int writePos = 0;
    int maxDelay = 0;
    float lfoPhase = 0.0f;

    float rate = 0.5f;      // LFO rate in Hz
    float depth = 0.7f;     // Modulation depth
    float feedback = 0.5f;  // Feedback amount (-0.95 to 0.95)
    float stereoSpread = 0.25f; // Phase offset for stereo
};

/**
 * Distortion / Saturation
 */
class DistortionEffect : public Effect
{
public:
    enum class Type
    {
        SoftClip,
        HardClip,
        Tube,
        Foldback,
        Bitcrush
    };

    void prepare(double sr, int blockSize) override
    {
        sampleRate = sr;
        samplesPerBlock = blockSize;
    }

    void process(juce::AudioBuffer<float>& buffer) override
    {
        if (!enabled)
            return;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            float* samples = buffer.getWritePointer(ch);

            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float dry = samples[i];
                float wet = processDistortion(samples[i] * drive);
                samples[i] = dry * (1.0f - mix) + wet * mix;
            }
        }
    }

    void reset() override {}

    juce::String getName() const override { return "Distortion"; }

    void setType(Type t) { type = t; }
    void setDrive(float d) { drive = juce::jlimit(1.0f, 100.0f, d); }
    void setBitDepth(int bits) { bitDepth = juce::jlimit(1, 16, bits); }

private:
    float processDistortion(float x) const
    {
        switch (type)
        {
            case Type::SoftClip:
                return std::tanh(x);

            case Type::HardClip:
                return juce::jlimit(-1.0f, 1.0f, x);

            case Type::Tube:
            {
                // Asymmetric soft clipping for tube warmth
                if (x > 0)
                    return 1.0f - std::exp(-x);
                else
                    return -1.0f + std::exp(x);
            }

            case Type::Foldback:
            {
                // Wavefolder
                while (x > 1.0f || x < -1.0f)
                {
                    if (x > 1.0f)
                        x = 2.0f - x;
                    else if (x < -1.0f)
                        x = -2.0f - x;
                }
                return x;
            }

            case Type::Bitcrush:
            {
                float levels = std::pow(2.0f, static_cast<float>(bitDepth));
                return std::round(x * levels) / levels;
            }

            default:
                return x;
        }
    }

    Type type = Type::SoftClip;
    float drive = 1.0f;
    int bitDepth = 8;
};

/**
 * Compressor
 */
class CompressorEffect : public Effect
{
public:
    void prepare(double sr, int blockSize) override
    {
        sampleRate = sr;
        samplesPerBlock = blockSize;

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sr;
        spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
        spec.numChannels = 2;

        compressor.prepare(spec);
        updateParameters();
    }

    void process(juce::AudioBuffer<float>& buffer) override
    {
        if (!enabled)
            return;

        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        compressor.process(context);
    }

    void reset() override
    {
        compressor.reset();
    }

    juce::String getName() const override { return "Compressor"; }

    void setThreshold(float db) { threshold = db; updateParameters(); }
    void setRatio(float r) { ratio = r; updateParameters(); }
    void setAttack(float ms) { attack = ms; updateParameters(); }
    void setRelease(float ms) { release = ms; updateParameters(); }

private:
    void updateParameters()
    {
        compressor.setThreshold(threshold);
        compressor.setRatio(ratio);
        compressor.setAttack(attack);
        compressor.setRelease(release);
    }

    juce::dsp::Compressor<float> compressor;
    float threshold = -12.0f;
    float ratio = 4.0f;
    float attack = 10.0f;
    float release = 100.0f;
};

/**
 * EQ (3-band parametric)
 */
class EQEffect : public Effect
{
public:
    void prepare(double sr, int blockSize) override
    {
        sampleRate = sr;
        samplesPerBlock = blockSize;

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sr;
        spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
        spec.numChannels = 2;

        lowShelf.prepare(spec);
        midPeak.prepare(spec);
        highShelf.prepare(spec);

        updateFilters();
    }

    void process(juce::AudioBuffer<float>& buffer) override
    {
        if (!enabled)
            return;

        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);

        lowShelf.process(context);
        midPeak.process(context);
        highShelf.process(context);
    }

    void reset() override
    {
        lowShelf.reset();
        midPeak.reset();
        highShelf.reset();
    }

    juce::String getName() const override { return "EQ"; }

    void setLowGain(float db) { lowGain = db; updateFilters(); }
    void setLowFreq(float hz) { lowFreq = hz; updateFilters(); }
    void setMidGain(float db) { midGain = db; updateFilters(); }
    void setMidFreq(float hz) { midFreq = hz; updateFilters(); }
    void setMidQ(float q) { midQ = q; updateFilters(); }
    void setHighGain(float db) { highGain = db; updateFilters(); }
    void setHighFreq(float hz) { highFreq = hz; updateFilters(); }

private:
    void updateFilters()
    {
        *lowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            sampleRate, lowFreq, 0.707f, juce::Decibels::decibelsToGain(lowGain));

        *midPeak.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            sampleRate, midFreq, midQ, juce::Decibels::decibelsToGain(midGain));

        *highShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            sampleRate, highFreq, 0.707f, juce::Decibels::decibelsToGain(highGain));
    }

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>> lowShelf, midPeak, highShelf;

    float lowGain = 0.0f, lowFreq = 100.0f;
    float midGain = 0.0f, midFreq = 1000.0f, midQ = 1.0f;
    float highGain = 0.0f, highFreq = 8000.0f;
};

/**
 * FX Rack - manages a chain of effects
 */
class FXRack
{
public:
    FXRack()
    {
        // Add default effects in order
        effects.push_back(std::make_unique<DistortionEffect>());
        effects.push_back(std::make_unique<EQEffect>());
        effects.push_back(std::make_unique<CompressorEffect>());
        effects.push_back(std::make_unique<ChorusEffect>());
        effects.push_back(std::make_unique<FlangerEffect>());
        effects.push_back(std::make_unique<DelayEffect>());
        effects.push_back(std::make_unique<ReverbEffect>());

        // Disable all by default
        for (auto& fx : effects)
            fx->setEnabled(false);
    }

    void prepare(double sampleRate, int samplesPerBlock)
    {
        for (auto& fx : effects)
            fx->prepare(sampleRate, samplesPerBlock);
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        for (auto& fx : effects)
        {
            if (fx->isEnabled())
                fx->process(buffer);
        }
    }

    void reset()
    {
        for (auto& fx : effects)
            fx->reset();
    }

    Effect* getEffect(int index)
    {
        if (index >= 0 && index < static_cast<int>(effects.size()))
            return effects[index].get();
        return nullptr;
    }

    template<typename T>
    T* getEffect()
    {
        for (auto& fx : effects)
        {
            if (auto* cast = dynamic_cast<T*>(fx.get()))
                return cast;
        }
        return nullptr;
    }

    int getNumEffects() const { return static_cast<int>(effects.size()); }

    // Reorder effects
    void moveEffect(int fromIndex, int toIndex)
    {
        if (fromIndex < 0 || fromIndex >= static_cast<int>(effects.size()))
            return;
        if (toIndex < 0 || toIndex >= static_cast<int>(effects.size()))
            return;

        auto effect = std::move(effects[fromIndex]);
        effects.erase(effects.begin() + fromIndex);
        effects.insert(effects.begin() + toIndex, std::move(effect));
    }

private:
    std::vector<std::unique_ptr<Effect>> effects;
};

} // namespace DSP
} // namespace NulyBeats
