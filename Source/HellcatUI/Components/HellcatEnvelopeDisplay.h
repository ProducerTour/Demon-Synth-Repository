#pragma once
#include <JuceHeader.h>
#include "../HellcatLookAndFeel.h"

class HellcatEnvelopeDisplay : public juce::Component,
                               private juce::Timer
{
public:
    HellcatEnvelopeDisplay()
    {
        // One-shot timer to force repaint after component is laid out
        startTimer(100);
    }

    ~HellcatEnvelopeDisplay() override
    {
        stopTimer();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        if (bounds.isEmpty()) return;

        // Reserve bottom area for ADSR values
        auto valueBounds = bounds.removeFromBottom(60);
        auto graphBounds = bounds;

        // Background with gradient
        juce::ColourGradient bgGradient(
            HellcatColors::background, 0, 0,
            juce::Colour(0xff0a0c0f), 0, static_cast<float>(graphBounds.getHeight()),
            false
        );
        g.setGradientFill(bgGradient);
        g.fillRoundedRectangle(graphBounds.toFloat(), 8.0f);

        // Border
        g.setColour(HellcatColors::panelLight);
        g.drawRoundedRectangle(graphBounds.toFloat(), 8.0f, 1.0f);

        // Grid
        drawGrid(g, graphBounds.reduced(20));

        // Build path based on current bounds
        auto pathBounds = graphBounds.reduced(20).toFloat();
        if (pathBounds.getWidth() < 10 || pathBounds.getHeight() < 10) return;

        juce::Path envPath;
        float width = pathBounds.getWidth();
        float height = pathBounds.getHeight();

        float totalTime = attackTime + decayTime + 0.4f + releaseTime;
        float attackX = (attackTime / totalTime) * width;
        float decayX = attackX + (decayTime / totalTime) * width;
        float sustainX = decayX + (0.4f / totalTime) * width;

        envPath.startNewSubPath(pathBounds.getX(), pathBounds.getBottom());
        envPath.lineTo(pathBounds.getX() + attackX, pathBounds.getY());
        envPath.lineTo(pathBounds.getX() + decayX, pathBounds.getY() + (1.0f - sustainLevel) * height);
        envPath.lineTo(pathBounds.getX() + sustainX, pathBounds.getY() + (1.0f - sustainLevel) * height);
        envPath.lineTo(pathBounds.getRight(), pathBounds.getBottom());

        // Fill under curve
        juce::Path fillPath = envPath;
        fillPath.lineTo(pathBounds.getRight(), pathBounds.getBottom());
        fillPath.lineTo(pathBounds.getX(), pathBounds.getBottom());
        fillPath.closeSubPath();

        juce::ColourGradient fillGradient(
            HellcatColors::hellcatRed.withAlpha(0.2f),
            pathBounds.getCentreX(), pathBounds.getY(),
            HellcatColors::hellcatRed.withAlpha(0.0f),
            pathBounds.getCentreX(), pathBounds.getBottom(),
            false
        );
        g.setGradientFill(fillGradient);
        g.fillPath(fillPath);

        // Draw envelope line
        g.setColour(HellcatColors::hellcatRed);
        g.strokePath(envPath, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

        // Glow effect
        g.setColour(HellcatColors::hellcatRed.withAlpha(0.4f));
        g.strokePath(envPath, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

        // Draw ADSR values at bottom
        drawADSRValues(g, valueBounds);
    }

    void setADSR(float attack, float decay, float sustain, float release)
    {
        attackTime = attack;
        decayTime = decay;
        sustainLevel = sustain;
        releaseTime = release;
        repaint();
    }

    void resized() override
    {
        repaint();
    }

private:
    void timerCallback() override
    {
        stopTimer();
        repaint();
    }

    void drawGrid(juce::Graphics& g, juce::Rectangle<int> area)
    {
        g.setColour(HellcatColors::hellcatRed.withAlpha(0.1f));

        for (int i = 0; i <= 4; ++i)
        {
            float y = area.getY() + (area.getHeight() / 4.0f) * i;
            g.drawLine(static_cast<float>(area.getX()), y, static_cast<float>(area.getRight()), y, 1.0f);
        }

        for (int i = 0; i <= 5; ++i)
        {
            float x = area.getX() + (area.getWidth() / 5.0f) * i;
            g.drawLine(x, static_cast<float>(area.getY()), x, static_cast<float>(area.getBottom()), 1.0f);
        }
    }

    void drawADSRValues(juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        bounds.reduce(10, 5);
        int colWidth = bounds.getWidth() / 4;

        const juce::String labels[] = {"ATTACK", "DECAY", "SUSTAIN", "RELEASE"};
        const juce::String values[] = {
            juce::String(static_cast<int>(attackTime * 1000)) + "ms",
            juce::String(static_cast<int>(decayTime * 1000)) + "ms",
            juce::String(static_cast<int>(sustainLevel * 100)) + "%",
            juce::String(static_cast<int>(releaseTime * 1000)) + "ms"
        };

        for (int i = 0; i < 4; ++i)
        {
            auto col = bounds.removeFromLeft(colWidth);

            // Label - use Orbitron for dashboard consistency
            g.setColour(HellcatColors::textTertiary);
            if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
                g.setFont(lf->getOrbitronFont(9.0f));
            else
                g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.0f, juce::Font::bold));
            g.drawText(labels[i], col.removeFromTop(15), juce::Justification::centred);

            // Value - use Orbitron Black for large values
            g.setColour(HellcatColors::textPrimary);
            if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
                g.setFont(lf->getOrbitronBlackFont(16.0f));
            else
                g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 16.0f, juce::Font::bold));
            auto valueArea = col.removeFromTop(25);
            g.drawText(values[i], valueArea, juce::Justification::centred);

            // Red underline
            g.setColour(HellcatColors::hellcatRed);
            auto underlineBounds = valueArea.reduced(10, 0);
            g.fillRect(underlineBounds.getX(), underlineBounds.getBottom() + 2,
                      underlineBounds.getWidth(), 3);
        }
    }

    float attackTime = 0.045f;
    float decayTime = 0.28f;
    float sustainLevel = 0.65f;
    float releaseTime = 0.52f;
};
