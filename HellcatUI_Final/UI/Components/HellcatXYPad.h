#pragma once
#include <JuceHeader.h>
#include "../HellcatLookAndFeel.h"

class HellcatXYPad : public juce::Component
{
public:
    HellcatXYPad(const juce::String& xLabel, const juce::String& yLabel)
        : xAxisLabel(xLabel), yAxisLabel(yLabel)
    {
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        auto padBounds = bounds.removeFromTop(bounds.getHeight() - 20);
        
        // Background gradient
        juce::ColourGradient bgGradient(
            juce::Colour(0xff0a0c0f), 0, 0,
            HellcatColors::background, 0, padBounds.getHeight(),
            false
        );
        g.setGradientFill(bgGradient);
        g.fillRoundedRectangle(padBounds.toFloat(), 8.0f);
        
        // Border
        g.setColour(HellcatColors::panelLight);
        g.drawRoundedRectangle(padBounds.toFloat(), 8.0f, 1.0f);
        
        // Crosshair
        g.setColour(HellcatColors::hellcatRed.withAlpha(0.3f));
        g.drawLine(padBounds.getX(), padBounds.getCentreY(), 
                   padBounds.getRight(), padBounds.getCentreY(), 1.0f);
        g.drawLine(padBounds.getCentreX(), padBounds.getY(), 
                   padBounds.getCentreX(), padBounds.getBottom(), 1.0f);
        
        // Draw cursor
        auto cursorX = padBounds.getX() + xValue * padBounds.getWidth();
        auto cursorY = padBounds.getY() + yValue * padBounds.getHeight();
        
        // Glow
        g.setColour(HellcatColors::hellcatRed.withAlpha(0.4f));
        g.fillEllipse(cursorX - 18, cursorY - 18, 36, 36);
        
        // Cursor dot
        juce::ColourGradient cursorGradient(
            HellcatColors::redBright, cursorX - 4, cursorY - 4,
            HellcatColors::hellcatRed, cursorX + 4, cursorY + 4,
            true
        );
        g.setGradientFill(cursorGradient);
        g.fillEllipse(cursorX - 8, cursorY - 8, 16, 16);
        
        // Cursor border
        g.setColour(juce::Colours::white);
        g.drawEllipse(cursorX - 8, cursorY - 8, 16, 16, 2.0f);
        
        // Labels
        auto labelBounds = bounds;
        g.setColour(HellcatColors::textSecondary);
        g.setFont(9.0f);
        g.drawText(xAxisLabel, labelBounds.removeFromLeft(labelBounds.getWidth() / 2), 
                   juce::Justification::centredLeft);
        g.drawText(yAxisLabel, labelBounds, juce::Justification::centredRight);
    }
    
    void mouseDown(const juce::MouseEvent& e) override
    {
        mouseDrag(e);
    }
    
    void mouseDrag(const juce::MouseEvent& e) override
    {
        auto bounds = getLocalBounds().removeFromTop(getHeight() - 20);
        
        xValue = juce::jlimit(0.0f, 1.0f, 
                             (e.x - bounds.getX()) / (float)bounds.getWidth());
        yValue = juce::jlimit(0.0f, 1.0f, 
                             (e.y - bounds.getY()) / (float)bounds.getHeight());
        
        repaint();
        
        if (onValueChange)
            onValueChange(xValue, yValue);
    }
    
    void setValues(float x, float y)
    {
        xValue = juce::jlimit(0.0f, 1.0f, x);
        yValue = juce::jlimit(0.0f, 1.0f, y);
        repaint();
    }
    
    std::function<void(float, float)> onValueChange;
    
private:
    juce::String xAxisLabel;
    juce::String yAxisLabel;
    float xValue = 0.5f;
    float yValue = 0.5f;
};
