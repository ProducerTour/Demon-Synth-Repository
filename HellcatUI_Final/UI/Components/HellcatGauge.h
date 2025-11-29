#pragma once
#include <JuceHeader.h>
#include "../HellcatLookAndFeel.h"

class HellcatGauge : public juce::Component,
                     private juce::Timer
{
public:
    enum class Type { Oscillator, Filter };
    
    HellcatGauge(Type type, const juce::String& label)
        : gaugeType(type), gaugeLabel(label)
    {
        startTimerHz(30);
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        auto gaugeBounds = bounds.removeFromTop(bounds.getHeight() - 60);
        auto centerX = gaugeBounds.getWidth() * 0.5f;
        auto centerY = gaugeBounds.getHeight() * 0.5f;
        auto radius = juce::jmin(gaugeBounds.getWidth(), gaugeBounds.getHeight()) * 0.4f;
        
        // Draw title
        g.setColour(HellcatColors::textSecondary);
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.drawText(gaugeLabel.toUpperCase(), bounds.removeFromTop(20), 
                   juce::Justification::centredTop);
        
        // Draw metallic bezel
        drawBezel(g, centerX, centerY, radius + 15);
        
        // Draw gauge background
        juce::ColourGradient bgGradient(
            HellcatColors::panelLight, centerX - radius * 0.3f, centerY - radius * 0.3f,
            HellcatColors::background, centerX + radius * 0.7f, centerY + radius * 0.7f,
            true
        );
        g.setGradientFill(bgGradient);
        g.fillEllipse(centerX - radius, centerY - radius, radius * 2.0f, radius * 2.0f);
        
        // Draw tick marks
        drawTickMarks(g, centerX, centerY, radius);
        
        // Draw arc
        drawArc(g, centerX, centerY, radius);
        
        // Draw carbon fiber center
        drawCarbonFiberCenter(g, centerX, centerY, radius * 0.85f);
        
        // Draw value
        g.setColour(HellcatColors::textPrimary);
        g.setFont(juce::Font(52.0f, juce::Font::bold));
        g.drawText(juce::String(currentValue, 1), 
                   gaugeBounds.reduced(radius * 0.5f),
                   juce::Justification::centred);
        
        // Draw sublabels
        g.setColour(HellcatColors::textSecondary);
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        auto labelBounds = gaugeBounds.withSizeKeepingCentre(radius * 1.5f, radius * 1.5f);
        g.drawText(subLabel, labelBounds.removeFromBottom(40), juce::Justification::centredTop);
        
        g.setColour(HellcatColors::textTertiary);
        g.setFont(9.0f);
        g.drawText(unitLabel, labelBounds.removeFromBottom(20), juce::Justification::centredTop);
    }
    
    void setValue(float newValue)
    {
        currentValue = newValue;
        repaint();
    }
    
    void setSubLabel(const juce::String& label) { subLabel = label; }
    void setUnitLabel(const juce::String& label) { unitLabel = label; }
    void setMaxValue(float max) { maxValue = max; }
    
private:
    void drawBezel(juce::Graphics& g, float centerX, float centerY, float radius)
    {
        juce::ColourGradient bezelGradient(
            juce::Colour(0xff646464), centerX - radius * 0.3f, centerY - radius * 0.3f,
            juce::Colour(0xff141414), centerX + radius * 0.7f, centerY + radius * 0.7f,
            true
        );
        g.setGradientFill(bezelGradient);
        g.fillEllipse(centerX - radius, centerY - radius, radius * 2.0f, radius * 2.0f);
        
        // Highlight
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.fillEllipse(centerX - radius * 0.95f, centerY - radius * 0.95f, 
                     radius * 0.5f, radius * 0.5f);
    }
    
    void drawTickMarks(juce::Graphics& g, float centerX, float centerY, float radius)
    {
        const float startAngle = -2.356f;  // -135 degrees
        const float endAngle = 2.356f;     // 135 degrees
        const int numTicks = 17;
        
        for (int i = 0; i < numTicks; ++i)
        {
            float angle = startAngle + (i / (float)(numTicks - 1)) * (endAngle - startAngle);
            bool isMajor = (i % 2 == 0);
            
            float tickLength = isMajor ? 18.0f : 12.0f;
            float tickWidth = isMajor ? 3.0f : 2.0f;
            
            auto tickStart = radius - 10.0f;
            auto tickEnd = tickStart - tickLength;
            
            juce::Point<float> start(
                centerX + std::cos(angle) * tickStart,
                centerY + std::sin(angle) * tickStart
            );
            
            juce::Point<float> end(
                centerX + std::cos(angle) * tickEnd,
                centerY + std::sin(angle) * tickEnd
            );
            
            // Color based on gauge type and position
            bool isDanger = (gaugeType == Type::Filter && i >= 12);
            g.setColour(isDanger ? HellcatColors::hellcatRed : 
                       (isMajor ? HellcatColors::hellcatRed.darker(0.5f) : 
                        HellcatColors::panelLight));
            
            g.drawLine(start.x, start.y, end.x, end.y, tickWidth);
        }
    }
    
    void drawArc(juce::Graphics& g, float centerX, float centerY, float radius)
    {
        const float startAngle = -2.356f;
        const float endAngle = 2.356f;
        float valueAngle = startAngle + (currentValue / maxValue) * (endAngle - startAngle);
        
        juce::Path arc;
        arc.addCentredArc(centerX, centerY, radius - 5, radius - 5,
                         0.0f, startAngle, valueAngle, true);
        
        // Draw glow first
        juce::ColourGradient glowGradient(
            HellcatColors::hellcatRed.withAlpha(0.3f), centerX, centerY - radius,
            HellcatColors::redBright.withAlpha(0.3f), centerX, centerY + radius,
            false
        );
        g.setGradientFill(glowGradient);
        g.strokePath(arc, juce::PathStrokeType(15.0f));
        
        // Draw main arc
        juce::ColourGradient arcGradient(
            HellcatColors::redDark, centerX, centerY - radius,
            HellcatColors::redBright, centerX, centerY + radius,
            false
        );
        g.setGradientFill(arcGradient);
        g.strokePath(arc, juce::PathStrokeType(10.0f, juce::PathStrokeType::curved, 
                                                juce::PathStrokeType::rounded));
    }
    
    void drawCarbonFiberCenter(juce::Graphics& g, float centerX, float centerY, float radius)
    {
        auto centerBounds = juce::Rectangle<float>(
            centerX - radius, centerY - radius,
            radius * 2.0f, radius * 2.0f
        );
        
        // Dark base
        g.setColour(HellcatColors::background);
        g.fillEllipse(centerBounds);
        
        // Carbon fiber texture (simplified)
        for (int i = 0; i < 20; ++i)
        {
            g.setColour(juce::Colour(0xff0a0c0f).withAlpha(0.5f));
            g.drawEllipse(centerBounds.reduced(i), 0.5f);
        }
    }
    
    void timerCallback() override
    {
        // Update from parameter attachment here if needed
    }
    
    Type gaugeType;
    juce::String gaugeLabel;
    juce::String subLabel;
    juce::String unitLabel;
    float currentValue = 8.0f;
    float maxValue = 16.0f;
};
