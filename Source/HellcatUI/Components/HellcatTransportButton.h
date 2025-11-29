#pragma once
#include <JuceHeader.h>
#include "../HellcatLookAndFeel.h"

class HellcatTransportButton : public juce::Button
{
public:
    HellcatTransportButton(const juce::String& name, const juce::String& icon)
        : juce::Button(name), iconText(icon)
    {
        setClickingTogglesState(true);
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                    bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2);

        if (getToggleState())
        {
            juce::ColourGradient activeGradient(
                HellcatColors::hellcatRed, bounds.getX(), bounds.getY(),
                HellcatColors::redDark, bounds.getX(), bounds.getBottom(),
                false
            );
            g.setGradientFill(activeGradient);
            g.fillRoundedRectangle(bounds, 10.0f);

            g.setColour(HellcatColors::hellcatRed.withAlpha(0.4f));
            g.drawRoundedRectangle(bounds.expanded(3), 10.0f, 6.0f);

            g.setColour(juce::Colours::white.withAlpha(0.15f));
            g.drawRoundedRectangle(bounds.reduced(1), 10.0f, 1.0f);
        }
        else
        {
            juce::ColourGradient inactiveGradient(
                HellcatColors::panelDark.brighter(0.15f), bounds.getX(), bounds.getY(),
                HellcatColors::panelDark, bounds.getX(), bounds.getBottom(),
                false
            );
            g.setGradientFill(inactiveGradient);
            g.fillRoundedRectangle(bounds, 10.0f);
        }

        g.setColour(getToggleState() ? HellcatColors::hellcatRed : HellcatColors::panelLight);
        g.drawRoundedRectangle(bounds, 10.0f, 1.5f);

        auto iconBounds = bounds.removeFromTop(bounds.getHeight() * 0.55f);
        g.setColour(getToggleState() ? juce::Colours::white : HellcatColors::textSecondary);
        g.setFont(22.0f);
        g.drawText(iconText, iconBounds, juce::Justification::centred);

        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.drawText(getButtonText(), bounds, juce::Justification::centred);
    }

private:
    juce::String iconText;
};

// ARM button with record dot
class HellcatArmButton : public juce::Button
{
public:
    HellcatArmButton() : juce::Button("ARM") { setClickingTogglesState(true); }

    void paintButton(juce::Graphics& g, bool, bool) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2);

        if (getToggleState())
        {
            juce::ColourGradient activeGradient(
                HellcatColors::hellcatRed, bounds.getX(), bounds.getY(),
                HellcatColors::redDark, bounds.getX(), bounds.getBottom(), false);
            g.setGradientFill(activeGradient);
            g.fillRoundedRectangle(bounds, 10.0f);
            g.setColour(HellcatColors::hellcatRed.withAlpha(0.4f));
            g.drawRoundedRectangle(bounds.expanded(3), 10.0f, 6.0f);
        }
        else
        {
            juce::ColourGradient inactiveGradient(
                HellcatColors::panelDark.brighter(0.15f), bounds.getX(), bounds.getY(),
                HellcatColors::panelDark, bounds.getX(), bounds.getBottom(), false);
            g.setGradientFill(inactiveGradient);
            g.fillRoundedRectangle(bounds, 10.0f);
        }

        g.setColour(getToggleState() ? HellcatColors::hellcatRed : HellcatColors::panelLight);
        g.drawRoundedRectangle(bounds, 10.0f, 1.5f);

        auto iconBounds = bounds.removeFromTop(bounds.getHeight() * 0.55f);
        g.setColour(getToggleState() ? juce::Colours::white : HellcatColors::textSecondary);
        float dotSize = 12.0f;
        g.fillEllipse(iconBounds.getCentreX() - dotSize/2, iconBounds.getCentreY() - dotSize/2, dotSize, dotSize);

        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.drawText("ARM", bounds, juce::Justification::centred);
    }
};

// LOCK button with lock icon
class HellcatLockButton : public juce::Button
{
public:
    HellcatLockButton() : juce::Button("LOCK") { setClickingTogglesState(true); }

    void paintButton(juce::Graphics& g, bool, bool) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2);

        if (getToggleState())
        {
            juce::ColourGradient activeGradient(
                HellcatColors::hellcatRed, bounds.getX(), bounds.getY(),
                HellcatColors::redDark, bounds.getX(), bounds.getBottom(), false);
            g.setGradientFill(activeGradient);
            g.fillRoundedRectangle(bounds, 10.0f);
        }
        else
        {
            juce::ColourGradient inactiveGradient(
                HellcatColors::panelDark.brighter(0.15f), bounds.getX(), bounds.getY(),
                HellcatColors::panelDark, bounds.getX(), bounds.getBottom(), false);
            g.setGradientFill(inactiveGradient);
            g.fillRoundedRectangle(bounds, 10.0f);
        }

        g.setColour(getToggleState() ? HellcatColors::hellcatRed : HellcatColors::panelLight);
        g.drawRoundedRectangle(bounds, 10.0f, 1.5f);

        auto iconBounds = bounds.removeFromTop(bounds.getHeight() * 0.55f);
        g.setColour(getToggleState() ? juce::Colours::white : HellcatColors::textSecondary);
        float lockWidth = 14.0f, lockHeight = 10.0f;
        float cx = iconBounds.getCentreX(), cy = iconBounds.getCentreY() + 2;
        g.fillRoundedRectangle(cx - lockWidth/2, cy, lockWidth, lockHeight, 2.0f);

        // Draw lock shackle using Path
        juce::Path shackle;
        shackle.addArc(cx - 5, cy - 8, 10, 10, juce::MathConstants<float>::pi, juce::MathConstants<float>::twoPi, true);
        g.strokePath(shackle, juce::PathStrokeType(2.0f));

        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.drawText("LOCK", bounds, juce::Justification::centred);
    }
};

// SRT-style "Push to Start" circular button - like a Dodge vehicle start button
class HellcatPushToStartButton : public juce::Button
{
public:
    HellcatPushToStartButton() : juce::Button("ENGINE START") { setClickingTogglesState(true); }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat();
        float size = std::min(bounds.getWidth(), bounds.getHeight());
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();
        float radius = size * 0.48f;

        // Outer chrome ring - beveled metallic look
        {
            juce::ColourGradient chromeGradient(
                juce::Colour(0xff606060), cx - radius, cy - radius,
                juce::Colour(0xff303030), cx + radius, cy + radius, false);
            chromeGradient.addColour(0.3, juce::Colour(0xff808080));
            chromeGradient.addColour(0.7, juce::Colour(0xff404040));
            g.setGradientFill(chromeGradient);
            g.fillEllipse(cx - radius, cy - radius, radius * 2, radius * 2);
        }

        // Inner button area
        float innerRadius = radius * 0.88f;

        if (getToggleState())
        {
            // ENGINE ON - Glowing red with pulsing effect
            juce::ColourGradient activeGradient(
                HellcatColors::hellcatRed.brighter(0.3f), cx, cy - innerRadius * 0.5f,
                HellcatColors::redDark, cx, cy + innerRadius, false);
            g.setGradientFill(activeGradient);
            g.fillEllipse(cx - innerRadius, cy - innerRadius, innerRadius * 2, innerRadius * 2);

            // Red glow effect
            g.setColour(HellcatColors::hellcatRed.withAlpha(0.5f));
            g.drawEllipse(cx - radius - 4, cy - radius - 4, radius * 2 + 8, radius * 2 + 8, 6.0f);
            g.setColour(HellcatColors::hellcatRed.withAlpha(0.25f));
            g.drawEllipse(cx - radius - 8, cy - radius - 8, radius * 2 + 16, radius * 2 + 16, 4.0f);
        }
        else
        {
            // ENGINE OFF - Dark with subtle gradient
            juce::ColourGradient inactiveGradient(
                juce::Colour(0xff2a2a2a), cx, cy - innerRadius * 0.5f,
                juce::Colour(0xff1a1a1a), cx, cy + innerRadius, false);
            g.setGradientFill(inactiveGradient);
            g.fillEllipse(cx - innerRadius, cy - innerRadius, innerRadius * 2, innerRadius * 2);
        }

        // Pressed state - slightly darker
        if (shouldDrawButtonAsDown)
        {
            g.setColour(juce::Colours::black.withAlpha(0.3f));
            g.fillEllipse(cx - innerRadius, cy - innerRadius, innerRadius * 2, innerRadius * 2);
        }

        // Hover highlight
        if (shouldDrawButtonAsHighlighted && !shouldDrawButtonAsDown)
        {
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.fillEllipse(cx - innerRadius, cy - innerRadius, innerRadius * 2, innerRadius * 2);
        }

        // Inner chrome ring
        g.setColour(juce::Colour(0xff505050));
        g.drawEllipse(cx - innerRadius, cy - innerRadius, innerRadius * 2, innerRadius * 2, 2.0f);

        // SRT logo area - draw "ENGINE" text on top, "START/STOP" below
        float textRadius = innerRadius * 0.7f;

        g.setColour(getToggleState() ? juce::Colours::white : HellcatColors::textSecondary);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), size * 0.09f, juce::Font::bold));

        // "ENGINE" text - moved up higher
        g.drawText("ENGINE",
                   juce::Rectangle<float>(cx - textRadius, cy - textRadius * 0.85f, textRadius * 2, size * 0.12f),
                   juce::Justification::centred);

        // Power symbol in center - moved up slightly
        float symbolSize = size * 0.18f;
        float symbolY = cy - symbolSize * 0.15f;

        // Power circle (broken at top)
        juce::Path powerCircle;
        float arcRadius = symbolSize * 0.4f;
        powerCircle.addArc(cx - arcRadius, symbolY - arcRadius, arcRadius * 2, arcRadius * 2,
                          juce::MathConstants<float>::pi * 0.3f,
                          juce::MathConstants<float>::pi * 2.7f, true);

        g.setColour(getToggleState() ? juce::Colours::white : HellcatColors::textSecondary);
        g.strokePath(powerCircle, juce::PathStrokeType(2.5f));

        // Vertical line at top of power symbol
        g.drawLine(cx, symbolY - arcRadius - 2, cx, symbolY - arcRadius * 0.3f, 2.5f);

        // "START STOP" text
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), size * 0.08f, juce::Font::bold));
        g.drawText("START  STOP",
                   juce::Rectangle<float>(cx - textRadius, cy + textRadius * 0.4f, textRadius * 2, size * 0.1f),
                   juce::Justification::centred);
    }
};
