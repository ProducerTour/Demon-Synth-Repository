#pragma once
#include <JuceHeader.h>
#include "../HellcatLookAndFeel.h"

/**
 * Single effect module with enable toggle and 2-4 parameter knobs
 */
class HellcatFXModule : public juce::Component
{
public:
    HellcatFXModule(const juce::String& name, const juce::StringArray& knobNames)
        : title(name)
    {
        // Enable toggle
        enableButton.setButtonText("ON");
        enableButton.setColour(juce::TextButton::buttonColourId, HellcatColors::panelDark);
        enableButton.setColour(juce::TextButton::buttonOnColourId, HellcatColors::hellcatRed);
        enableButton.setColour(juce::TextButton::textColourOffId, HellcatColors::textTertiary);
        enableButton.setColour(juce::TextButton::textColourOnId, HellcatColors::textPrimary);
        enableButton.setClickingTogglesState(true);
        enableButton.onClick = [this]() {
            if (onEnableChanged)
                onEnableChanged(enableButton.getToggleState());
            repaint(); // Redraw border glow
        };
        addAndMakeVisible(enableButton);

        // Create knobs
        numKnobs = juce::jmin(knobNames.size(), 4);
        for (int i = 0; i < numKnobs; ++i)
        {
            knobs[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            knobs[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            knobs[i].setTooltip(title + " " + knobNames[i]);
            addAndMakeVisible(knobs[i]);

            labels[i].setText(knobNames[i], juce::dontSendNotification);
            labels[i].setJustificationType(juce::Justification::centred);
            labels[i].setColour(juce::Label::textColourId, HellcatColors::textTertiary);
            labels[i].setFont(juce::Font(9.0f, juce::Font::bold));
            addAndMakeVisible(labels[i]);
        }
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(3);

        // Background
        g.setColour(HellcatColors::panelDark);
        g.fillRoundedRectangle(bounds, 8.0f);

        // Border — red glow when enabled
        if (enableButton.getToggleState())
        {
            g.setColour(HellcatColors::hellcatRed.withAlpha(0.4f));
            g.drawRoundedRectangle(bounds, 8.0f, 2.0f);
        }
        else
        {
            g.setColour(HellcatColors::panelLight);
            g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
        }

        // Title
        g.setColour(enableButton.getToggleState() ? HellcatColors::hellcatRed : HellcatColors::textSecondary);
        if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
            g.setFont(lf->getOrbitronFont(11.0f));
        else
            g.setFont(juce::Font(11.0f, juce::Font::bold));

        g.drawText(title, bounds.removeFromTop(24).reduced(8, 0), juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(6);

        // Title row with enable button
        auto titleRow = bounds.removeFromTop(24);
        enableButton.setBounds(titleRow.removeFromRight(36).reduced(2));

        bounds.removeFromTop(4);

        // Knobs in a row
        int knobWidth = numKnobs > 0 ? bounds.getWidth() / numKnobs : bounds.getWidth();
        for (int i = 0; i < numKnobs; ++i)
        {
            auto col = bounds.removeFromLeft(knobWidth);
            labels[i].setBounds(col.removeFromBottom(14));
            knobs[i].setBounds(col.reduced(4, 2));
        }
    }

    juce::Slider& getKnob(int index) { return knobs[index]; }
    void setEnabled(bool enabled)
    {
        enableButton.setToggleState(enabled, juce::dontSendNotification);
        repaint();
    }

    std::function<void(bool)> onEnableChanged;

private:
    juce::String title;
    juce::TextButton enableButton;
    juce::Slider knobs[4];
    juce::Label labels[4];
    int numKnobs = 0;
};

/**
 * FX rack panel with 4 effect modules in a 2x2 grid
 */
class HellcatFXPanel : public juce::Component
{
public:
    HellcatFXPanel()
        : reverbModule("REVERB", {"MIX", "SIZE", "DAMP"}),
          delayModule("DELAY", {"MIX", "TIME", "FB"}),
          chorusModule("CHORUS", {"MIX", "RATE", "DEPTH"}),
          flangerModule("FLANGER", {"MIX", "RATE", "DEPTH", "FB"})
    {
        addAndMakeVisible(reverbModule);
        addAndMakeVisible(delayModule);
        addAndMakeVisible(chorusModule);
        addAndMakeVisible(flangerModule);

        // Forward enable callbacks
        reverbModule.onEnableChanged = [this](bool en) { if (onReverbEnableChanged) onReverbEnableChanged(en); };
        delayModule.onEnableChanged = [this](bool en) { if (onDelayEnableChanged) onDelayEnableChanged(en); };
        chorusModule.onEnableChanged = [this](bool en) { if (onChorusEnableChanged) onChorusEnableChanged(en); };
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(HellcatColors::background);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(8);
        int halfW = bounds.getWidth() / 2;
        int halfH = bounds.getHeight() / 2;

        reverbModule.setBounds(bounds.getX(), bounds.getY(), halfW, halfH);
        delayModule.setBounds(bounds.getX() + halfW, bounds.getY(), halfW, halfH);
        chorusModule.setBounds(bounds.getX(), bounds.getY() + halfH, halfW, halfH);
        flangerModule.setBounds(bounds.getX() + halfW, bounds.getY() + halfH, halfW, halfH);
    }

    // Reverb slider accessors
    juce::Slider& getReverbMixSlider() { return reverbModule.getKnob(0); }
    juce::Slider& getReverbSizeSlider() { return reverbModule.getKnob(1); }
    juce::Slider& getReverbDampingSlider() { return reverbModule.getKnob(2); }

    // Delay slider accessors
    juce::Slider& getDelayMixSlider() { return delayModule.getKnob(0); }
    juce::Slider& getDelayTimeSlider() { return delayModule.getKnob(1); }
    juce::Slider& getDelayFeedbackSlider() { return delayModule.getKnob(2); }

    // Chorus slider accessors
    juce::Slider& getChorusMixSlider() { return chorusModule.getKnob(0); }
    juce::Slider& getChorusRateSlider() { return chorusModule.getKnob(1); }
    juce::Slider& getChorusDepthSlider() { return chorusModule.getKnob(2); }

    // Flanger slider accessors
    juce::Slider& getFlangerMixSlider() { return flangerModule.getKnob(0); }
    juce::Slider& getFlangerRateSlider() { return flangerModule.getKnob(1); }
    juce::Slider& getFlangerDepthSlider() { return flangerModule.getKnob(2); }
    juce::Slider& getFlangerFeedbackSlider() { return flangerModule.getKnob(3); }

    // Enable state setters (for timer sync)
    void setReverbEnabled(bool en) { reverbModule.setEnabled(en); }
    void setDelayEnabled(bool en) { delayModule.setEnabled(en); }
    void setChorusEnabled(bool en) { chorusModule.setEnabled(en); }

    // Enable callbacks
    std::function<void(bool)> onReverbEnableChanged;
    std::function<void(bool)> onDelayEnableChanged;
    std::function<void(bool)> onChorusEnableChanged;

private:
    HellcatFXModule reverbModule;
    HellcatFXModule delayModule;
    HellcatFXModule chorusModule;
    HellcatFXModule flangerModule;
};
