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
        
        // Hover highlight
        if (isHovered || isDragging)
        {
            g.setColour(HellcatColors::hellcatRed.withAlpha(isDragging ? 0.08f : 0.04f));
            g.fillRoundedRectangle(padBounds.toFloat(), 8.0f);
        }

        // Crosshair
        g.setColour(HellcatColors::hellcatRed.withAlpha(isHovered ? 0.5f : 0.3f));
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
        if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
            g.setFont(lf->getOrbitronFont(9.0f));
        else
            g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.drawText(xAxisLabel, labelBounds.removeFromLeft(labelBounds.getWidth() / 2), 
                   juce::Justification::centredLeft);
        g.drawText(yAxisLabel, labelBounds, juce::Justification::centredRight);
    }
    
    void mouseDown(const juce::MouseEvent& e) override
    {
        isDragging = true;
        setMouseCursor(juce::MouseCursor::CrosshairCursor);
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

    void mouseUp(const juce::MouseEvent&) override
    {
        isDragging = false;
        setMouseCursor(juce::MouseCursor::NormalCursor);
        repaint();
    }

    void mouseEnter(const juce::MouseEvent&) override
    {
        isHovered = true;
        setMouseCursor(juce::MouseCursor::CrosshairCursor);
        repaint();
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        isHovered = false;
        setMouseCursor(juce::MouseCursor::NormalCursor);
        repaint();
    }

    void setValues(float x, float y)
    {
        float newX = juce::jlimit(0.0f, 1.0f, x);
        float newY = juce::jlimit(0.0f, 1.0f, y);
        if (newX != xValue || newY != yValue)
        {
            xValue = newX;
            yValue = newY;
            repaint();
        }
    }
    
    std::function<void(float, float)> onValueChange;
    
private:
    juce::String xAxisLabel;
    juce::String yAxisLabel;
    float xValue = 0.5f;
    float yValue = 0.5f;
    bool isHovered = false;
    bool isDragging = false;
};
