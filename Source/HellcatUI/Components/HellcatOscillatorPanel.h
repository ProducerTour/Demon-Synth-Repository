#pragma once
#include <JuceHeader.h>
#include "../HellcatLookAndFeel.h"

class HellcatWaveformButton : public juce::Button
{
public:
    HellcatWaveformButton(const juce::String& name)
        : juce::Button(name)
    {
        setClickingTogglesState(true);
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                    bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2);

        if (getToggleState())
        {
            // Active state - red fill
            juce::ColourGradient activeGradient(
                HellcatColors::hellcatRed, bounds.getX(), bounds.getY(),
                HellcatColors::redDark, bounds.getX(), bounds.getBottom(),
                false
            );
            g.setGradientFill(activeGradient);
            g.fillRoundedRectangle(bounds, 6.0f);

            g.setColour(juce::Colours::white);
        }
        else
        {
            // Inactive state
            g.setColour(HellcatColors::panelLight);
            g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
            g.setColour(HellcatColors::textSecondary);
        }

        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(getButtonText(), bounds, juce::Justification::centred);
    }
};

class HellcatOscillatorPanel : public juce::Component
{
public:
    HellcatOscillatorPanel()
    {
        // OSC1/OSC2 sub-tab buttons
        auto setupSubTab = [](juce::TextButton& btn, const juce::String& text, bool active) {
            btn.setButtonText(text);
            btn.setColour(juce::TextButton::buttonColourId, HellcatColors::panelDark);
            btn.setColour(juce::TextButton::buttonOnColourId, HellcatColors::hellcatRed);
            btn.setColour(juce::TextButton::textColourOffId, HellcatColors::textTertiary);
            btn.setColour(juce::TextButton::textColourOnId, HellcatColors::textPrimary);
            btn.setClickingTogglesState(true);
            btn.setRadioGroupId(300);
            btn.setToggleState(active, juce::dontSendNotification);
        };

        setupSubTab(osc1Button, "OSC1", true);
        setupSubTab(osc2Button, "OSC2", false);
        osc1Button.onClick = [this]() { switchToOsc(0); };
        osc2Button.onClick = [this]() { switchToOsc(1); };
        addAndMakeVisible(osc1Button);
        addAndMakeVisible(osc2Button);

        // OSC1 enable toggle (shown when OSC1 tab is active)
        auto setupEnableButton = [](juce::TextButton& btn) {
            btn.setButtonText("OFF");
            btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff252830));
            btn.setColour(juce::TextButton::buttonOnColourId, HellcatColors::hellcatRed);
            btn.setColour(juce::TextButton::textColourOffId, HellcatColors::textSecondary);
            btn.setColour(juce::TextButton::textColourOnId, HellcatColors::textPrimary);
            btn.setClickingTogglesState(true);
        };

        setupEnableButton(osc1EnableButton);
        osc1EnableButton.onClick = [this]() {
            osc1EnableButton.setButtonText(osc1EnableButton.getToggleState() ? "ON" : "OFF");
            if (onOsc1EnabledChange)
                onOsc1EnabledChange(osc1EnableButton.getToggleState());
        };
        addAndMakeVisible(osc1EnableButton); // Shown when OSC1 is showing

        // OSC2 enable toggle (shown when OSC2 tab is active)
        setupEnableButton(osc2EnableButton);
        osc2EnableButton.onClick = [this]() {
            osc2EnableButton.setButtonText(osc2EnableButton.getToggleState() ? "ON" : "OFF");
            if (onOsc2EnabledChange)
                onOsc2EnabledChange(osc2EnableButton.getToggleState());
        };
        addChildComponent(osc2EnableButton); // Hidden when OSC1 is showing

        // Waveform buttons
        sawButton.setButtonText("SAW");
        sawButton.setRadioGroupId(100);
        sawButton.setToggleState(true, juce::dontSendNotification);
        addAndMakeVisible(sawButton);

        sqrButton.setButtonText("SQR");
        sqrButton.setRadioGroupId(100);
        addAndMakeVisible(sqrButton);

        triButton.setButtonText("TRI");
        triButton.setRadioGroupId(100);
        addAndMakeVisible(triButton);

        sinButton.setButtonText("SIN");
        sinButton.setRadioGroupId(100);
        addAndMakeVisible(sinButton);

        // Button callbacks route through currentOsc
        sawButton.onClick = [this]() { fireWaveformChange(0); };
        sqrButton.onClick = [this]() { fireWaveformChange(1); };
        triButton.onClick = [this]() { fireWaveformChange(2); };
        sinButton.onClick = [this]() { fireWaveformChange(3); };

        // Noise level knob
        noiseSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        noiseSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        noiseSlider.setRange(0.0, 1.0, 0.01);
        noiseSlider.setTooltip("Noise Level");
        addAndMakeVisible(noiseSlider);

        noiseLabel.setText("NSE", juce::dontSendNotification);
        noiseLabel.setJustificationType(juce::Justification::centred);
        noiseLabel.setColour(juce::Label::textColourId, HellcatColors::textTertiary);
        noiseLabel.setFont(juce::Font(9.0f, juce::Font::bold));
        addAndMakeVisible(noiseLabel);

        // Unison detune knob (visible only when OSC1 is selected)
        detuneSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        detuneSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        detuneSlider.setRange(0.0, 100.0, 0.1);
        detuneSlider.setTooltip("Unison Detune (cents)");
        addAndMakeVisible(detuneSlider);

        detuneLabel.setText("DET", juce::dontSendNotification);
        detuneLabel.setJustificationType(juce::Justification::centred);
        detuneLabel.setColour(juce::Label::textColourId, HellcatColors::textTertiary);
        detuneLabel.setFont(juce::Font(9.0f, juce::Font::bold));
        addAndMakeVisible(detuneLabel);

        // Per-oscillator pitch/pan knobs
        auto setupSmallKnob = [](juce::Slider& slider, const juce::String& tooltip) {
            slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            slider.setTooltip(tooltip);
        };
        auto setupKnobLabel = [](juce::Label& label, const juce::String& text) {
            label.setText(text, juce::dontSendNotification);
            label.setJustificationType(juce::Justification::centred);
            label.setColour(juce::Label::textColourId, HellcatColors::textTertiary);
            label.setFont(juce::Font(8.0f, juce::Font::bold));
        };

        // OSC1 pitch/pan
        setupSmallKnob(osc1OctaveSlider, "Osc 1 Octave (-3 to +3)");
        osc1OctaveSlider.setRange(-3.0, 3.0, 1.0);
        addAndMakeVisible(osc1OctaveSlider);
        setupKnobLabel(osc1OctaveLabel, "OCT");
        addAndMakeVisible(osc1OctaveLabel);

        setupSmallKnob(osc1SemiSlider, "Osc 1 Semitone (-12 to +12)");
        osc1SemiSlider.setRange(-12.0, 12.0, 1.0);
        addAndMakeVisible(osc1SemiSlider);
        setupKnobLabel(osc1SemiLabel, "SEMI");
        addAndMakeVisible(osc1SemiLabel);

        setupSmallKnob(osc1FineSlider, "Osc 1 Fine Tune (-100 to +100 cents)");
        osc1FineSlider.setRange(-100.0, 100.0, 1.0);
        addAndMakeVisible(osc1FineSlider);
        setupKnobLabel(osc1FineLabel, "FINE");
        addAndMakeVisible(osc1FineLabel);

        setupSmallKnob(osc1PanSlider, "Osc 1 Pan (L/R)");
        osc1PanSlider.setRange(-1.0, 1.0, 0.01);
        addAndMakeVisible(osc1PanSlider);
        setupKnobLabel(osc1PanLabel, "PAN");
        addAndMakeVisible(osc1PanLabel);

        // OSC2 pitch/pan (hidden by default)
        setupSmallKnob(osc2OctaveSlider, "Osc 2 Octave (-3 to +3)");
        osc2OctaveSlider.setRange(-3.0, 3.0, 1.0);
        addChildComponent(osc2OctaveSlider);
        setupKnobLabel(osc2OctaveLabel, "OCT");
        addChildComponent(osc2OctaveLabel);

        setupSmallKnob(osc2SemiSlider, "Osc 2 Semitone (-12 to +12)");
        osc2SemiSlider.setRange(-12.0, 12.0, 1.0);
        addChildComponent(osc2SemiSlider);
        setupKnobLabel(osc2SemiLabel, "SEMI");
        addChildComponent(osc2SemiLabel);

        setupSmallKnob(osc2FineSlider, "Osc 2 Fine Tune (-100 to +100 cents)");
        osc2FineSlider.setRange(-100.0, 100.0, 1.0);
        addChildComponent(osc2FineSlider);
        setupKnobLabel(osc2FineLabel, "FINE");
        addChildComponent(osc2FineLabel);

        setupSmallKnob(osc2PanSlider, "Osc 2 Pan (L/R)");
        osc2PanSlider.setRange(-1.0, 1.0, 0.01);
        addChildComponent(osc2PanSlider);
        setupKnobLabel(osc2PanLabel, "PAN");
        addChildComponent(osc2PanLabel);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Panel background with carbon fiber texture
        drawPanelCarbonFiber(g, bounds, 12.0f);

        // Chrome/metallic border
        juce::ColourGradient borderGradient(
            juce::Colour(0xff4a4a4a), bounds.getX(), bounds.getY(),
            juce::Colour(0xff2a2a2a), bounds.getRight(), bounds.getBottom(),
            true
        );
        g.setGradientFill(borderGradient);
        g.drawRoundedRectangle(bounds.reduced(1), 12.0f, 2.0f);

        // Inner border highlight
        g.setColour(HellcatColors::panelLight.withAlpha(0.3f));
        g.drawRoundedRectangle(bounds.reduced(3), 10.0f, 1.0f);

        // Title with Orbitron font
        g.setColour(HellcatColors::textSecondary);
        if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
            g.setFont(lf->getOrbitronFont(11.0f));
        else
            g.setFont(juce::Font(11.0f, juce::Font::bold));

        juce::String title = (currentOsc == 0) ? "OSCILLATOR 1" : "OSCILLATOR 2";
        g.drawText(title, bounds.removeFromTop(25), juce::Justification::centred);

        // Draw the gauge
        drawGauge(g);
    }

    void drawPanelCarbonFiber(juce::Graphics& g, juce::Rectangle<float> bounds, float cornerSize)
    {
        // Dark gradient base - like brushed dark metal
        juce::ColourGradient baseGradient(
            juce::Colour(0xff141414), bounds.getX(), bounds.getY(),
            juce::Colour(0xff0a0a0a), bounds.getX(), bounds.getBottom(),
            false
        );
        g.setGradientFill(baseGradient);
        g.fillRoundedRectangle(bounds, cornerSize);

        // Save graphics state and clip to rounded rect
        g.saveState();
        juce::Path clipPath;
        clipPath.addRoundedRectangle(bounds, cornerSize);
        g.reduceClipRegion(clipPath);

        // Carbon fiber texture overlay
        if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
        {
            auto& img = lf->getCarbonFiberImage();
            if (img.isValid())
            {
                g.setOpacity(0.45f);
                g.drawImage(img, bounds,
                           juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination);
                g.setOpacity(1.0f);
            }
        }

        g.restoreState();

        // Subtle vignette/shadow around edges
        juce::ColourGradient vignetteGradient(
            juce::Colours::transparentBlack, bounds.getCentreX(), bounds.getCentreY(),
            juce::Colour(0x40000000), bounds.getX(), bounds.getY(),
            true
        );
        g.setGradientFill(vignetteGradient);
        g.fillRoundedRectangle(bounds, cornerSize);

        // Top edge highlight
        juce::ColourGradient topHighlight(
            juce::Colour(0x15ffffff), bounds.getX(), bounds.getY(),
            juce::Colours::transparentWhite, bounds.getX(), bounds.getY() + 30,
            false
        );
        g.setGradientFill(topHighlight);
        g.fillRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), 30, cornerSize);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(25); // Title

        // Sub-tab row
        auto subTabRow = bounds.removeFromTop(28).reduced(10, 2);
        osc1Button.setBounds(subTabRow.removeFromLeft(55).reduced(2));
        osc2Button.setBounds(subTabRow.removeFromLeft(55).reduced(2));
        osc1EnableButton.setBounds(subTabRow.removeFromRight(40).reduced(2));
        osc2EnableButton.setBounds(osc1EnableButton.getBounds()); // Same position

        // Pitch/pan row — 4 small knobs with labels below
        auto pitchRow = bounds.removeFromTop(42).reduced(15, 0);
        int knobWidth = pitchRow.getWidth() / 4;
        auto layoutKnobWithLabel = [&](juce::Slider& slider, juce::Label& label, juce::Rectangle<int> area) {
            label.setBounds(area.removeFromBottom(12));
            slider.setBounds(area);
        };
        // OSC1 pitch knobs
        layoutKnobWithLabel(osc1OctaveSlider, osc1OctaveLabel, pitchRow.removeFromLeft(knobWidth).reduced(2));
        layoutKnobWithLabel(osc1SemiSlider, osc1SemiLabel, pitchRow.removeFromLeft(knobWidth).reduced(2));
        layoutKnobWithLabel(osc1FineSlider, osc1FineLabel, pitchRow.removeFromLeft(knobWidth).reduced(2));
        layoutKnobWithLabel(osc1PanSlider, osc1PanLabel, pitchRow.reduced(2));
        // OSC2 pitch knobs — same positions (shown/hidden based on tab)
        pitchRow = bounds.withY(bounds.getY() - 42).removeFromTop(42).reduced(15, 0);
        // recalculate since we consumed pitchRow
        auto pitchRow2 = getLocalBounds();
        pitchRow2.removeFromTop(25 + 28); // title + sub-tab
        pitchRow2 = pitchRow2.removeFromTop(42).reduced(15, 0);
        int knobWidth2 = pitchRow2.getWidth() / 4;
        layoutKnobWithLabel(osc2OctaveSlider, osc2OctaveLabel, pitchRow2.removeFromLeft(knobWidth2).reduced(2));
        layoutKnobWithLabel(osc2SemiSlider, osc2SemiLabel, pitchRow2.removeFromLeft(knobWidth2).reduced(2));
        layoutKnobWithLabel(osc2FineSlider, osc2FineLabel, pitchRow2.removeFromLeft(knobWidth2).reduced(2));
        layoutKnobWithLabel(osc2PanSlider, osc2PanLabel, pitchRow2.reduced(2));

        // Button area at bottom
        auto buttonArea = bounds.removeFromBottom(40).reduced(15, 5);
        int buttonWidth = buttonArea.getWidth() / 4;
        sawButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(3));
        sqrButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(3));
        triButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(3));
        sinButton.setBounds(buttonArea.reduced(3));

        // Noise + detune row above waveform buttons
        auto noiseRow = bounds.removeFromBottom(28).reduced(static_cast<int>(bounds.getWidth() * 0.15f), 0);
        int halfRow = noiseRow.getWidth() / 2;
        auto noiseArea = noiseRow.removeFromLeft(halfRow);
        noiseLabel.setBounds(noiseArea.removeFromLeft(24));
        noiseSlider.setBounds(noiseArea);
        auto detuneArea = noiseRow;
        detuneLabel.setBounds(detuneArea.removeFromLeft(24));
        detuneSlider.setBounds(detuneArea);

        // Gauge area - remaining center
        gaugeBounds = bounds;
    }

    void setValue(float newValue)
    {
        if (currentOsc == 0)
        {
            osc1GaugeValue = newValue;
            currentValue = newValue;
            maxValue = 8.0f;
        }
        repaint();
    }

    void setWaveform(int waveform)
    {
        osc1Waveform = waveform;
        if (currentOsc == 0)
        {
            sawButton.setToggleState(waveform == 0, juce::dontSendNotification);
            sqrButton.setToggleState(waveform == 1, juce::dontSendNotification);
            triButton.setToggleState(waveform == 2, juce::dontSendNotification);
            sinButton.setToggleState(waveform == 3, juce::dontSendNotification);
        }
    }

    void setOsc2Waveform(int waveform)
    {
        osc2Waveform = waveform;
        if (currentOsc == 1)
        {
            sawButton.setToggleState(waveform == 0, juce::dontSendNotification);
            sqrButton.setToggleState(waveform == 1, juce::dontSendNotification);
            triButton.setToggleState(waveform == 2, juce::dontSendNotification);
            sinButton.setToggleState(waveform == 3, juce::dontSendNotification);
        }
    }

    void setOsc2Level(float level)
    {
        osc2GaugeValue = level * 100.0f; // Store as 0-100 for gauge display
        if (currentOsc == 1)
        {
            currentValue = osc2GaugeValue;
            repaint();
        }
    }

    void setOsc1Enabled(bool enabled)
    {
        osc1EnableButton.setToggleState(enabled, juce::dontSendNotification);
        osc1EnableButton.setButtonText(enabled ? "ON" : "OFF");
    }

    void setOsc2Enabled(bool enabled)
    {
        osc2EnableButton.setToggleState(enabled, juce::dontSendNotification);
        osc2EnableButton.setButtonText(enabled ? "ON" : "OFF");
    }

    juce::Slider& getNoiseSlider() { return noiseSlider; }
    juce::Slider& getDetuneSlider() { return detuneSlider; }

    // Per-osc pitch/pan sliders for SliderAttachment
    juce::Slider& getOsc1OctaveSlider() { return osc1OctaveSlider; }
    juce::Slider& getOsc1SemiSlider() { return osc1SemiSlider; }
    juce::Slider& getOsc1FineSlider() { return osc1FineSlider; }
    juce::Slider& getOsc1PanSlider() { return osc1PanSlider; }
    juce::Slider& getOsc2OctaveSlider() { return osc2OctaveSlider; }
    juce::Slider& getOsc2SemiSlider() { return osc2SemiSlider; }
    juce::Slider& getOsc2FineSlider() { return osc2FineSlider; }
    juce::Slider& getOsc2PanSlider() { return osc2PanSlider; }

    // Callbacks
    std::function<void(int)> onWaveformChange;      // OSC1 waveform (kept for backward compat)
    std::function<void(int)> onOsc2WaveformChange;   // OSC2 waveform
    std::function<void(float)> onGaugeValueChange;   // OSC1 gauge (unison voices)
    std::function<void(float)> onOsc2GaugeValueChange; // OSC2 gauge (level 0-1)
    std::function<void(bool)> onOsc1EnabledChange;   // OSC1 enable toggle
    std::function<void(bool)> onOsc2EnabledChange;   // OSC2 enable toggle

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (gaugeBounds.contains(e.getPosition()))
        {
            isDraggingGauge = true;
            dragStartY = e.y;
            dragStartValue = currentValue;
            setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (isDraggingGauge)
        {
            if (currentOsc == 0)
            {
                // OSC1: unison voices (integer 1-8)
                float delta = (dragStartY - e.y) / 30.0f;
                float newValue = juce::jlimit(1.0f, 8.0f, dragStartValue + delta);
                newValue = std::round(newValue);
                if (newValue != currentValue)
                {
                    currentValue = newValue;
                    osc1GaugeValue = newValue;
                    repaint();
                    if (onGaugeValueChange)
                        onGaugeValueChange(currentValue);
                }
            }
            else
            {
                // OSC2: level (0-100%)
                float delta = (dragStartY - e.y) / 2.0f; // 2px per percent
                float newValue = juce::jlimit(0.0f, 100.0f, dragStartValue + delta);
                if (std::abs(newValue - currentValue) > 0.5f)
                {
                    currentValue = newValue;
                    osc2GaugeValue = newValue;
                    repaint();
                    if (onOsc2GaugeValueChange)
                        onOsc2GaugeValueChange(currentValue / 100.0f); // Send as 0-1
                }
            }
        }
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        isDraggingGauge = false;
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        bool overGauge = gaugeBounds.contains(e.getPosition());
        if (overGauge != gaugeHovered)
        {
            gaugeHovered = overGauge;
            setMouseCursor(overGauge ? juce::MouseCursor::UpDownResizeCursor
                                     : juce::MouseCursor::NormalCursor);
            repaint();
        }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (gaugeHovered)
        {
            gaugeHovered = false;
            setMouseCursor(juce::MouseCursor::NormalCursor);
            repaint();
        }
    }

private:
    void switchToOsc(int osc)
    {
        if (currentOsc == osc) return;
        currentOsc = osc;

        osc1EnableButton.setVisible(osc == 0);
        osc2EnableButton.setVisible(osc == 1);

        // Show/hide per-osc pitch knobs
        bool showOsc1 = (osc == 0);
        osc1OctaveSlider.setVisible(showOsc1); osc1OctaveLabel.setVisible(showOsc1);
        osc1SemiSlider.setVisible(showOsc1);   osc1SemiLabel.setVisible(showOsc1);
        osc1FineSlider.setVisible(showOsc1);   osc1FineLabel.setVisible(showOsc1);
        osc1PanSlider.setVisible(showOsc1);    osc1PanLabel.setVisible(showOsc1);

        osc2OctaveSlider.setVisible(!showOsc1); osc2OctaveLabel.setVisible(!showOsc1);
        osc2SemiSlider.setVisible(!showOsc1);   osc2SemiLabel.setVisible(!showOsc1);
        osc2FineSlider.setVisible(!showOsc1);   osc2FineLabel.setVisible(!showOsc1);
        osc2PanSlider.setVisible(!showOsc1);    osc2PanLabel.setVisible(!showOsc1);

        // Detune only visible on OSC1
        detuneSlider.setVisible(showOsc1);
        detuneLabel.setVisible(showOsc1);

        if (osc == 0)
        {
            currentValue = osc1GaugeValue;
            maxValue = 8.0f;
            sawButton.setToggleState(osc1Waveform == 0, juce::dontSendNotification);
            sqrButton.setToggleState(osc1Waveform == 1, juce::dontSendNotification);
            triButton.setToggleState(osc1Waveform == 2, juce::dontSendNotification);
            sinButton.setToggleState(osc1Waveform == 3, juce::dontSendNotification);
        }
        else
        {
            currentValue = osc2GaugeValue;
            maxValue = 100.0f;
            sawButton.setToggleState(osc2Waveform == 0, juce::dontSendNotification);
            sqrButton.setToggleState(osc2Waveform == 1, juce::dontSendNotification);
            triButton.setToggleState(osc2Waveform == 2, juce::dontSendNotification);
            sinButton.setToggleState(osc2Waveform == 3, juce::dontSendNotification);
        }

        repaint();
    }

    void fireWaveformChange(int waveIndex)
    {
        if (currentOsc == 0)
        {
            osc1Waveform = waveIndex;
            if (onWaveformChange) onWaveformChange(waveIndex);
        }
        else
        {
            osc2Waveform = waveIndex;
            if (onOsc2WaveformChange) onOsc2WaveformChange(waveIndex);
        }
    }

    void drawGauge(juce::Graphics& g)
    {
        auto centerX = static_cast<float>(gaugeBounds.getCentreX());
        auto centerY = static_cast<float>(gaugeBounds.getCentreY());
        auto radius = juce::jmin(gaugeBounds.getWidth(), gaugeBounds.getHeight()) * 0.38f;

        // === Outer chrome bezel with multiple rings (like Hellcat) ===
        // Outer glow ring (red when active, brighter on hover)
        float glowIntensity = currentValue / maxValue;
        float hoverBoost = gaugeHovered ? 0.15f : 0.0f;
        g.setColour(HellcatColors::hellcatRed.withAlpha(0.15f + glowIntensity * 0.2f + hoverBoost));
        g.drawEllipse(centerX - radius - 12, centerY - radius - 12,
                     (radius + 12) * 2, (radius + 12) * 2, gaugeHovered ? 10.0f : 8.0f);

        // Chrome bezel - outer ring
        juce::ColourGradient bezelGradient(
            juce::Colour(0xff3a3a3a), centerX - radius, centerY - radius,
            juce::Colour(0xff1a1a1a), centerX + radius, centerY + radius,
            true
        );
        g.setGradientFill(bezelGradient);
        g.drawEllipse(centerX - radius - 4, centerY - radius - 4,
                     (radius + 4) * 2, (radius + 4) * 2, 4.0f);

        // Inner bezel highlight
        g.setColour(juce::Colour(0xff4a4a4a));
        g.drawEllipse(centerX - radius - 2, centerY - radius - 2,
                     (radius + 2) * 2, (radius + 2) * 2, 1.0f);

        // Gauge background - dark with subtle gradient
        juce::ColourGradient bgGradient(
            juce::Colour(0xff0a0a0a), centerX, centerY - radius,
            juce::Colour(0xff050505), centerX, centerY + radius,
            false
        );
        g.setGradientFill(bgGradient);
        g.fillEllipse(centerX - radius, centerY - radius, radius * 2, radius * 2);

        // Draw tick marks and numbers
        drawTickMarks(g, centerX, centerY, radius);

        // Draw value arc with glow
        drawArc(g, centerX, centerY, radius - 10);

        // === Center section with carbon fiber texture ===
        float innerRadius = radius * 0.65f;

        // Carbon fiber background
        drawCarbonFiber(g, centerX, centerY, innerRadius);

        // Inner ring (like speaker grille surround)
        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawEllipse(centerX - innerRadius, centerY - innerRadius,
                     innerRadius * 2, innerRadius * 2, 2.0f);

        // Draw value text - use Sofachrome font for racing dashboard style
        g.setColour(HellcatColors::textPrimary);
        if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
            g.setFont(lf->getSofachromeFont(currentOsc == 0 ? 48.0f : 36.0f));
        else
            g.setFont(juce::Font(currentOsc == 0 ? 48.0f : 36.0f, juce::Font::bold));

        auto textBounds = juce::Rectangle<float>(
            centerX - innerRadius, centerY - 25,
            innerRadius * 2.0f, 50
        );

        if (currentOsc == 0)
        {
            // OSC1: integer unison voices
            g.drawText(juce::String(static_cast<int>(currentValue)), textBounds.toNearestInt(),
                       juce::Justification::centred);
        }
        else
        {
            // OSC2: percentage level
            g.drawText(juce::String(juce::roundToInt(currentValue)) + "%", textBounds.toNearestInt(),
                       juce::Justification::centred);
        }

        // Draw labels below value
        g.setColour(HellcatColors::textSecondary);
        if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
            g.setFont(lf->getOrbitronFont(11.0f));
        else
            g.setFont(juce::Font(11.0f, juce::Font::bold));

        auto labelBounds = juce::Rectangle<float>(
            centerX - 50, centerY + 20, 100, 20
        );
        g.drawText(currentOsc == 0 ? "VOICES" : "LEVEL", labelBounds.toNearestInt(), juce::Justification::centred);

        g.setColour(HellcatColors::textTertiary);
        if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
            g.setFont(lf->getOrbitronFont(9.0f));
        else
            g.setFont(juce::Font(9.0f, juce::Font::plain));
        labelBounds.translate(0, 14);
        g.drawText(currentOsc == 0 ? "UNISON" : "OSC 2", labelBounds.toNearestInt(), juce::Justification::centred);
    }

    void drawCarbonFiber(juce::Graphics& g, float centerX, float centerY, float radius)
    {
        // Dark background
        g.setColour(juce::Colour(0xff0c0c0c));
        g.fillEllipse(centerX - radius, centerY - radius, radius * 2, radius * 2);

        // Draw carbon fiber texture image
        if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
        {
            auto& img = lf->getCarbonFiberImage();
            if (img.isValid())
            {
                g.saveState();
                juce::Path clipCircle;
                clipCircle.addEllipse(centerX - radius, centerY - radius, radius * 2, radius * 2);
                g.reduceClipRegion(clipCircle);

                auto destBounds = juce::Rectangle<float>(
                    centerX - radius, centerY - radius, radius * 2, radius * 2);
                g.drawImage(img, destBounds,
                           juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination);

                g.restoreState();
            }
        }
    }

    void drawTickMarks(juce::Graphics& g, float centerX, float centerY, float radius)
    {
        const float startAngle = -2.356f;
        const float endAngle = 2.356f;

        int numMajorTicks = (currentOsc == 0) ? 9 : 11; // 0-8 or 0-100 by 10s
        int dangerStart = (currentOsc == 0) ? 6 : 9;

        for (int i = 0; i < numMajorTicks; ++i)
        {
            float angle = startAngle + (i / static_cast<float>(numMajorTicks - 1)) * (endAngle - startAngle);

            float tickLength = 14.0f;
            float tickWidth = 2.5f;

            auto tickStart = radius - 3.0f;
            auto tickEnd = tickStart - tickLength;

            juce::Point<float> start(
                centerX + std::cos(angle) * tickStart,
                centerY + std::sin(angle) * tickStart
            );

            juce::Point<float> end(
                centerX + std::cos(angle) * tickEnd,
                centerY + std::sin(angle) * tickEnd
            );

            bool isDanger = (i >= dangerStart);
            g.setColour(isDanger ? HellcatColors::hellcatRed : HellcatColors::textSecondary);
            g.drawLine(start.x, start.y, end.x, end.y, tickWidth);

            // Draw numbers next to major ticks
            auto numberRadius = tickEnd - 12.0f;
            juce::Point<float> numberPos(
                centerX + std::cos(angle) * numberRadius,
                centerY + std::sin(angle) * numberRadius
            );

            g.setColour(isDanger ? HellcatColors::hellcatRed : HellcatColors::textPrimary);
            if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
                g.setFont(lf->getOrbitronFont(currentOsc == 0 ? 12.0f : 9.0f));
            else
                g.setFont(juce::Font(currentOsc == 0 ? 12.0f : 9.0f, juce::Font::bold));

            juce::String tickLabel = (currentOsc == 0) ? juce::String(i) : juce::String(i * 10);
            g.drawText(tickLabel,
                      juce::Rectangle<float>(numberPos.x - 12, numberPos.y - 8, 24, 16).toNearestInt(),
                      juce::Justification::centred);
        }

        // Draw minor tick marks between major ticks
        int numMinorTicks = (currentOsc == 0) ? 17 : 21;
        for (int i = 0; i < numMinorTicks; ++i)
        {
            if (i % 2 == 0) continue;

            float angle = startAngle + (i / static_cast<float>(numMinorTicks - 1)) * (endAngle - startAngle);

            float tickLength = 6.0f;
            float tickWidth = 1.0f;

            auto tickStart = radius - 3.0f;
            auto tickEnd = tickStart - tickLength;

            juce::Point<float> start(
                centerX + std::cos(angle) * tickStart,
                centerY + std::sin(angle) * tickStart
            );

            juce::Point<float> end(
                centerX + std::cos(angle) * tickEnd,
                centerY + std::sin(angle) * tickEnd
            );

            g.setColour(HellcatColors::panelLight);
            g.drawLine(start.x, start.y, end.x, end.y, tickWidth);
        }
    }

    void drawArc(juce::Graphics& g, float centerX, float centerY, float radius)
    {
        const float startAngle = -2.356f;
        const float endAngle = 2.356f;
        float valueAngle = startAngle + (currentValue / maxValue) * (endAngle - startAngle);

        juce::Path arc;
        arc.addCentredArc(centerX, centerY, radius, radius,
                         0.0f, startAngle, valueAngle, true);

        // Draw glow
        g.setColour(HellcatColors::hellcatRed.withAlpha(0.3f));
        g.strokePath(arc, juce::PathStrokeType(12.0f));

        // Draw main arc with gradient
        juce::ColourGradient arcGradient(
            HellcatColors::redDark, centerX, centerY - radius,
            HellcatColors::redBright, centerX, centerY + radius,
            false
        );
        g.setGradientFill(arcGradient);
        g.strokePath(arc, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    }

    // Sub-tab state
    int currentOsc = 0; // 0=OSC1, 1=OSC2
    int osc1Waveform = 0;
    int osc2Waveform = 1; // Default: square (maps to UI index 1)
    float osc1GaugeValue = 1.0f;
    float osc2GaugeValue = 100.0f; // Stored as 0-100

    // Sub-tab buttons
    juce::TextButton osc1Button;
    juce::TextButton osc2Button;
    juce::TextButton osc1EnableButton;
    juce::TextButton osc2EnableButton;

    // Waveform buttons
    HellcatWaveformButton sawButton{"SAW"};
    HellcatWaveformButton sqrButton{"SQR"};
    HellcatWaveformButton triButton{"TRI"};
    HellcatWaveformButton sinButton{"SIN"};

    // Noise knob
    juce::Slider noiseSlider;
    juce::Label noiseLabel;

    // Detune knob (unison)
    juce::Slider detuneSlider;
    juce::Label detuneLabel;

    // Per-oscillator pitch/pan knobs
    juce::Slider osc1OctaveSlider, osc1SemiSlider, osc1FineSlider, osc1PanSlider;
    juce::Label osc1OctaveLabel, osc1SemiLabel, osc1FineLabel, osc1PanLabel;
    juce::Slider osc2OctaveSlider, osc2SemiSlider, osc2FineSlider, osc2PanSlider;
    juce::Label osc2OctaveLabel, osc2SemiLabel, osc2FineLabel, osc2PanLabel;

    // Gauge
    juce::Rectangle<int> gaugeBounds;
    float currentValue = 1.0f;
    float maxValue = 8.0f;

    // Drag state for interactive gauge
    bool isDraggingGauge = false;
    bool gaugeHovered = false;
    float dragStartY = 0.0f;
    float dragStartValue = 0.0f;
};
