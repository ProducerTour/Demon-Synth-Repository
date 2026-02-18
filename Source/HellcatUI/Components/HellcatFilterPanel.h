#pragma once
#include <JuceHeader.h>
#include "../HellcatLookAndFeel.h"

class HellcatFilterButton : public juce::Button
{
public:
    HellcatFilterButton(const juce::String& name)
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
            // Inactive state - dark fill with border
            g.setColour(HellcatColors::panelDark.brighter(0.1f));
            g.fillRoundedRectangle(bounds, 6.0f);
            g.setColour(HellcatColors::panelLight);
            g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
            g.setColour(HellcatColors::textSecondary);
        }

        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(getButtonText(), bounds, juce::Justification::centred);
    }
};

class HellcatFilterPanel : public juce::Component
{
public:
    HellcatFilterPanel()
    {
        // Filter type buttons - 2x2 grid
        lp12Button.setButtonText("LP12");
        lp12Button.setRadioGroupId(101);
        lp12Button.setToggleState(true, juce::dontSendNotification);
        addAndMakeVisible(lp12Button);

        lp24Button.setButtonText("LP24");
        lp24Button.setRadioGroupId(101);
        addAndMakeVisible(lp24Button);

        bpButton.setButtonText("BP");
        bpButton.setRadioGroupId(101);
        addAndMakeVisible(bpButton);

        hpButton.setButtonText("HP");
        hpButton.setRadioGroupId(101);
        addAndMakeVisible(hpButton);

        // Button callbacks
        lp12Button.onClick = [this]() { if (onFilterTypeChange) onFilterTypeChange(0); };
        lp24Button.onClick = [this]() { if (onFilterTypeChange) onFilterTypeChange(1); };
        bpButton.onClick = [this]() { if (onFilterTypeChange) onFilterTypeChange(2); };
        hpButton.onClick = [this]() { if (onFilterTypeChange) onFilterTypeChange(3); };

        // Resonance knob
        resoSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        resoSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        resoSlider.setRange(0.0, 1.0, 0.01);
        resoSlider.setTooltip("Filter Resonance");
        addAndMakeVisible(resoSlider);

        resoLabel.setText("RES", juce::dontSendNotification);
        resoLabel.setJustificationType(juce::Justification::centred);
        resoLabel.setColour(juce::Label::textColourId, HellcatColors::textTertiary);
        resoLabel.setFont(juce::Font(9.0f, juce::Font::bold));
        addAndMakeVisible(resoLabel);

        // Key track knob
        keytrackSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        keytrackSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        keytrackSlider.setRange(0.0, 1.0, 0.01);
        keytrackSlider.setTooltip("Filter Key Track — scales filter cutoff with MIDI note (0 = off, 1 = full)");
        addAndMakeVisible(keytrackSlider);

        keytrackLabel.setText("KEY", juce::dontSendNotification);
        keytrackLabel.setJustificationType(juce::Justification::centred);
        keytrackLabel.setColour(juce::Label::textColourId, HellcatColors::textTertiary);
        keytrackLabel.setFont(juce::Font(9.0f, juce::Font::bold));
        addAndMakeVisible(keytrackLabel);
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
        g.drawText("FILTER DRIVE", bounds.removeFromTop(35), juce::Justification::centred);

        // Draw the gauge
        drawGauge(g);

        // Draw mode dots
        drawModeDots(g);
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

        // Carbon fiber texture overlay
        if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
        {
            auto& img = lf->getCarbonFiberImage();
            if (img.isValid())
            {
                g.setOpacity(0.45f);
                g.drawImage(img, bounds,
                           juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination);
                g.setOpacity(1.0f);
            }
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

        // Gauge area (includes dots)
        gaugeBounds = bounds.removeFromTop(bounds.getHeight() - 110);

        // Resonance + Keytrack knob row
        auto resoArea = bounds.removeFromTop(30);
        auto halfRow = resoArea.getWidth() / 2;
        auto resoHalf = resoArea.removeFromLeft(halfRow);
        resoLabel.setBounds(resoHalf.removeFromLeft(24));
        resoSlider.setBounds(resoHalf);
        keytrackLabel.setBounds(resoArea.removeFromLeft(24));
        keytrackSlider.setBounds(resoArea);

        // Button area - 2x2 grid
        auto buttonArea = bounds.reduced(15, 5);
        int buttonWidth = buttonArea.getWidth() / 2;
        int buttonHeight = buttonArea.getHeight() / 2;

        auto topRow = buttonArea.removeFromTop(buttonHeight);
        auto bottomRow = buttonArea;

        lp12Button.setBounds(topRow.removeFromLeft(buttonWidth).reduced(3));
        lp24Button.setBounds(topRow.reduced(3));
        bpButton.setBounds(bottomRow.removeFromLeft(buttonWidth).reduced(3));
        hpButton.setBounds(bottomRow.reduced(3));
    }

    void setValue(float newValue)
    {
        currentValue = newValue;
        repaint();
    }

    void setFilterType(int type)
    {
        lp12Button.setToggleState(type == 0, juce::dontSendNotification);
        lp24Button.setToggleState(type == 1, juce::dontSendNotification);
        bpButton.setToggleState(type == 2, juce::dontSendNotification);
        hpButton.setToggleState(type == 3, juce::dontSendNotification);
    }

    std::function<void(int)> onFilterTypeChange;
    std::function<void(float)> onGaugeValueChange;

    juce::Slider& getResoSlider()     { return resoSlider; }
    juce::Slider& getKeytrackSlider() { return keytrackSlider; }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (gaugeBounds.contains(e.getPosition()))
        {
            isDraggingGauge = true;
            dragStartY = e.y;
            dragStartValue = currentValue;
            setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (isDraggingGauge)
        {
            // Map vertical drag to kHz range
            float delta = (dragStartY - e.y) / 8.0f; // 8px per kHz
            float newValue = juce::jlimit(0.02f, maxValue, dragStartValue + delta);
            if (std::abs(newValue - currentValue) > 0.01f)
            {
                currentValue = newValue;
                repaint();
                if (onGaugeValueChange)
                    onGaugeValueChange(currentValue * 1000.0f); // Convert kHz back to Hz
            }
        }
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        isDraggingGauge = false;
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        bool overGauge = gaugeBounds.contains(e.getPosition());
        if (overGauge != gaugeHovered)
        {
            gaugeHovered = overGauge;
            setMouseCursor(overGauge ? juce::MouseCursor::UpDownResizeCursor
                                     : juce::MouseCursor::NormalCursor);
            repaint();
        }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (gaugeHovered)
        {
            gaugeHovered = false;
            setMouseCursor(juce::MouseCursor::NormalCursor);
            repaint();
        }
    }

private:
    void drawGauge(juce::Graphics& g)
    {
        auto centerX = static_cast<float>(gaugeBounds.getCentreX());
        auto centerY = static_cast<float>(gaugeBounds.getCentreY() - 10);
        auto radius = juce::jmin(gaugeBounds.getWidth(), gaugeBounds.getHeight()) * 0.38f;

        // === Outer chrome bezel with multiple rings (like Hellcat speedometer) ===
        // Outer glow ring (intensity based on value, brighter on hover)
        float glowIntensity = currentValue / maxValue;
        float hoverBoost = gaugeHovered ? 0.15f : 0.0f;
        g.setColour(HellcatColors::hellcatRed.withAlpha(0.15f + glowIntensity * 0.2f + hoverBoost));
        g.drawEllipse(centerX - radius - 12, centerY - radius - 12,
                     (radius + 12) * 2, (radius + 12) * 2, gaugeHovered ? 10.0f : 8.0f);

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

        // Draw tick marks with numbers (like speedometer)
        drawTickMarks(g, centerX, centerY, radius);

        // Draw value arc with glow
        drawArc(g, centerX, centerY, radius - 10);

        // === Center section with carbon fiber texture ===
        float innerRadius = radius * 0.65f;

        // Carbon fiber background
        drawCarbonFiber(g, centerX, centerY, innerRadius);

        // Inner ring
        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawEllipse(centerX - innerRadius, centerY - innerRadius,
                     innerRadius * 2, innerRadius * 2, 2.0f);

        // Draw value text - use Sofachrome font for racing dashboard style
        g.setColour(HellcatColors::textPrimary);
        if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
            g.setFont(lf->getSofachromeFont(42.0f));
        else
            g.setFont(juce::Font(42.0f, juce::Font::bold));

        // Show value with appropriate precision — more decimals at low values
        juce::String valueText = currentValue < 1.0f ? juce::String(currentValue, 2)
                                                      : juce::String(currentValue, 1);
        auto textBounds = juce::Rectangle<float>(
            centerX - innerRadius, centerY - 24,
            innerRadius * 2.0f, 48
        );
        g.drawText(valueText, textBounds.toNearestInt(), juce::Justification::centred);

        // Draw labels below value
        g.setColour(HellcatColors::textSecondary);
        if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
            g.setFont(lf->getOrbitronFont(11.0f));
        else
            g.setFont(juce::Font(11.0f, juce::Font::bold));

        auto labelBounds = juce::Rectangle<float>(
            centerX - 50, centerY + 18, 100, 20
        );
        g.drawText("CUTOFF", labelBounds.toNearestInt(), juce::Justification::centred);

        g.setColour(HellcatColors::textTertiary);
        if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
            g.setFont(lf->getOrbitronFont(9.0f));
        else
            g.setFont(juce::Font(9.0f, juce::Font::plain));
        labelBounds.translate(0, 14);
        g.drawText("kHz", labelBounds.toNearestInt(), juce::Justification::centred);
    }

    void drawCarbonFiber(juce::Graphics& g, float centerX, float centerY, float radius)
    {
        // Dark background
        g.setColour(juce::Colour(0xff0c0c0c));
        g.fillEllipse(centerX - radius, centerY - radius, radius * 2, radius * 2);

        // Draw carbon fiber texture image
        if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
        {
            auto& img = lf->getCarbonFiberImage();
            if (img.isValid())
            {
                g.saveState();
                juce::Path clipCircle;
                clipCircle.addEllipse(centerX - radius, centerY - radius, radius * 2, radius * 2);
                g.reduceClipRegion(clipCircle);

                auto destBounds = juce::Rectangle<float>(
                    centerX - radius, centerY - radius, radius * 2, radius * 2);
                g.drawImage(img, destBounds,
                           juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination);

                g.restoreState();
            }
        }
    }

    void drawTickMarks(juce::Graphics& g, float centerX, float centerY, float radius)
    {
        const float startAngle = -2.356f;  // ~225 degrees
        const float endAngle = 2.356f;      // ~135 degrees
        const int numMajorTicks = 11;       // 0, 20, 40, 60, 80, 100, 120, 140, 160, 180, 200, 220

        // Tick values representing kHz scaled (0-20kHz shown as 0-220 like speedometer MPH)
        const int tickValues[] = {0, 20, 40, 60, 80, 100, 120, 140, 160, 180, 200};

        // Draw major tick marks with numbers
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

            // High frequencies (upper range) shown in red like redline
            bool isDanger = (i >= 8);
            g.setColour(isDanger ? HellcatColors::hellcatRed : HellcatColors::textSecondary);
            g.drawLine(start.x, start.y, end.x, end.y, tickWidth);

            // Draw numbers next to major ticks
            auto numberRadius = tickEnd - 14.0f;
            juce::Point<float> numberPos(
                centerX + std::cos(angle) * numberRadius,
                centerY + std::sin(angle) * numberRadius
            );

            g.setColour(isDanger ? HellcatColors::hellcatRed : HellcatColors::textPrimary);
            if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
                g.setFont(lf->getOrbitronFont(10.0f));
            else
                g.setFont(juce::Font(10.0f, juce::Font::bold));

            g.drawText(juce::String(tickValues[i]),
                      juce::Rectangle<float>(numberPos.x - 14, numberPos.y - 7, 28, 14).toNearestInt(),
                      juce::Justification::centred);
        }

        // Draw minor tick marks between major ticks
        const int numMinorTicks = 21;
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

        // Draw main arc with gradient (red to orange/yellow)
        juce::ColourGradient arcGradient(
            HellcatColors::hellcatRed, centerX - radius, centerY,
            juce::Colour(0xffFF8C00), centerX + radius, centerY, // Orange at high end
            false
        );
        g.setGradientFill(arcGradient);
        g.strokePath(arc, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    }

    void drawModeDots(juce::Graphics& g)
    {
        // Draw mode indicator dots below gauge
        auto dotsY = gaugeBounds.getBottom() - 25;
        auto dotsX = gaugeBounds.getCentreX() - 50;
        const int numDots = 9;
        const float dotSize = 6.0f;
        const float dotSpacing = 12.0f;

        int activeDots = static_cast<int>((currentValue / maxValue) * numDots);

        for (int i = 0; i < numDots; ++i)
        {
            float x = dotsX + i * dotSpacing;

            if (i < activeDots)
            {
                g.setColour(HellcatColors::hellcatRed);
            }
            else
            {
                g.setColour(HellcatColors::panelLight);
            }
            g.fillEllipse(x, static_cast<float>(dotsY), dotSize, dotSize);
        }
    }

    HellcatFilterButton lp12Button{"LP12"};
    HellcatFilterButton lp24Button{"LP24"};
    HellcatFilterButton bpButton{"BP"};
    HellcatFilterButton hpButton{"HP"};

    juce::Slider resoSlider;
    juce::Label  resoLabel;
    juce::Slider keytrackSlider;
    juce::Label  keytrackLabel;

    juce::Rectangle<int> gaugeBounds;
    float currentValue = 7.2f;
    float maxValue = 20.0f;

    // Drag state for interactive gauge
    bool isDraggingGauge = false;
    bool gaugeHovered = false;
    float dragStartY = 0.0f;
    float dragStartValue = 0.0f;
};
