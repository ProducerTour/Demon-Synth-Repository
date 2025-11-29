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

        // Preset display label (shows current preset name)
        presetLabel.setText("Select Preset", juce::dontSendNotification);
        presetLabel.setJustificationType(juce::Justification::centred);
        presetLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        presetLabel.setColour(juce::Label::backgroundColourId, HellcatColors::panelDark);
        presetLabel.setColour(juce::Label::outlineColourId, HellcatColors::panelLight);
        addAndMakeVisible(presetLabel);

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

        // Engine mode (right)
        auto modeBounds = bounds.removeFromRight(250).reduced(0, 12);
        auto buttonWidth = modeBounds.getWidth() / 3;
        ecoButton.setBounds(modeBounds.removeFromLeft(buttonWidth).reduced(2));
        sportButton.setBounds(modeBounds.removeFromLeft(buttonWidth).reduced(2));
        trackButton.setBounds(modeBounds.reduced(2));

        // Preset browser area (center)
        auto presetArea = bounds.withSizeKeepingCentre(280, 32);
        prevButton.setBounds(presetArea.removeFromLeft(30));
        browserButton.setBounds(presetArea.removeFromRight(30));
        nextButton.setBounds(presetArea.removeFromRight(30));
        presetLabel.setBounds(presetArea);
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
        presetLabel.setText(name, juce::dontSendNotification);
    }

    std::function<void(int id, const juce::String& name)> onPresetChange;
    std::function<void()> onBrowserButtonClicked;
    std::function<void()> onPrevPreset;
    std::function<void()> onNextPreset;

private:
    void timerCallback() override
    {
        meterLevel = juce::Random::getSystemRandom().nextInt(juce::Range<int>(4, 9));
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

    juce::TextButton ecoButton;
    juce::TextButton sportButton;
    juce::TextButton trackButton;
    juce::ComboBox presetCombo;
    juce::Label presetLabel;
    juce::TextButton browserButton;
    juce::TextButton prevButton;
    juce::TextButton nextButton;

    juce::Image logoImage;
    int meterLevel = 6;
};
