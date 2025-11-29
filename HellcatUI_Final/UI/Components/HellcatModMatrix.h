#pragma once
#include <JuceHeader.h>
#include "../HellcatLookAndFeel.h"

class HellcatModMatrixRow : public juce::Component
{
public:
    HellcatModMatrixRow()
    {
        sourceCombo.addItem("LFO 1", 1);
        sourceCombo.addItem("LFO 2", 2);
        sourceCombo.addItem("ENV 1", 3);
        sourceCombo.addItem("ENV 2", 4);
        sourceCombo.addItem("Velocity", 5);
        sourceCombo.addItem("Mod Wheel", 6);
        addAndMakeVisible(sourceCombo);
        
        destCombo.addItem("Filter Cutoff", 1);
        destCombo.addItem("Filter Resonance", 2);
        destCombo.addItem("Osc Pitch", 3);
        destCombo.addItem("Osc Mix", 4);
        destCombo.addItem("Pan", 5);
        destCombo.addItem("Volume", 6);
        addAndMakeVisible(destCombo);
        
        amountSlider.setRange(0.0, 1.0);
        amountSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        amountSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        addAndMakeVisible(amountSlider);
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        
        // Bottom border
        g.setColour(HellcatColors::background);
        g.drawLine(bounds.getX(), bounds.getBottom() - 1, 
                   bounds.getRight(), bounds.getBottom() - 1, 1.0f);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10, 6);
        
        sourceCombo.setBounds(bounds.removeFromLeft(bounds.getWidth() / 3 - 5));
        bounds.removeFromLeft(5);
        destCombo.setBounds(bounds.removeFromLeft(bounds.getWidth() / 2 - 5));
        bounds.removeFromLeft(5);
        amountSlider.setBounds(bounds);
    }
    
    juce::ComboBox sourceCombo;
    juce::ComboBox destCombo;
    juce::Slider amountSlider;
};

class HellcatModMatrix : public juce::Component
{
public:
    HellcatModMatrix()
    {
        for (int i = 0; i < 5; ++i)
        {
            auto* row = new HellcatModMatrixRow();
            rows.add(row);
            addAndMakeVisible(row);
        }
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        auto headerBounds = bounds.removeFromTop(40);
        
        // Draw header background
        g.setColour(HellcatColors::background.brighter(0.05f));
        g.fillRect(headerBounds);
        
        // Draw header border
        g.setColour(HellcatColors::panelLight);
        g.drawLine(headerBounds.getX(), headerBounds.getBottom(), 
                   headerBounds.getRight(), headerBounds.getBottom(), 1.0f);
        
        // Draw header text
        g.setColour(HellcatColors::textSecondary);
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        
        auto col1 = headerBounds.removeFromLeft(headerBounds.getWidth() / 3);
        auto col2 = headerBounds.removeFromLeft(headerBounds.getWidth() / 2);
        auto col3 = headerBounds;
        
        g.drawText("SOURCE", col1.reduced(10, 0), juce::Justification::centredLeft);
        g.drawText("DESTINATION", col2.reduced(10, 0), juce::Justification::centredLeft);
        g.drawText("AMOUNT", col3.reduced(10, 0), juce::Justification::centredLeft);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(40); // Header
        
        int rowHeight = bounds.getHeight() / rows.size();
        for (auto* row : rows)
            row->setBounds(bounds.removeFromTop(rowHeight));
    }
    
private:
    juce::OwnedArray<HellcatModMatrixRow> rows;
};
