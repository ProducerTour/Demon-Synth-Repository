#pragma once
#include <JuceHeader.h>
#include "../HellcatLookAndFeel.h"

// Binary data
namespace BinaryData
{
    extern const char* square_png;
    extern const int square_pngSize;
}

class HellcatTopBar : public juce::Component,
                      private juce::Timer
{
public:
    HellcatTopBar()
    {
        // Load NullyBeats logo from binary resources
        logoImage = juce::ImageCache::getFromMemory(BinaryData::square_png,
                                                     BinaryData::square_pngSize);

        // Voice mode buttons
        polyButton.setButtonText("POLY");
        polyButton.setRadioGroupId(1);
        polyButton.setToggleState(true, juce::dontSendNotification);
        polyButton.onClick = [this]() { if (onVoiceModeChange) onVoiceModeChange(0); };
        addAndMakeVisible(polyButton);

        monoButton.setButtonText("MONO");
        monoButton.setRadioGroupId(1);
        monoButton.onClick = [this]() { if (onVoiceModeChange) onVoiceModeChange(1); };
        addAndMakeVisible(monoButton);

        legatoButton.setButtonText("LEGATO");
        legatoButton.setRadioGroupId(1);
        legatoButton.onClick = [this]() { if (onVoiceModeChange) onVoiceModeChange(2); };
        addAndMakeVisible(legatoButton);

        // Glide knob
        glideSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        glideSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        glideSlider.setRange(0.0, 2.0, 0.01);
        glideSlider.setTooltip("Glide Time");
        addAndMakeVisible(glideSlider);

        glideLabel.setText("GLIDE", juce::dontSendNotification);
        glideLabel.setJustificationType(juce::Justification::centred);
        glideLabel.setColour(juce::Label::textColourId, HellcatColors::textTertiary);
        glideLabel.setFont(juce::Font(8.0f, juce::Font::bold));
        addAndMakeVisible(glideLabel);

        // Glide Always toggle
        glideAlwaysButton.setButtonText("ALW");
        glideAlwaysButton.setColour(juce::TextButton::buttonColourId, HellcatColors::panelDark);
        glideAlwaysButton.setColour(juce::TextButton::buttonOnColourId, HellcatColors::hellcatRed);
        glideAlwaysButton.setColour(juce::TextButton::textColourOffId, HellcatColors::textTertiary);
        glideAlwaysButton.setColour(juce::TextButton::textColourOnId, HellcatColors::textPrimary);
        glideAlwaysButton.setClickingTogglesState(true);
        glideAlwaysButton.setTooltip("Glide Always - portamento on every note");
        glideAlwaysButton.onClick = [this]() {
            if (onGlideAlwaysChange) onGlideAlwaysChange(glideAlwaysButton.getToggleState());
        };
        addAndMakeVisible(glideAlwaysButton);

        // Preset display button (shows current preset name, clickable to open browser)
        presetButton.setButtonText("Select Preset");
        presetButton.setColour(juce::TextButton::buttonColourId, HellcatColors::panelDark);
        presetButton.setColour(juce::TextButton::buttonOnColourId, HellcatColors::panelDark);
        presetButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        presetButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        presetButton.onClick = [this]() {
            if (onPresetLabelClicked)
                onPresetLabelClicked();
        };
        addAndMakeVisible(presetButton);

        // Browser button
        browserButton.setButtonText("...");
        browserButton.setColour(juce::TextButton::buttonColourId, HellcatColors::panelDark);
        browserButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        browserButton.onClick = [this]() {
            if (onBrowserButtonClicked)
                onBrowserButtonClicked();
        };
        addAndMakeVisible(browserButton);

        // Previous/Next buttons
        prevButton.setButtonText("<");
        prevButton.setColour(juce::TextButton::buttonColourId, HellcatColors::panelDark);
        prevButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        prevButton.onClick = [this]() {
            if (onPrevPreset)
                onPrevPreset();
        };
        addAndMakeVisible(prevButton);

        nextButton.setButtonText(">");
        nextButton.setColour(juce::TextButton::buttonColourId, HellcatColors::panelDark);
        nextButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        nextButton.onClick = [this]() {
            if (onNextPreset)
                onNextPreset();
        };
        addAndMakeVisible(nextButton);

        // Legacy preset combo (hidden, for compatibility)
        presetCombo.setVisible(false);
        addChildComponent(presetCombo);
        presetCombo.onChange = [this]() {
            if (onPresetChange)
                onPresetChange(presetCombo.getSelectedId(), presetCombo.getText());
        };

        // Animate meter
        startTimerHz(10);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        // Background gradient
        juce::ColourGradient bgGradient(
            HellcatColors::panelDark, 0, 0,
            juce::Colour(0xff0a0c0f), 0, static_cast<float>(bounds.getHeight()),
            false
        );
        g.setGradientFill(bgGradient);
        g.fillRect(bounds);

        // Bottom border
        g.setColour(HellcatColors::panelLight);
        g.drawLine(0, static_cast<float>(bounds.getBottom() - 1),
                   static_cast<float>(bounds.getRight()), static_cast<float>(bounds.getBottom() - 1), 1.0f);

        // Draw NullyBeats logo image
        float logoHeight = 44.0f;
        float logoWidth = 44.0f;  // Square logo
        float logoX = static_cast<float>(logoBounds.getX());
        float logoY = static_cast<float>(logoBounds.getCentreY()) - logoHeight / 2.0f;

        if (logoImage.isValid())
        {
            logoWidth = logoHeight * (static_cast<float>(logoImage.getWidth()) / logoImage.getHeight());
            g.drawImage(logoImage, logoX, logoY, logoWidth, logoHeight,
                        0, 0, logoImage.getWidth(), logoImage.getHeight());
        }

        // Logo text - use italic Sofachrome font for DEMON text, positioned right of logo
        g.setColour(juce::Colours::white);
        if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
            g.setFont(lf->getSofachromeItalicFont(20.0f));
        else
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 20.0f, juce::Font::italic));

        // Position text to the right of the logo image with 8px gap
        float textX = logoX + logoWidth + 8.0f;
        auto textBounds = logoBounds.toFloat().withLeft(textX);
        g.drawText("DEMON", textBounds, juce::Justification::centredLeft);

        // Output meter
        drawOutputMeter(g, meterBounds);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(25, 0);

        // Logo (left) - give more space for NullyBeats logo + DEMON text
        logoBounds = bounds.removeFromLeft(220);

        // Output meter (right)
        meterBounds = bounds.removeFromRight(100);

        // Voice mode + glide (right)
        auto modeBounds = bounds.removeFromRight(300).reduced(0, 12);
        // Glide Always toggle on the far right
        glideAlwaysButton.setBounds(modeBounds.removeFromRight(36).reduced(2));
        // Glide knob
        auto glideBounds = modeBounds.removeFromRight(50);
        glideLabel.setBounds(glideBounds.removeFromBottom(12));
        glideSlider.setBounds(glideBounds);
        // Voice mode buttons
        auto buttonWidth = modeBounds.getWidth() / 3;
        polyButton.setBounds(modeBounds.removeFromLeft(buttonWidth).reduced(2));
        monoButton.setBounds(modeBounds.removeFromLeft(buttonWidth).reduced(2));
        legatoButton.setBounds(modeBounds.reduced(2));

        // Preset browser area (center)
        auto presetArea = bounds.withSizeKeepingCentre(280, 32);
        prevButton.setBounds(presetArea.removeFromLeft(30));
        browserButton.setBounds(presetArea.removeFromRight(30));
        nextButton.setBounds(presetArea.removeFromRight(30));
        presetButton.setBounds(presetArea);
    }

    void setPresets(const juce::StringArray& presetNames)
    {
        presetCombo.clear();
        for (int i = 0; i < presetNames.size(); ++i)
            presetCombo.addItem(presetNames[i], i + 1);
        if (presetNames.size() > 0)
            presetCombo.setSelectedId(1);
    }

    void addPresetSection(const juce::String& sectionName, const juce::StringArray& presets, int startId)
    {
        presetCombo.addSectionHeading(sectionName);
        for (int i = 0; i < presets.size(); ++i)
            presetCombo.addItem(presets[i], startId + i);
    }

    juce::ComboBox& getPresetCombo() { return presetCombo; }

    void setCurrentPresetName(const juce::String& name)
    {
        presetButton.setButtonText(name);
    }

    std::function<void(int id, const juce::String& name)> onPresetChange;
    std::function<void()> onBrowserButtonClicked;
    std::function<void()> onPresetLabelClicked;
    std::function<void()> onPrevPreset;
    std::function<void()> onNextPreset;
    std::function<void(int)> onVoiceModeChange;
    std::function<void(bool)> onGlideAlwaysChange;

    void setGlideAlways(bool always)
    {
        glideAlwaysButton.setToggleState(always, juce::dontSendNotification);
    }

    void setVoiceMode(int mode)
    {
        polyButton.setToggleState(mode == 0, juce::dontSendNotification);
        monoButton.setToggleState(mode == 1, juce::dontSendNotification);
        legatoButton.setToggleState(mode == 2, juce::dontSendNotification);
    }

    juce::Slider& getGlideSlider() { return glideSlider; }

    /** Set the real RMS level from the audio processor (0.0 - 1.0) */
    void setRmsLevel(float rms)
    {
        currentRms = juce::jlimit(0.0f, 1.0f, rms);
    }

private:
    void timerCallback() override
    {
        // Convert RMS (0.0-1.0) to meter bar count (0-10)
        // Use a dB-like mapping for more musical response
        float dbLevel = (currentRms > 0.0001f) ? 20.0f * std::log10(currentRms) : -100.0f;
        // Map -60dB..0dB to 0..10 bars
        int target = static_cast<int>(juce::jmap(dbLevel, -60.0f, 0.0f, 0.0f, 10.0f));
        target = juce::jlimit(0, 10, target);

        // Smooth falloff for visual appeal
        if (target >= meterLevel)
            meterLevel = target;
        else
            meterLevel = std::max(0, meterLevel - 1);

        repaint(meterBounds);
    }

    void drawDemonIcon(juce::Graphics& g, float x, float y, float width, float height)
    {
        juce::Path demon;

        // Demon head/horns shape - stylized like the Dodge Demon logo
        float centerX = x + width * 0.5f;

        // Left horn
        demon.startNewSubPath(x + width * 0.15f, y + height * 0.7f);
        demon.quadraticTo(x + width * 0.1f, y + height * 0.3f,
                         x + width * 0.25f, y);
        demon.quadraticTo(x + width * 0.35f, y + height * 0.25f,
                         x + width * 0.4f, y + height * 0.5f);

        // Head curve (left to center)
        demon.quadraticTo(x + width * 0.45f, y + height * 0.65f,
                         centerX, y + height * 0.7f);

        // Head curve (center to right)
        demon.quadraticTo(x + width * 0.55f, y + height * 0.65f,
                         x + width * 0.6f, y + height * 0.5f);

        // Right horn
        demon.quadraticTo(x + width * 0.65f, y + height * 0.25f,
                         x + width * 0.75f, y);
        demon.quadraticTo(x + width * 0.9f, y + height * 0.3f,
                         x + width * 0.85f, y + height * 0.7f);

        // Bottom of head
        demon.quadraticTo(x + width * 0.7f, y + height * 0.9f,
                         centerX, y + height);
        demon.quadraticTo(x + width * 0.3f, y + height * 0.9f,
                         x + width * 0.15f, y + height * 0.7f);

        demon.closeSubPath();

        // Fill with gradient
        juce::ColourGradient demonGradient(
            HellcatColors::redBright, x, y,
            HellcatColors::hellcatRed, x, y + height,
            false
        );
        g.setGradientFill(demonGradient);
        g.fillPath(demon);

        // Glow effect
        g.setColour(HellcatColors::hellcatRed.withAlpha(0.5f));
        g.strokePath(demon, juce::PathStrokeType(2.0f));

        // Eyes - two glowing dots
        float eyeY = y + height * 0.55f;
        float eyeSize = width * 0.08f;
        g.setColour(juce::Colours::white);
        g.fillEllipse(x + width * 0.35f - eyeSize/2, eyeY - eyeSize/2, eyeSize, eyeSize);
        g.fillEllipse(x + width * 0.65f - eyeSize/2, eyeY - eyeSize/2, eyeSize, eyeSize);
    }

    void drawLightningBolt(juce::Graphics& g, float x, float y, float width, float height)
    {
        juce::Path bolt;

        // Lightning bolt shape
        bolt.startNewSubPath(x + width * 0.6f, y);
        bolt.lineTo(x + width * 0.3f, y + height * 0.45f);
        bolt.lineTo(x + width * 0.55f, y + height * 0.45f);
        bolt.lineTo(x + width * 0.4f, y + height);
        bolt.lineTo(x + width * 0.7f, y + height * 0.55f);
        bolt.lineTo(x + width * 0.45f, y + height * 0.55f);
        bolt.closeSubPath();

        // Fill with gradient
        juce::ColourGradient boltGradient(
            HellcatColors::redBright, x, y,
            HellcatColors::hellcatRed, x, y + height,
            false
        );
        g.setGradientFill(boltGradient);
        g.fillPath(bolt);

        // Glow
        g.setColour(HellcatColors::hellcatRed.withAlpha(0.4f));
        g.strokePath(bolt, juce::PathStrokeType(2.0f));
    }

    void drawOutputMeter(juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        const int numBars = 10;
        const int barWidth = 4;
        const int gap = 3;

        auto x = static_cast<float>(bounds.getRight() - (numBars * (barWidth + gap)));
        auto y = static_cast<float>(bounds.getCentreY() - 15);

        for (int i = 0; i < numBars; ++i)
        {
            bool isActive = i < meterLevel;
            float barHeight = 8.0f + (i * 2.2f);

            if (isActive)
            {
                // Green to red gradient based on level
                juce::Colour barColor;
                if (i < 5)
                    barColor = juce::Colour(0xff4CAF50); // Green
                else if (i < 7)
                    barColor = juce::Colour(0xffFFC107); // Yellow
                else
                    barColor = HellcatColors::hellcatRed; // Red

                g.setColour(barColor);
            }
            else
            {
                g.setColour(HellcatColors::panelLight);
            }

            g.fillRoundedRectangle(x, y + (30 - barHeight), static_cast<float>(barWidth), barHeight, 1.0f);
            x += barWidth + gap;
        }
    }

    juce::Rectangle<int> logoBounds;
    juce::Rectangle<int> meterBounds;

    juce::TextButton polyButton;
    juce::TextButton monoButton;
    juce::TextButton legatoButton;
    juce::Slider glideSlider;
    juce::Label glideLabel;
    juce::TextButton glideAlwaysButton;
    juce::ComboBox presetCombo;
    juce::TextButton presetButton;
    juce::TextButton browserButton;
    juce::TextButton prevButton;
    juce::TextButton nextButton;

    juce::Image logoImage;
    int meterLevel = 0;
    float currentRms = 0.0f;
};
