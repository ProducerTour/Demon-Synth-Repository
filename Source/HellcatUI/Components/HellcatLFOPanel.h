#pragma once
#include <JuceHeader.h>
#include "../HellcatLookAndFeel.h"

class HellcatLFOPanel : public juce::Component
{
public:
    HellcatLFOPanel(const juce::String& name) : title(name)
    {
        // Wave type buttons (styled button group like oscillator panel)
        const char* waveNames[] = {"SIN", "TRI", "SAW", "SQR", "S&H"};
        for (int i = 0; i < 5; ++i)
        {
            waveButtons[i].setButtonText(waveNames[i]);
            waveButtons[i].setRadioGroupId(1);
            waveButtons[i].setClickingTogglesState(true);
            waveButtons[i].onClick = [this, i]() {
                selectedWave = i;
                repaint();
                if (onWaveChange) onWaveChange(i);
            };
            addAndMakeVisible(waveButtons[i]);
        }
        waveButtons[0].setToggleState(true, juce::dontSendNotification);

        // Rate knob
        rateSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        rateSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
        rateSlider.setRange(0.1, 20.0, 0.1);
        rateSlider.setValue(2.5);
        rateSlider.setTextValueSuffix(" Hz");
        addAndMakeVisible(rateSlider);

        // Sync toggle button
        syncButton.setButtonText("SYNC");
        syncButton.setColour(juce::TextButton::buttonColourId, HellcatColors::panelDark);
        syncButton.setColour(juce::TextButton::buttonOnColourId, HellcatColors::hellcatRed);
        syncButton.setColour(juce::TextButton::textColourOffId, HellcatColors::textTertiary);
        syncButton.setColour(juce::TextButton::textColourOnId, HellcatColors::textPrimary);
        syncButton.setClickingTogglesState(true);
        syncButton.setTooltip("Sync LFO rate to host tempo (scales with BPM, baseline 120)");
        syncButton.onClick = [this]() {
            if (onSyncChange) onSyncChange(syncButton.getToggleState());
        };
        addAndMakeVisible(syncButton);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Dark panel background with border
        juce::ColourGradient bgGradient(
            juce::Colour(0xff0f1114), 0, 0,
            juce::Colour(0xff0a0c0f), 0, bounds.getHeight(),
            false
        );
        g.setGradientFill(bgGradient);
        g.fillRoundedRectangle(bounds, 8.0f);

        g.setColour(HellcatColors::panelLight);
        g.drawRoundedRectangle(bounds, 8.0f, 1.0f);

        // Title in red with Orbitron font
        g.setColour(HellcatColors::hellcatRed);
        if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
            g.setFont(lf->getOrbitronFont(14.0f));
        else
            g.setFont(juce::Font(14.0f, juce::Font::bold));
        g.drawText(title, titleBounds, juce::Justification::centred);

        // Draw waveform visualization
        drawWaveform(g, waveformBounds);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);

        // Title at top
        titleBounds = bounds.removeFromTop(25);

        // Waveform display
        waveformBounds = bounds.removeFromTop(70).reduced(10, 5);

        // Wave buttons
        auto buttonBounds = bounds.removeFromTop(30).reduced(5, 0);
        int buttonWidth = buttonBounds.getWidth() / 5;
        for (int i = 0; i < 5; ++i)
        {
            waveButtons[i].setBounds(buttonBounds.removeFromLeft(buttonWidth).reduced(2));
        }

        bounds.removeFromTop(10);

        // Rate slider + sync button
        auto rateBounds = bounds.reduced(10, 0);
        syncButton.setBounds(rateBounds.removeFromBottom(24).reduced(10, 2));
        rateSlider.setBounds(rateBounds);
    }

    juce::Slider& getRateSlider() { return rateSlider; }
    juce::TextButton& getSyncButton() { return syncButton; }
    int getSelectedWave() const { return selectedWave; }

    void setSyncState(bool synced)
    {
        syncButton.setToggleState(synced, juce::dontSendNotification);
    }

    std::function<void(int)> onWaveChange;
    std::function<void(bool)> onSyncChange;

private:
    void drawWaveform(juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        if (bounds.isEmpty()) return;

        // Background for waveform display
        g.setColour(HellcatColors::background);
        g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
        g.setColour(HellcatColors::panelLight);
        g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 1.0f);

        // Draw wave shape based on selected type
        juce::Path wave;
        float w = static_cast<float>(bounds.getWidth());
        float h = static_cast<float>(bounds.getHeight());
        float cx = static_cast<float>(bounds.getX());
        float cy = static_cast<float>(bounds.getCentreY());
        float amp = h * 0.35f;

        switch (selectedWave)
        {
            case 0: // Sine
                wave.startNewSubPath(cx, cy);
                for (int i = 0; i <= static_cast<int>(w); ++i)
                {
                    float x = cx + static_cast<float>(i);
                    float y = cy - std::sin(static_cast<float>(i) / w * juce::MathConstants<float>::twoPi * 2.0f) * amp;
                    wave.lineTo(x, y);
                }
                break;

            case 1: // Triangle
                wave.startNewSubPath(cx, cy);
                wave.lineTo(cx + w * 0.25f, cy - amp);
                wave.lineTo(cx + w * 0.5f, cy);
                wave.lineTo(cx + w * 0.75f, cy + amp);
                wave.lineTo(cx + w, cy);
                break;

            case 2: // Saw
                wave.startNewSubPath(cx, cy + amp);
                wave.lineTo(cx + w * 0.5f, cy - amp);
                wave.lineTo(cx + w * 0.5f, cy + amp);
                wave.lineTo(cx + w, cy - amp);
                break;

            case 3: // Square
                wave.startNewSubPath(cx, cy - amp);
                wave.lineTo(cx + w * 0.25f, cy - amp);
                wave.lineTo(cx + w * 0.25f, cy + amp);
                wave.lineTo(cx + w * 0.5f, cy + amp);
                wave.lineTo(cx + w * 0.5f, cy - amp);
                wave.lineTo(cx + w * 0.75f, cy - amp);
                wave.lineTo(cx + w * 0.75f, cy + amp);
                wave.lineTo(cx + w, cy + amp);
                break;

            case 4: // S&H (Sample & Hold)
                wave.startNewSubPath(cx, cy);
                for (int i = 0; i < 8; ++i)
                {
                    float stepWidth = w / 8.0f;
                    float stepHeight = (std::sin(static_cast<float>(i) * 2.3f) * 0.8f + 0.1f) * amp;
                    float x1 = cx + static_cast<float>(i) * stepWidth;
                    float x2 = cx + static_cast<float>(i + 1) * stepWidth;
                    wave.lineTo(x1, cy - stepHeight);
                    wave.lineTo(x2, cy - stepHeight);
                }
                break;
        }

        // Draw wave with glow
        g.setColour(HellcatColors::hellcatRed.withAlpha(0.4f));
        g.strokePath(wave, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour(HellcatColors::hellcatRed);
        g.strokePath(wave, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    juce::String title;
    juce::TextButton waveButtons[5];
    juce::Slider rateSlider;
    juce::TextButton syncButton;
    juce::Rectangle<int> titleBounds;
    juce::Rectangle<int> waveformBounds;
    int selectedWave = 0;
};
