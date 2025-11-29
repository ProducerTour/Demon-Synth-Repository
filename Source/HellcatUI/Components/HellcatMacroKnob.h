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

        // Proportional sizing - 65% knob, 18% label, 17% value
        int knobHeight = static_cast<int>(bounds.getHeight() * 0.65f);
        int labelHeight = static_cast<int>(bounds.getHeight() * 0.18f);

        bounds.removeFromTop(knobHeight); // Skip knob area (slider draws itself)

        // Label
        g.setColour(HellcatColors::hellcatRed);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 11.0f, juce::Font::bold));
        auto labelBounds = bounds.removeFromTop(labelHeight);
        g.drawText(knobName.toUpperCase(), labelBounds, juce::Justification::centred);

        // Value
        g.setColour(HellcatColors::textPrimary);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 13.0f, juce::Font::bold));
        g.drawText(juce::String(static_cast<int>(slider.getValue())) + "%", bounds,
                   juce::Justification::centred);
    }

    void resized() override
    {
        // Proportional knob height
        int knobHeight = static_cast<int>(getHeight() * 0.65f);
        slider.setBounds(getLocalBounds().removeFromTop(knobHeight));
    }
    
    juce::Slider& getSlider() { return slider; }
    
private:
    juce::Slider slider;
    juce::String knobName;
};
