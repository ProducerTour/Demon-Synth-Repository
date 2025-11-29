#pragma once
#include <JuceHeader.h>
#include "../HellcatLookAndFeel.h"

class HellcatEnvelopeDisplay : public juce::Component
{
public:
    HellcatEnvelopeDisplay()
    {
        updatePath();
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        
        // Background with gradient
        juce::ColourGradient bgGradient(
            HellcatColors::background, 0, 0,
            juce::Colour(0xff0a0c0f), 0, bounds.getHeight(),
            false
        );
        g.setGradientFill(bgGradient);
        g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
        
        // Border
        g.setColour(HellcatColors::panelLight);
        g.drawRoundedRectangle(bounds.toFloat(), 8.0f, 1.0f);
        
        // Grid
        drawGrid(g, bounds.reduced(20));
        
        // Envelope path with fill
        auto graphBounds = bounds.reduced(20).toFloat();
        
        // Fill under curve
        juce::Path fillPath = envelopePath;
        fillPath.lineTo(graphBounds.getRight(), graphBounds.getBottom());
        fillPath.lineTo(graphBounds.getX(), graphBounds.getBottom());
        fillPath.closeSubPath();
        
        juce::ColourGradient fillGradient(
            HellcatColors::hellcatRed.withAlpha(0.2f), 
            graphBounds.getCentreX(), graphBounds.getY(),
            HellcatColors::hellcatRed.withAlpha(0.0f), 
            graphBounds.getCentreX(), graphBounds.getBottom(),
            false
        );
        g.setGradientFill(fillGradient);
        g.fillPath(fillPath);
        
        // Draw envelope line
        g.setColour(HellcatColors::hellcatRed);
        g.strokePath(envelopePath, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
        
        // Glow effect
        g.setColour(HellcatColors::hellcatRed.withAlpha(0.4f));
        g.strokePath(envelopePath, juce::PathStrokeType(8.0f, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
    }
    
    void setADSR(float attack, float decay, float sustain, float release)
    {
        attackTime = attack;
        decayTime = decay;
        sustainLevel = sustain;
        releaseTime = release;
        updatePath();
        repaint();
    }
    
    void resized() override
    {
        updatePath();
    }
    
private:
    void drawGrid(juce::Graphics& g, juce::Rectangle<int> area)
    {
        g.setColour(HellcatColors::hellcatRed.withAlpha(0.1f));
        
        // Horizontal lines
        for (int i = 0; i <= 4; ++i)
        {
            float y = area.getY() + (area.getHeight() / 4.0f) * i;
            g.drawLine(area.getX(), y, area.getRight(), y, 1.0f);
        }
        
        // Vertical lines
        for (int i = 0; i <= 5; ++i)
        {
            float x = area.getX() + (area.getWidth() / 5.0f) * i;
            g.drawLine(x, area.getY(), x, area.getBottom(), 1.0f);
        }
    }
    
    void updatePath()
    {
        envelopePath.clear();
        
        auto bounds = getLocalBounds().reduced(20).toFloat();
        float width = bounds.getWidth();
        float height = bounds.getHeight();
        
        // Normalize times to width
        float totalTime = attackTime + decayTime + 0.4f + releaseTime;
        float attackX = (attackTime / totalTime) * width;
        float decayX = attackX + (decayTime / totalTime) * width;
        float sustainX = decayX + (0.4f / totalTime) * width;
        float releaseX = width;
        
        // Build path
        envelopePath.startNewSubPath(bounds.getX(), bounds.getBottom());
        envelopePath.lineTo(bounds.getX() + attackX, bounds.getY());
        envelopePath.lineTo(bounds.getX() + decayX, 
                           bounds.getY() + (1.0f - sustainLevel) * height);
        envelopePath.lineTo(bounds.getX() + sustainX, 
                           bounds.getY() + (1.0f - sustainLevel) * height);
        envelopePath.lineTo(bounds.getX() + releaseX, bounds.getBottom());
    }
    
    juce::Path envelopePath;
    float attackTime = 0.045f;
    float decayTime = 0.28f;
    float sustainLevel = 0.65f;
    float releaseTime = 0.52f;
};
