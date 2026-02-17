#pragma once
#include <JuceHeader.h>
#include "../HellcatLookAndFeel.h"

/**
 * Simple waveform oscilloscope that displays the most recent audio output.
 * Uses a lock-free ring buffer for thread-safe audio→GUI data transfer.
 */
class HellcatOscilloscope : public juce::Component
{
public:
    static constexpr int SCOPE_SIZE = 512;

    HellcatOscilloscope() = default;

    /** Called from audio thread — pushes samples into the ring buffer */
    void pushBuffer(const float* data, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            int pos = writePos.load(std::memory_order_relaxed);
            ringBuffer[static_cast<size_t>(pos)] = data[i];
            writePos.store((pos + 1) % SCOPE_SIZE, std::memory_order_release);
        }
        newDataAvailable.store(true, std::memory_order_release);
    }

    /** Called from timer thread — snapshots ring buffer for display */
    void updateDisplay()
    {
        if (!newDataAvailable.exchange(false, std::memory_order_acquire))
            return;

        int pos = writePos.load(std::memory_order_acquire);
        for (int i = 0; i < SCOPE_SIZE; ++i)
        {
            int idx = (pos + i) % SCOPE_SIZE;
            displayBuffer[static_cast<size_t>(i)] = ringBuffer[static_cast<size_t>(idx)];
        }
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Background
        g.setColour(HellcatColors::panelDark);
        g.fillRoundedRectangle(bounds, 6.0f);

        // Border
        g.setColour(HellcatColors::panelLight);
        g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

        // Grid lines
        g.setColour(HellcatColors::hellcatRed.withAlpha(0.08f));
        float midY = bounds.getCentreY();
        g.drawLine(bounds.getX(), midY, bounds.getRight(), midY, 1.0f);
        float quarterH = bounds.getHeight() * 0.25f;
        g.drawLine(bounds.getX(), midY - quarterH, bounds.getRight(), midY - quarterH, 0.5f);
        g.drawLine(bounds.getX(), midY + quarterH, bounds.getRight(), midY + quarterH, 0.5f);

        // Draw waveform
        auto area = bounds.reduced(4, 4);
        float w = area.getWidth();
        float h = area.getHeight();
        float centerY = area.getCentreY();

        juce::Path waveform;
        bool started = false;

        for (int i = 0; i < SCOPE_SIZE; ++i)
        {
            float x = area.getX() + (static_cast<float>(i) / SCOPE_SIZE) * w;
            float sample = juce::jlimit(-1.0f, 1.0f, displayBuffer[static_cast<size_t>(i)]);
            float y = centerY - sample * (h * 0.45f);

            if (!started)
            {
                waveform.startNewSubPath(x, y);
                started = true;
            }
            else
            {
                waveform.lineTo(x, y);
            }
        }

        // Glow
        g.setColour(HellcatColors::hellcatRed.withAlpha(0.2f));
        g.strokePath(waveform, juce::PathStrokeType(4.0f));

        // Main line
        g.setColour(HellcatColors::hellcatRed);
        g.strokePath(waveform, juce::PathStrokeType(1.5f));
    }

private:
    std::array<float, SCOPE_SIZE> ringBuffer{};
    std::array<float, SCOPE_SIZE> displayBuffer{};
    std::atomic<int> writePos{0};
    std::atomic<bool> newDataAvailable{false};
};
