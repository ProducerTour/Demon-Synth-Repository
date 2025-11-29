#pragma once
#include <JuceHeader.h>
#include "../HellcatLookAndFeel.h"

class HellcatTopBar : public juce::Component,
                      private juce::Timer
{
public:
    HellcatTopBar()
    {
        // Engine mode buttons
        ecoButton.setButtonText("ECO");
        ecoButton.setRadioGroupId(1);
        addAndMakeVisible(ecoButton);
        
        sportButton.setButtonText("SPORT");
        sportButton.setRadioGroupId(1);
        addAndMakeVisible(sportButton);
        
        trackButton.setButtonText("TRACK");
        trackButton.setRadioGroupId(1);
        trackButton.setToggleState(true, juce::dontSendNotification);
        addAndMakeVisible(trackButton);
        
        // Preset browser
        presetCombo.addItem("Dark Energy", 1);
        presetCombo.addItem("Midnight Run", 2);
        presetCombo.addItem("Red Line", 3);
        presetCombo.addItem("Supercharger", 4);
        presetCombo.addItem("Nitrous Oxide", 5);
        presetCombo.setSelectedId(1);
        addAndMakeVisible(presetCombo);
        
        // Animate meter
        startTimerHz(10);
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        
        // Background gradient
        juce::ColourGradient bgGradient(
            HellcatColors::panelDark, 0, 0,
            juce::Colour(0xff0a0c0f), 0, bounds.getHeight(),
            false
        );
        g.setGradientFill(bgGradient);
        g.fillRect(bounds);
        
        // Bottom border
        g.setColour(HellcatColors::panelLight);
        g.drawLine(0, bounds.getBottom() - 1, bounds.getRight(), bounds.getBottom() - 1, 1.0f);
        
        // Logo
        g.setColour(HellcatColors::hellcatRed);
        g.setFont(juce::Font(24.0f, juce::Font::bold));
        g.drawText("⚡ HELLCAT", logoBounds, juce::Justification::centredLeft);
        
        // Output meter
        drawOutputMeter(g, meterBounds);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds().reduced(25, 0);
        
        // Logo (left)
        logoBounds = bounds.removeFromLeft(200);
        
        // Output meter (right)
        meterBounds = bounds.removeFromRight(100);
        
        // Engine mode (right)
        auto modeBounds = bounds.removeFromRight(250).reduced(0, 12);
        auto buttonWidth = modeBounds.getWidth() / 3;
        ecoButton.setBounds(modeBounds.removeFromLeft(buttonWidth).reduced(2));
        sportButton.setBounds(modeBounds.removeFromLeft(buttonWidth).reduced(2));
        trackButton.setBounds(modeBounds.reduced(2));
        
        // Preset browser (center)
        presetCombo.setBounds(bounds.withSizeKeepingCentre(200, 30));
    }
    
private:
    void timerCallback() override
    {
        // Animate meter
        meterLevel = juce::Random::getSystemRandom().nextInt(juce::Range<int>(4, 9));
        repaint(meterBounds);
    }
    
    void drawOutputMeter(juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        const int numBars = 10;
        const int barWidth = 4;
        const int gap = 3;
        
        auto x = bounds.getRight() - (numBars * (barWidth + gap));
        auto y = bounds.getCentreY() - 15;
        
        for (int i = 0; i < numBars; ++i)
        {
            bool isActive = i < meterLevel;
            float barHeight = 8.0f + (i * 2.2f);
            
            if (isActive)
            {
                juce::ColourGradient barGradient(
                    HellcatColors::hellcatRed, x, y,
                    juce::Colour(0xff4CAF50), x, y + barHeight,
                    false
                );
                g.setGradientFill(barGradient);
            }
            else
            {
                g.setColour(HellcatColors::panelLight);
            }
            
            g.fillRoundedRectangle(x, y + (30 - barHeight), barWidth, barHeight, 2.0f);
            x += barWidth + gap;
        }
    }
    
    juce::Rectangle<int> logoBounds;
    juce::Rectangle<int> meterBounds;
    
    juce::TextButton ecoButton;
    juce::TextButton sportButton;
    juce::TextButton trackButton;
    juce::ComboBox presetCombo;
    
    int meterLevel = 6;
};
