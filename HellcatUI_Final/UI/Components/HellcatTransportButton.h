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
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        if (getToggleState())
        {
            juce::ColourGradient activeGradient(
                HellcatColors::hellcatRed, bounds.getX(), bounds.getY(),
                HellcatColors::redDark, bounds.getX(), bounds.getBottom(),
                false
            );
            g.setGradientFill(activeGradient);
            g.fillRoundedRectangle(bounds, 8.0f);
            
            // Glow
            g.setColour(HellcatColors::hellcatRed.withAlpha(0.5f));
            g.drawRoundedRectangle(bounds.expanded(5), 8.0f, 10.0f);
            
            // Inner highlight
            g.setColour(juce::Colours::white.withAlpha(0.2f));
            g.drawRoundedRectangle(bounds.reduced(1), 8.0f, 1.0f);
        }
        else
        {
            juce::ColourGradient inactiveGradient(
                HellcatColors::panelLight, bounds.getX(), bounds.getY(),
                HellcatColors::background, bounds.getX(), bounds.getBottom(),
                false
            );
            g.setGradientFill(inactiveGradient);
            g.fillRoundedRectangle(bounds, 8.0f);
        }
        
        // Border
        g.setColour(getToggleState() ? HellcatColors::hellcatRed : HellcatColors::panelLight);
        g.drawRoundedRectangle(bounds, 8.0f, 2.0f);
        
        // Icon
        auto iconBounds = bounds.removeFromTop(bounds.getHeight() * 0.6f);
        g.setColour(getToggleState() ? juce::Colours::white : HellcatColors::textTertiary);
        g.setFont(20.0f);
        g.drawText(iconText, iconBounds, juce::Justification::centred);
        
        // Label
        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.drawText(getButtonText(), bounds, juce::Justification::centred);
    }
    
private:
    juce::String iconText;
};
