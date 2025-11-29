#pragma once
#include <JuceHeader.h>

namespace HellcatColors
{
    const juce::Colour background     (0xff050608);
    const juce::Colour panelDark      (0xff111217);
    const juce::Colour panelLight     (0xff1a1d22);
    const juce::Colour hellcatRed     (0xffDF1F2F);
    const juce::Colour redDark        (0xffa01620);
    const juce::Colour redBright      (0xffff4040);
    const juce::Colour textPrimary    (0xffffffff);
    const juce::Colour textSecondary  (0xff888888);
    const juce::Colour textTertiary   (0xff666666);
}

class HellcatLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HellcatLookAndFeel()
    {
        // Set default colors
        setColour(juce::ResizableWindow::backgroundColourId, HellcatColors::background);
        setColour(juce::DocumentWindow::backgroundColourId, HellcatColors::background);
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff0a0c0f));
        setColour(juce::ComboBox::outlineColourId, HellcatColors::panelLight);
        setColour(juce::ComboBox::textColourId, HellcatColors::textPrimary);
        setColour(juce::TextButton::buttonColourId, HellcatColors::panelLight);
        setColour(juce::TextButton::textColourOffId, HellcatColors::textTertiary);
        setColour(juce::TextButton::textColourOnId, HellcatColors::textPrimary);
    }
    
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                         juce::Slider& slider) override
    {
        auto radius = juce::jmin(width / 2, height / 2) - 4.0f;
        auto centerX = x + width * 0.5f;
        auto centerY = y + height * 0.5f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        
        // Draw knob body with metallic gradient
        juce::ColourGradient knobGradient(
            HellcatColors::panelLight, centerX - radius * 0.3f, centerY - radius * 0.3f,
            HellcatColors::background, centerX + radius * 0.7f, centerY + radius * 0.7f,
            true
        );
        g.setGradientFill(knobGradient);
        g.fillEllipse(centerX - radius, centerY - radius, radius * 2.0f, radius * 2.0f);
        
        // Draw border
        g.setColour(HellcatColors::panelLight);
        g.drawEllipse(centerX - radius, centerY - radius, radius * 2.0f, radius * 2.0f, 3.0f);
        
        // Draw indicator
        juce::Path indicator;
        auto indicatorLength = radius * 0.3f;
        auto indicatorThickness = 4.0f;
        
        indicator.addRectangle(-indicatorThickness * 0.5f, -radius + 8.0f, 
                               indicatorThickness, indicatorLength);
        
        g.setColour(HellcatColors::hellcatRed);
        g.fillPath(indicator, juce::AffineTransform::rotation(angle)
                                .translated(centerX, centerY));
        
        // Add glow effect
        g.setColour(HellcatColors::hellcatRed.withAlpha(0.3f));
        g.drawEllipse(centerX - radius - 5, centerY - radius - 5, 
                     (radius + 5) * 2.0f, (radius + 5) * 2.0f, 10.0f);
    }
    
    void drawTabButton(juce::TabBarButton& button, juce::Graphics& g, bool isMouseOver, bool isMouseDown) override
    {
        auto area = button.getActiveArea();
        auto isActive = button.getToggleState();
        
        // Background
        if (isActive)
        {
            g.setColour(HellcatColors::panelDark);
            g.fillRect(area);
        }
        else if (isMouseOver)
        {
            g.setColour(HellcatColors::hellcatRed.withAlpha(0.05f));
            g.fillRect(area);
        }
        
        // Text
        g.setColour(isActive ? HellcatColors::textPrimary : HellcatColors::textTertiary);
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawText(button.getButtonText(), area, juce::Justification::centred);
        
        // Active indicator line
        if (isActive)
        {
            g.setColour(HellcatColors::hellcatRed);
            g.fillRect(area.removeFromBottom(2));
        }
    }
    
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                     int buttonX, int buttonY, int buttonW, int buttonH,
                     juce::ComboBox& box) override
    {
        auto cornerSize = 4.0f;
        juce::Rectangle<int> boxBounds(0, 0, width, height);
        
        // Background gradient
        juce::ColourGradient bgGradient(
            juce::Colour(0xff0f1114), 0, 0,
            juce::Colour(0xff0a0c0f), 0, height,
            false
        );
        g.setGradientFill(bgGradient);
        g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);
        
        // Border
        g.setColour(box.hasKeyboardFocus(true) ? HellcatColors::hellcatRed : HellcatColors::panelLight);
        g.drawRoundedRectangle(boxBounds.toFloat(), cornerSize, 1.0f);
        
        // Arrow
        juce::Path arrow;
        arrow.addTriangle(buttonX + buttonW * 0.3f, buttonY + buttonH * 0.4f,
                         buttonX + buttonW * 0.7f, buttonY + buttonH * 0.4f,
                         buttonX + buttonW * 0.5f, buttonY + buttonH * 0.7f);
        g.setColour(HellcatColors::hellcatRed);
        g.fillPath(arrow);
    }
    
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                             bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        auto cornerSize = 6.0f;
        
        if (button.getToggleState())
        {
            juce::ColourGradient activeGradient(
                HellcatColors::hellcatRed, bounds.getX(), bounds.getY(),
                HellcatColors::redDark, bounds.getX(), bounds.getBottom(),
                false
            );
            g.setGradientFill(activeGradient);
            g.fillRoundedRectangle(bounds, cornerSize);
            
            // Inner highlight
            g.setColour(juce::Colours::white.withAlpha(0.2f));
            g.drawRoundedRectangle(bounds.reduced(1), cornerSize, 1.0f);
        }
        else
        {
            juce::ColourGradient inactiveGradient(
                juce::Colour(0xff0f1114), bounds.getX(), bounds.getY(),
                juce::Colour(0xff0a0c0f), bounds.getX(), bounds.getBottom(),
                false
            );
            g.setGradientFill(inactiveGradient);
            g.fillRoundedRectangle(bounds, cornerSize);
        }
        
        // Border
        g.setColour(button.getToggleState() ? HellcatColors::hellcatRed : HellcatColors::panelLight);
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
        
        if (shouldDrawButtonAsHighlighted && !button.getToggleState())
        {
            g.setColour(HellcatColors::hellcatRed.withAlpha(0.1f));
            g.fillRoundedRectangle(bounds, cornerSize);
        }
    }
};
