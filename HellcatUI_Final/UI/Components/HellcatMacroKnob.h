#pragma once
#include <JuceHeader.h>
#include "../HellcatLookAndFeel.h"

class HellcatMacroKnob : public juce::Component
{
public:
    HellcatMacroKnob(const juce::String& name)
        : knobName(name)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        slider.setRange(0.0, 100.0);
        slider.setValue(50.0);
        slider.onValueChange = [this]() { repaint(); };
        addAndMakeVisible(slider);
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        auto knobBounds = bounds.removeFromTop(70);
        
        // Label
        g.setColour(HellcatColors::hellcatRed);
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        auto labelBounds = bounds.removeFromTop(20);
        g.drawText(knobName.toUpperCase(), labelBounds, juce::Justification::centred);
        
        // Value
        g.setColour(HellcatColors::textPrimary);
        g.setFont(13.0f);
        g.drawText(juce::String((int)slider.getValue()) + "%", bounds, 
                   juce::Justification::centred);
    }
    
    void resized() override
    {
        slider.setBounds(getLocalBounds().removeFromTop(70));
    }
    
    juce::Slider& getSlider() { return slider; }
    
private:
    juce::Slider slider;
    juce::String knobName;
};
