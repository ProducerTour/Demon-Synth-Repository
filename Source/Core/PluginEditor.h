#pragma once

#include <JuceHeader.h>
#include <set>
#include "PluginProcessor.h"
#include "../HellcatUI/HellcatLookAndFeel.h"
#include "../HellcatUI/Components/HellcatTopBar.h"
#include "../HellcatUI/Components/HellcatOscillatorPanel.h"
#include "../HellcatUI/Components/HellcatFilterPanel.h"
#include "../HellcatUI/Components/HellcatEnvelopeDisplay.h"
#include "../HellcatUI/Components/HellcatMacroKnob.h"
#include "../HellcatUI/Components/HellcatXYPad.h"
#include "../HellcatUI/Components/HellcatTransportButton.h"
#include "../HellcatUI/Components/HellcatModMatrix.h"
#include "../HellcatUI/Components/HellcatLFOPanel.h"
#include "../HellcatUI/Components/HellcatTabbedPanel.h"
#include "../HellcatUI/Components/HellcatPresetBrowser.h"
#include "../HellcatUI/Components/HellcatFXPanel.h"
#include "../HellcatUI/Components/HellcatOscilloscope.h"

namespace NulyBeats {

// Envelope preset shape definition
struct EnvelopePreset
{
    const char* name;
    float attack, decay, sustain, release;
    float attackCurve, decayCurve, releaseCurve;
};

// Container component for envelope display that handles its own layout
class EnvelopePanelContainer : public juce::Component
{
public:
    EnvelopePanelContainer(HellcatEnvelopeDisplay& ampDisplay, HellcatEnvelopeDisplay& filterDisplay)
        : ampEnvDisplay(ampDisplay), filterEnvDisplay(filterDisplay)
    {
        addAndMakeVisible(ampEnvDisplay);
        addChildComponent(filterEnvDisplay); // Hidden by default

        // AMP/FILTER sub-tab buttons
        auto setupSubTab = [](juce::TextButton& btn, const juce::String& text, bool active) {
            btn.setButtonText(text);
            btn.setColour(juce::TextButton::buttonColourId, HellcatColors::panelDark);
            btn.setColour(juce::TextButton::buttonOnColourId, HellcatColors::hellcatRed);
            btn.setColour(juce::TextButton::textColourOffId, HellcatColors::textTertiary);
            btn.setColour(juce::TextButton::textColourOnId, HellcatColors::textPrimary);
            btn.setClickingTogglesState(true);
            btn.setRadioGroupId(200);
            btn.setToggleState(active, juce::dontSendNotification);
        };

        setupSubTab(ampButton, "AMP", true);
        setupSubTab(filterButton, "FILTER", false);

        ampButton.onClick = [this]() { showAmpEnvelope(); };
        filterButton.onClick = [this]() { showFilterEnvelope(); };
        addAndMakeVisible(ampButton);
        addAndMakeVisible(filterButton);

        // Filter env amount knob
        envAmtSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        envAmtSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        envAmtSlider.setRange(-1.0, 1.0, 0.01);
        envAmtSlider.setTooltip("Filter Envelope Amount");
        addAndMakeVisible(envAmtSlider);

        envAmtLabel.setText("AMT", juce::dontSendNotification);
        envAmtLabel.setJustificationType(juce::Justification::centred);
        envAmtLabel.setColour(juce::Label::textColourId, HellcatColors::textTertiary);
        addAndMakeVisible(envAmtLabel);

        // ENV enable button
        enableButton.setButtonText("ENV");
        enableButton.setColour(juce::TextButton::buttonColourId, HellcatColors::panelDark);
        enableButton.setColour(juce::TextButton::buttonOnColourId, HellcatColors::hellcatRed);
        enableButton.setColour(juce::TextButton::textColourOffId, HellcatColors::textTertiary);
        enableButton.setColour(juce::TextButton::textColourOnId, HellcatColors::textPrimary);
        enableButton.setClickingTogglesState(true);
        enableButton.setToggleState(true, juce::dontSendNotification);
        enableButton.onClick = [this]() {
            if (onEnableChanged)
                onEnableChanged(enableButton.getToggleState());
        };
        addAndMakeVisible(enableButton);

        // Preset shape buttons
        const char* presetNames[] = {"PLUCK", "PAD", "PERC", "KEYS", "BRASS"};
        for (int i = 0; i < 5; ++i)
        {
            auto* btn = new juce::TextButton(presetNames[i]);
            btn->setColour(juce::TextButton::buttonColourId, HellcatColors::panelDark);
            btn->setColour(juce::TextButton::textColourOffId, HellcatColors::textSecondary);
            btn->onClick = [this, i]() { applyPreset(i); };
            addAndMakeVisible(btn);
            presetButtons.add(btn);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10, 8);

        // Top row: AMP / FILTER / AMT knob / [spacer] / ENV
        auto topRow = bounds.removeFromTop(28);

        ampButton.setBounds(topRow.removeFromLeft(55).reduced(2));
        filterButton.setBounds(topRow.removeFromLeft(65).reduced(2));

        // AMT knob + label
        auto amtArea = topRow.removeFromLeft(50);
        envAmtLabel.setBounds(amtArea.removeFromLeft(22).withHeight(28));
        envAmtSlider.setBounds(amtArea.withHeight(28));

        enableButton.setBounds(topRow.removeFromRight(50).reduced(2));

        // Preset row
        bounds.removeFromTop(4);
        auto presetRow = bounds.removeFromTop(24);
        int presetWidth = presetRow.getWidth() / 5;
        for (auto* btn : presetButtons)
            btn->setBounds(presetRow.removeFromLeft(presetWidth).reduced(2));

        // Envelope display area
        bounds.removeFromTop(4);
        ampEnvDisplay.setBounds(bounds);
        filterEnvDisplay.setBounds(bounds);
    }

    void setEnvelopeEnabled(bool enabled)
    {
        enableButton.setToggleState(enabled, juce::dontSendNotification);
    }

    juce::Slider& getEnvAmtSlider() { return envAmtSlider; }
    bool isShowingAmp() const { return showingAmp; }

    std::function<void(bool)> onEnableChanged;
    std::function<void(int, float, float, float, float, float, float, float)> onPresetApplied;

private:
    void showAmpEnvelope()
    {
        showingAmp = true;
        ampEnvDisplay.setVisible(true);
        filterEnvDisplay.setVisible(false);
    }

    void showFilterEnvelope()
    {
        showingAmp = false;
        ampEnvDisplay.setVisible(false);
        filterEnvDisplay.setVisible(true);
    }

    void applyPreset(int index)
    {
        static const EnvelopePreset presets[] = {
            {"PLUCK",  0.001f, 0.15f, 0.0f,  0.1f,  -5.0f, 4.0f, 4.0f},
            {"PAD",    1.5f,   0.5f,  0.8f,  2.0f,  -2.0f, 2.0f, 2.0f},
            {"PERC",   0.001f, 0.3f,  0.0f,  0.05f, -6.0f, 5.0f, 5.0f},
            {"KEYS",   0.01f,  0.3f,  0.6f,  0.4f,  -3.0f, 3.0f, 3.0f},
            {"BRASS",  0.05f,  0.1f,  0.9f,  0.15f, -2.0f, 2.0f, 2.0f}
        };

        if (index < 0 || index >= 5) return;
        const auto& p = presets[index];

        // 0 = amp, 1 = filter
        int target = showingAmp ? 0 : 1;
        if (onPresetApplied)
            onPresetApplied(target, p.attack, p.decay, p.sustain, p.release,
                           p.attackCurve, p.decayCurve, p.releaseCurve);
    }

    HellcatEnvelopeDisplay& ampEnvDisplay;
    HellcatEnvelopeDisplay& filterEnvDisplay;
    bool showingAmp = true;

    juce::TextButton ampButton;
    juce::TextButton filterButton;
    juce::Slider envAmtSlider;
    juce::Label envAmtLabel;
    juce::TextButton enableButton;
    juce::OwnedArray<juce::TextButton> presetButtons;
};

// Container component for LFO panels that handles its own layout
class LFOPanelContainer : public juce::Component
{
public:
    LFOPanelContainer(HellcatLFOPanel& lfo1, HellcatLFOPanel& lfo2) : lfo1Panel(lfo1), lfo2Panel(lfo2)
    {
        addAndMakeVisible(lfo1Panel);
        addAndMakeVisible(lfo2Panel);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);
        int panelWidth = bounds.getWidth() / 2;
        lfo1Panel.setBounds(bounds.removeFromLeft(panelWidth).reduced(5));
        lfo2Panel.setBounds(bounds.reduced(5));
    }

private:
    HellcatLFOPanel& lfo1Panel;
    HellcatLFOPanel& lfo2Panel;
};

/**
 * Main plugin editor/GUI - Hellcat Dashboard Theme
 */
class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::Timer,
                     private juce::MidiKeyboardState::Listener
{
public:
    explicit PluginEditor(PluginProcessor&);
    ~PluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Keyboard input
    bool keyPressed(const juce::KeyPress& key) override;
    bool keyStateChanged(bool isKeyDown) override;

private:
    void timerCallback() override;

    // MidiKeyboardState::Listener
    void handleNoteOn(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;

    PluginProcessor& processor;

    // Hellcat Look and Feel
    HellcatLookAndFeel hellcatLookAndFeel;

    // ===== Top Bar =====
    HellcatTopBar topBar;

    // ===== Left Panel - Oscillator =====
    HellcatOscillatorPanel oscillatorPanel;

    // ===== Right Panel - Filter =====
    HellcatFilterPanel filterPanel;

    // ===== Center Tabbed Panel =====
    HellcatTabbedPanel tabbedPanel;

    // Tab content components
    std::unique_ptr<HellcatModMatrix> modMatrixPanel;
    std::unique_ptr<EnvelopePanelContainer> envelopePanel;
    std::unique_ptr<LFOPanelContainer> lfoPanel;
    std::unique_ptr<HellcatFXPanel> fxPanel;

    // Envelope displays within envelope panel
    HellcatEnvelopeDisplay ampEnvelopeDisplay;
    HellcatEnvelopeDisplay filterEnvelopeDisplay;

    // LFO panels
    HellcatLFOPanel lfo1Panel{"LFO 1"};
    HellcatLFOPanel lfo2Panel{"LFO 2"};

    // ===== Bottom Section =====
    HellcatXYPad xyPad{"WIDTH", "FX SEND"};
    HellcatOscilloscope oscilloscope;

    // Macro knobs
    HellcatMacroKnob boostKnob{"BOOST"};
    HellcatMacroKnob airKnob{"AIR"};
    HellcatMacroKnob bodyKnob{"BODY"};
    HellcatMacroKnob warpKnob{"WARP"};

    // Engine Start button (enables FX)
    HellcatPushToStartButton engineStartButton;

    // MIDI keyboard
    juce::MidiKeyboardState keyboardState;
    juce::MidiKeyboardComponent keyboardComponent{keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard};
    int baseOctave = 4;
    std::set<int> heldKeys;

    // Computer keyboard -> MIDI note mapping
    int getNoteFroKeyCode(int keyCode) const;

    // Preset browser
    HellcatPresetBrowser presetBrowser;
    bool presetBrowserVisible = false;
    void showPresetBrowser();
    void hidePresetBrowser();
    void updatePresetBrowserCategories();
    void updatePresetBrowserPresets(const juce::String& category);
    void selectNextPreset();
    void selectPrevPreset();
    int currentPresetIndex = 0;
    std::vector<std::pair<juce::String, juce::String>> allPresetsFlat; // category, name pairs

    // Legacy preset browser (compatibility)
    juce::ComboBox presetBox;
    void loadPresetList();
    void loadPreset();

    // Attachments
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;

    // MIDI learn right-click attachment for sliders
    struct MidiLearnListener : public juce::MouseListener
    {
        MidiLearnListener(const juce::String& pid, PluginProcessor& p)
            : paramId(pid), processor(p) {}

        void mouseDown(const juce::MouseEvent& e) override
        {
            if (!e.mods.isRightButtonDown()) return;

            juce::PopupMenu menu;
            int cc = processor.getMidiLearn().getCCForParam(paramId);
            if (cc >= 0)
                menu.addItem(1, "Mapped to CC " + juce::String(cc), false, false);
            menu.addItem(2, "MIDI Learn");
            if (cc >= 0)
                menu.addItem(3, "Clear MIDI Mapping");

            menu.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
                if (result == 2)
                    processor.getMidiLearn().startLearning(paramId);
                else if (result == 3)
                    processor.getMidiLearn().clearMapping(paramId);
            });
        }

        juce::String paramId;
        PluginProcessor& processor;
    };

    void attachMidiLearn(juce::Slider& slider, const juce::String& paramId)
    {
        auto listener = std::make_unique<MidiLearnListener>(paramId, processor);
        slider.addMouseListener(listener.get(), false);
        midiLearnListeners.push_back(std::move(listener));
    }

    std::vector<std::unique_ptr<MidiLearnListener>> midiLearnListeners;

    // Tooltip window for hover help
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;

    // Cached values to avoid unnecessary repaints
    float lastUnisonVoices = -1.0f;
    int lastOscWave = -1;
    float lastCutoffHz = -1.0f;
    int lastFilterType = -1;
    float lastSpread = -1.0f, lastReverbMix = -1.0f;
    float lastAmpA = -1, lastAmpD = -1, lastAmpS = -1, lastAmpR = -1;
    float lastAtkCurve = -99, lastDecCurve = -99, lastRelCurve = -99;
    float lastFiltA = -1, lastFiltD = -1, lastFiltS = -1, lastFiltR = -1;
    int lastOsc2Wave = -1;
    float lastOsc2Level = -1.0f;
    bool lastOsc1Enabled = false;
    bool lastOsc2Enabled = false;
    int lastVoiceMode = -1;
    bool lastReverbEnabled = false, lastDelayEnabled = false, lastChorusEnabled = false;
    bool lastGlideAlways = false;
    bool lastLfo1Sync = false, lastLfo2Sync = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};

} // namespace NulyBeats
