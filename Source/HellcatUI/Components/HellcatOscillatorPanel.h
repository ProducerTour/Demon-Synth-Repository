#pragma once
#include <JuceHeader.h>
#include "../HellcatLookAndFeel.h"

class HellcatWaveformButton : public juce::Button
{
public:
    HellcatWaveformButton(const juce::String& name)
        : juce::Button(name)
    {
        setClickingTogglesState(true);
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                    bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2);

        if (getToggleState())
        {
            // Active state - red fill
            juce::ColourGradient activeGradient(
                HellcatColors::hellcatRed, bounds.getX(), bounds.getY(),
                HellcatColors::redDark, bounds.getX(), bounds.getBottom(),
                false
            );
            g.setGradientFill(activeGradient);
            g.fillRoundedRectangle(bounds, 6.0f);

            g.setColour(juce::Colours::white);
        }
        else
        {
            // Inactive state
            g.setColour(HellcatColors::panelLight);
            g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
            g.setColour(HellcatColors::textSecondary);
        }

        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(getButtonText(), bounds, juce::Justification::centred);
    }
};

class HellcatOscillatorPanel : public juce::Component
{
public:
    HellcatOscillatorPanel()
    {
        // Waveform buttons
        sawButton.setButtonText("SAW");
        sawButton.setRadioGroupId(100);
        sawButton.setToggleState(true, juce::dontSendNotification);
        addAndMakeVisible(sawButton);

        sqrButton.setButtonText("SQR");
        sqrButton.setRadioGroupId(100);
        addAndMakeVisible(sqrButton);

        triButton.setButtonText("TRI");
        triButton.setRadioGroupId(100);
        addAndMakeVisible(triButton);

        sinButton.setButtonText("SIN");
        sinButton.setRadioGroupId(100);
        addAndMakeVisible(sinButton);

        // Button callbacks
        sawButton.onClick = [this]() { if (onWaveformChange) onWaveformChange(0); };
        sqrButton.onClick = [this]() { if (onWaveformChange) onWaveformChange(1); };
        triButton.onClick = [this]() { if (onWaveformChange) onWaveformChange(2); };
        sinButton.onClick = [this]() { if (onWaveformChange) onWaveformChange(3); };
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Panel background with carbon fiber texture
        drawPanelCarbonFiber(g, bounds, 12.0f);

        // Chrome/metallic border
        juce::ColourGradient borderGradient(
            juce::Colour(0xff4a4a4a), bounds.getX(), bounds.getY(),
            juce::Colour(0xff2a2a2a), bounds.getRight(), bounds.getBottom(),
            true
        );
        g.setGradientFill(borderGradient);
        g.drawRoundedRectangle(bounds.reduced(1), 12.0f, 2.0f);

        // Inner border highlight
        g.setColour(HellcatColors::panelLight.withAlpha(0.3f));
        g.drawRoundedRectangle(bounds.reduced(3), 10.0f, 1.0f);

        // Title with Orbitron font
        g.setColour(HellcatColors::textSecondary);
        if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
            g.setFont(lf->getOrbitronFont(11.0f));
        else
            g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText("OSCILLATOR ENGINE", bounds.removeFromTop(35), juce::Justification::centred);

        // Draw the gauge
        drawGauge(g);
    }

    void drawPanelCarbonFiber(juce::Graphics& g, juce::Rectangle<float> bounds, float cornerSize)
    {
        // Dark gradient base - like brushed dark metal
        juce::ColourGradient baseGradient(
            juce::Colour(0xff141414), bounds.getX(), bounds.getY(),
            juce::Colour(0xff0a0a0a), bounds.getX(), bounds.getBottom(),
            false
        );
        g.setGradientFill(baseGradient);
        g.fillRoundedRectangle(bounds, cornerSize);

        // Save graphics state and clip to rounded rect
        g.saveState();
        juce::Path clipPath;
        clipPath.addRoundedRectangle(bounds, cornerSize);
        g.reduceClipRegion(clipPath);

        // Subtle horizontal brushed metal lines
        g.setColour(juce::Colour(0xff1a1a1a));
        for (float y = bounds.getY(); y < bounds.getBottom(); y += 2.0f)
        {
            float alpha = 0.3f + 0.2f * std::sin(y * 0.5f);
            g.setColour(juce::Colour(0xff1c1c1c).withAlpha(alpha));
            g.drawHorizontalLine(static_cast<int>(y), bounds.getX(), bounds.getRight());
        }

        g.restoreState();

        // Subtle vignette/shadow around edges
        juce::ColourGradient vignetteGradient(
            juce::Colours::transparentBlack, bounds.getCentreX(), bounds.getCentreY(),
            juce::Colour(0x40000000), bounds.getX(), bounds.getY(),
            true
        );
        g.setGradientFill(vignetteGradient);
        g.fillRoundedRectangle(bounds, cornerSize);

        // Top edge highlight
        juce::ColourGradient topHighlight(
            juce::Colour(0x15ffffff), bounds.getX(), bounds.getY(),
            juce::Colours::transparentWhite, bounds.getX(), bounds.getY() + 30,
            false
        );
        g.setGradientFill(topHighlight);
        g.fillRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), 30, cornerSize);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(35); // Title

        // Gauge area
        gaugeBounds = bounds.removeFromTop(bounds.getHeight() - 60);

        // Button area
        auto buttonArea = bounds.reduced(15, 10);
        int buttonWidth = buttonArea.getWidth() / 4;

        sawButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(3));
        sqrButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(3));
        triButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(3));
        sinButton.setBounds(buttonArea.reduced(3));
    }

    void setValue(float newValue)
    {
        currentValue = newValue;
        repaint();
    }

    void setWaveform(int waveform)
    {
        sawButton.setToggleState(waveform == 0, juce::dontSendNotification);
        sqrButton.setToggleState(waveform == 1, juce::dontSendNotification);
        triButton.setToggleState(waveform == 2, juce::dontSendNotification);
        sinButton.setToggleState(waveform == 3, juce::dontSendNotification);
    }

    std::function<void(int)> onWaveformChange;

private:
    void drawGauge(juce::Graphics& g)
    {
        auto centerX = static_cast<float>(gaugeBounds.getCentreX());
        auto centerY = static_cast<float>(gaugeBounds.getCentreY());
        auto radius = juce::jmin(gaugeBounds.getWidth(), gaugeBounds.getHeight()) * 0.38f;

        // === Outer chrome bezel with multiple rings (like Hellcat) ===
        // Outer glow ring (red when active)
        float glowIntensity = currentValue / maxValue;
        g.setColour(HellcatColors::hellcatRed.withAlpha(0.15f + glowIntensity * 0.2f));
        g.drawEllipse(centerX - radius - 12, centerY - radius - 12,
                     (radius + 12) * 2, (radius + 12) * 2, 8.0f);

        // Chrome bezel - outer ring
        juce::ColourGradient bezelGradient(
            juce::Colour(0xff3a3a3a), centerX - radius, centerY - radius,
            juce::Colour(0xff1a1a1a), centerX + radius, centerY + radius,
            true
        );
        g.setGradientFill(bezelGradient);
        g.drawEllipse(centerX - radius - 4, centerY - radius - 4,
                     (radius + 4) * 2, (radius + 4) * 2, 4.0f);

        // Inner bezel highlight
        g.setColour(juce::Colour(0xff4a4a4a));
        g.drawEllipse(centerX - radius - 2, centerY - radius - 2,
                     (radius + 2) * 2, (radius + 2) * 2, 1.0f);

        // Gauge background - dark with subtle gradient
        juce::ColourGradient bgGradient(
            juce::Colour(0xff0a0a0a), centerX, centerY - radius,
            juce::Colour(0xff050505), centerX, centerY + radius,
            false
        );
        g.setGradientFill(bgGradient);
        g.fillEllipse(centerX - radius, centerY - radius, radius * 2, radius * 2);

        // Draw tick marks and numbers (like RPM gauge)
        drawTickMarks(g, centerX, centerY, radius);

        // Draw value arc with glow
        drawArc(g, centerX, centerY, radius - 10);

        // === Center section with carbon fiber texture ===
        float innerRadius = radius * 0.65f;

        // Carbon fiber background
        drawCarbonFiber(g, centerX, centerY, innerRadius);

        // Inner ring (like speaker grille surround)
        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawEllipse(centerX - innerRadius, centerY - innerRadius,
                     innerRadius * 2, innerRadius * 2, 2.0f);

        // Draw value text - use Orbitron Black font for Hellcat dashboard style
        g.setColour(HellcatColors::textPrimary);
        if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
            g.setFont(lf->getOrbitronBlackFont(48.0f));
        else
            g.setFont(juce::Font(48.0f, juce::Font::bold));

        auto textBounds = juce::Rectangle<float>(
            centerX - innerRadius * 0.8f, centerY - 30,
            innerRadius * 1.6f, 60
        );
        g.drawText(juce::String(static_cast<int>(currentValue)), textBounds.toNearestInt(),
                   juce::Justification::centred);

        // Draw labels below value
        g.setColour(HellcatColors::textSecondary);
        if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
            g.setFont(lf->getOrbitronFont(11.0f));
        else
            g.setFont(juce::Font(11.0f, juce::Font::bold));

        auto labelBounds = juce::Rectangle<float>(
            centerX - 50, centerY + 20, 100, 20
        );
        g.drawText("VOICES", labelBounds.toNearestInt(), juce::Justification::centred);

        g.setColour(HellcatColors::textTertiary);
        if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
            g.setFont(lf->getOrbitronFont(9.0f));
        else
            g.setFont(juce::Font(9.0f, juce::Font::plain));
        labelBounds.translate(0, 14);
        g.drawText("UNISON", labelBounds.toNearestInt(), juce::Justification::centred);
    }

    void drawCarbonFiber(juce::Graphics& g, float centerX, float centerY, float radius)
    {
        // Dark background
        g.setColour(juce::Colour(0xff0c0c0c));
        g.fillEllipse(centerX - radius, centerY - radius, radius * 2, radius * 2);

        // Carbon fiber pattern (simplified diagonal grid)
        g.setColour(juce::Colour(0xff151515));
        float step = 4.0f;
        for (float offset = -radius; offset < radius; offset += step)
        {
            // Diagonal lines
            g.drawLine(centerX - radius + offset, centerY - radius,
                      centerX + offset, centerY + radius, 0.5f);
            g.drawLine(centerX + offset, centerY - radius,
                      centerX - radius + offset, centerY + radius, 0.5f);
        }
    }

    void drawTickMarks(juce::Graphics& g, float centerX, float centerY, float radius)
    {
        const float startAngle = -2.356f;  // ~225 degrees
        const float endAngle = 2.356f;      // ~135 degrees
        const int numMajorTicks = 9;        // 0-8 voices

        // Draw major tick marks with numbers (like RPM gauge: 1,2,3,4,5,6,7,8)
        for (int i = 0; i < numMajorTicks; ++i)
        {
            float angle = startAngle + (i / static_cast<float>(numMajorTicks - 1)) * (endAngle - startAngle);

            float tickLength = 14.0f;
            float tickWidth = 2.5f;

            auto tickStart = radius - 3.0f;
            auto tickEnd = tickStart - tickLength;

            juce::Point<float> start(
                centerX + std::cos(angle) * tickStart,
                centerY + std::sin(angle) * tickStart
            );

            juce::Point<float> end(
                centerX + std::cos(angle) * tickEnd,
                centerY + std::sin(angle) * tickEnd
            );

            // Color based on value (danger zone at high values)
            bool isDanger = (i >= 6);
            g.setColour(isDanger ? HellcatColors::hellcatRed : HellcatColors::textSecondary);
            g.drawLine(start.x, start.y, end.x, end.y, tickWidth);

            // Draw numbers next to major ticks
            auto numberRadius = tickEnd - 12.0f;
            juce::Point<float> numberPos(
                centerX + std::cos(angle) * numberRadius,
                centerY + std::sin(angle) * numberRadius
            );

            g.setColour(isDanger ? HellcatColors::hellcatRed : HellcatColors::textPrimary);
            if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
                g.setFont(lf->getOrbitronFont(12.0f));
            else
                g.setFont(juce::Font(12.0f, juce::Font::bold));

            g.drawText(juce::String(i),
                      juce::Rectangle<float>(numberPos.x - 10, numberPos.y - 8, 20, 16).toNearestInt(),
                      juce::Justification::centred);
        }

        // Draw minor tick marks between major ticks
        const int numMinorTicks = 17;
        for (int i = 0; i < numMinorTicks; ++i)
        {
            if (i % 2 == 0) continue; // Skip positions where major ticks are

            float angle = startAngle + (i / static_cast<float>(numMinorTicks - 1)) * (endAngle - startAngle);

            float tickLength = 6.0f;
            float tickWidth = 1.0f;

            auto tickStart = radius - 3.0f;
            auto tickEnd = tickStart - tickLength;

            juce::Point<float> start(
                centerX + std::cos(angle) * tickStart,
                centerY + std::sin(angle) * tickStart
            );

            juce::Point<float> end(
                centerX + std::cos(angle) * tickEnd,
                centerY + std::sin(angle) * tickEnd
            );

            g.setColour(HellcatColors::panelLight);
            g.drawLine(start.x, start.y, end.x, end.y, tickWidth);
        }
    }

    void drawArc(juce::Graphics& g, float centerX, float centerY, float radius)
    {
        const float startAngle = -2.356f;
        const float endAngle = 2.356f;
        float valueAngle = startAngle + (currentValue / maxValue) * (endAngle - startAngle);

        juce::Path arc;
        arc.addCentredArc(centerX, centerY, radius, radius,
                         0.0f, startAngle, valueAngle, true);

        // Draw glow
        g.setColour(HellcatColors::hellcatRed.withAlpha(0.3f));
        g.strokePath(arc, juce::PathStrokeType(12.0f));

        // Draw main arc with gradient
        juce::ColourGradient arcGradient(
            HellcatColors::redDark, centerX, centerY - radius,
            HellcatColors::redBright, centerX, centerY + radius,
            false
        );
        g.setGradientFill(arcGradient);
        g.strokePath(arc, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    }

    HellcatWaveformButton sawButton{"SAW"};
    HellcatWaveformButton sqrButton{"SQR"};
    HellcatWaveformButton triButton{"TRI"};
    HellcatWaveformButton sinButton{"SIN"};

    juce::Rectangle<int> gaugeBounds;
    float currentValue = 8.0f;
    float maxValue = 8.0f;
};
