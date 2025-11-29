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

namespace NulyBeats {

// Container component for envelope display that handles its own layout
class EnvelopePanelContainer : public juce::Component
{
public:
    EnvelopePanelContainer(HellcatEnvelopeDisplay& envDisplay) : envelopeDisplay(envDisplay)
    {
        addAndMakeVisible(envelopeDisplay);
    }

    void resized() override
    {
        envelopeDisplay.setBounds(getLocalBounds().reduced(15));
    }

private:
    HellcatEnvelopeDisplay& envelopeDisplay;
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

    // Envelope display within envelope panel
    HellcatEnvelopeDisplay ampEnvelopeDisplay;

    // LFO panels
    HellcatLFOPanel lfo1Panel{"LFO 1"};
    HellcatLFOPanel lfo2Panel{"LFO 2"};

    // ===== Bottom Section =====
    HellcatXYPad xyPad{"WIDTH", "FX SEND"};

    // Macro knobs
    HellcatMacroKnob boostKnob{"BOOST"};
    HellcatMacroKnob airKnob{"AIR"};
    HellcatMacroKnob bodyKnob{"BODY"};
    HellcatMacroKnob warpKnob{"WARP"};

    // Control buttons (ARM/LOCK/NITRO)
    HellcatArmButton armButton;
    HellcatLockButton lockButton;
    HellcatNitroButton nitroButton;

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};

} // namespace NulyBeats
