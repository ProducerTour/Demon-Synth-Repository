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

// NITRO button with lightning icon (drawn, not emoji)
class HellcatNitroButton : public juce::Button
{
public:
    HellcatNitroButton() : juce::Button("NITRO") { setClickingTogglesState(true); }

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

        // Draw lightning bolt
        float boltX = iconBounds.getCentreX() - 6;
        float boltY = iconBounds.getCentreY() - 8;
        float boltW = 12, boltH = 16;

        juce::Path bolt;
        bolt.startNewSubPath(boltX + boltW * 0.6f, boltY);
        bolt.lineTo(boltX + boltW * 0.25f, boltY + boltH * 0.45f);
        bolt.lineTo(boltX + boltW * 0.5f, boltY + boltH * 0.45f);
        bolt.lineTo(boltX + boltW * 0.35f, boltY + boltH);
        bolt.lineTo(boltX + boltW * 0.75f, boltY + boltH * 0.55f);
        bolt.lineTo(boltX + boltW * 0.5f, boltY + boltH * 0.55f);
        bolt.closeSubPath();

        g.setColour(getToggleState() ? juce::Colours::white : HellcatColors::textSecondary);
        g.fillPath(bolt);

        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.0f, juce::Font::bold));
        g.drawText("NITRO", bounds, juce::Justification::centred);
    }
};
