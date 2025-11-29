#include "PluginEditor.h"

namespace NulyBeats {

PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&hellcatLookAndFeel);

    // Window size - 1280x720 as specified
    setSize(1280, 720);
    setResizable(true, true);
    setResizeLimits(1024, 600, 1920, 1080);

    // ===== Top Bar =====
    addAndMakeVisible(topBar);

    // Connect preset browser button to TopBar
    topBar.onBrowserButtonClicked = [this]() {
        if (presetBrowserVisible)
            hidePresetBrowser();
        else
            showPresetBrowser();
    };

    // Clicking preset name also opens browser
    topBar.onPresetLabelClicked = [this]() {
        if (presetBrowserVisible)
            hidePresetBrowser();
        else
            showPresetBrowser();
    };

    topBar.onNextPreset = [this]() { selectNextPreset(); };
    topBar.onPrevPreset = [this]() { selectPrevPreset(); };

    // Legacy preset change (for combo box if ever used)
    topBar.onPresetChange = [this](int id, const juce::String& name) {
        processor.loadSamplePreset(name);
        topBar.setCurrentPresetName(name);
    };

    // Setup preset browser callbacks
    presetBrowser.onCategoryChanged = [this](const juce::String& category) {
        updatePresetBrowserPresets(category);
    };

    presetBrowser.onPresetLoaded = [this](int id, const juce::String& name) {
        DBG("=== Preset browser: loading preset ===");
        DBG("  Preset name: " + name);
        processor.loadSamplePreset(name);
        DBG("  After load, processor preset name: " + processor.getCurrentSamplePresetName());
        topBar.setCurrentPresetName(name);
        // Update current preset index
        for (size_t i = 0; i < allPresetsFlat.size(); ++i) {
            if (allPresetsFlat[i].second == name) {
                currentPresetIndex = static_cast<int>(i);
                break;
            }
        }
        hidePresetBrowser();
    };

    presetBrowser.onClose = [this]() {
        hidePresetBrowser();
    };

    addChildComponent(presetBrowser); // Hidden by default

    // ===== Left Panel - Oscillator Engine =====
    addAndMakeVisible(oscillatorPanel);
    oscillatorPanel.onWaveformChange = [this](int waveIndex) {
        // Map UI waveform index to parameter value
        // UI: 0=SAW, 1=SQR, 2=TRI, 3=SIN
        // Param: 0=Sine, 1=Saw, 2=Square, 3=Triangle, 4=Pulse, 5=Noise
        int paramValue = 0;
        switch (waveIndex) {
            case 0: paramValue = 1; break; // SAW -> Saw
            case 1: paramValue = 2; break; // SQR -> Square
            case 2: paramValue = 3; break; // TRI -> Triangle
            case 3: paramValue = 0; break; // SIN -> Sine
        }
        if (auto* param = processor.getAPVTS().getParameter("osc1_wave"))
            param->setValueNotifyingHost(static_cast<float>(paramValue) / 5.0f); // 6 wave types (0-5)
    };

    // ===== Right Panel - Filter Drive =====
    addAndMakeVisible(filterPanel);
    filterPanel.onFilterTypeChange = [this](int filterIndex) {
        // Map UI filter index to parameter value
        // UI: 0=LP12, 1=LP24, 2=BP, 3=HP
        // Param: 0=LP, 1=HP, 2=BP, 3=Notch
        int paramValue = 0;
        switch (filterIndex) {
            case 0: paramValue = 0; break; // LP12 -> LP
            case 1: paramValue = 0; break; // LP24 -> LP (same type, could add slope param later)
            case 2: paramValue = 2; break; // BP -> BP
            case 3: paramValue = 1; break; // HP -> HP
        }
        if (auto* param = processor.getAPVTS().getParameter("filter_type"))
            param->setValueNotifyingHost(static_cast<float>(paramValue) / 3.0f); // 4 filter types (0-3)
    };

    // ===== Center Tabbed Panel =====
    // Create tab content components
    modMatrixPanel = std::make_unique<HellcatModMatrix>();
    envelopePanel = std::make_unique<EnvelopePanelContainer>(ampEnvelopeDisplay);
    lfoPanel = std::make_unique<LFOPanelContainer>(lfo1Panel, lfo2Panel);

    // Add content to custom tabbed panel (avoids JUCE overflow issues)
    tabbedPanel.setTabContent(0, modMatrixPanel.get());
    tabbedPanel.setTabContent(1, envelopePanel.get());
    tabbedPanel.setTabContent(2, lfoPanel.get());
    tabbedPanel.setCurrentTab(1); // Default to ENVELOPES

    addAndMakeVisible(tabbedPanel);

    // ===== Bottom Section =====
    // XY Pad - connects to stereo width and reverb/delay mix
    addAndMakeVisible(xyPad);
    xyPad.onValueChange = [this](float x, float y) {
        // X axis (WIDTH) - controls unison spread (stereo width)
        if (auto* param = processor.getAPVTS().getParameter("unison_spread"))
            param->setValueNotifyingHost(x);

        // Y axis (FX SEND) - controls reverb mix
        if (auto* param = processor.getAPVTS().getParameter("reverb_mix"))
            param->setValueNotifyingHost(y);
    };

    // Macro Knobs
    addAndMakeVisible(boostKnob);
    addAndMakeVisible(airKnob);
    addAndMakeVisible(bodyKnob);
    addAndMakeVisible(warpKnob);

    // Engine Start Button - enables flanger FX
    addAndMakeVisible(engineStartButton);
    engineStartButton.onClick = [this]() {
        // Toggle flanger effect on/off
        if (auto* param = processor.getAPVTS().getParameter("flanger_enabled"))
            param->setValueNotifyingHost(engineStartButton.getToggleState() ? 1.0f : 0.0f);
    };

    // ===== MIDI Keyboard =====
    addAndMakeVisible(keyboardComponent);
    keyboardComponent.setKeyWidth(22.0f);
    keyboardComponent.setScrollButtonWidth(18);
    keyboardComponent.setAvailableRange(36, 96);
    keyboardComponent.setOctaveForMiddleC(4);
    keyboardComponent.setKeyPressBaseOctave(-1); // Disable built-in keyboard

    // Apply Hellcat colors to keyboard
    keyboardComponent.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, HellcatColors::hellcatRed);
    keyboardComponent.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, HellcatColors::hellcatRed.withAlpha(0.3f));
    keyboardComponent.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xff1a1d22));
    keyboardComponent.setColour(juce::MidiKeyboardComponent::blackNoteColourId, HellcatColors::background);

    // Listen for keyboard state changes
    keyboardState.addListener(this);

    // Enable keyboard focus
    setWantsKeyboardFocus(true);

    // ===== Parameter Attachments =====
    auto& apvts = processor.getAPVTS();

    // Macro knob attachments
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "macro_boost", boostKnob.getSlider()));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "macro_air", airKnob.getSlider()));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "macro_body", bodyKnob.getSlider()));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "macro_warp", warpKnob.getSlider()));

    // LFO rate slider attachments
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "lfo1_rate", lfo1Panel.getRateSlider()));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "lfo2_rate", lfo2Panel.getRateSlider()));

    // LFO wave change callbacks (update parameter manually since we're using button groups)
    lfo1Panel.onWaveChange = [this](int waveIndex) {
        if (auto* param = processor.getAPVTS().getParameter("lfo1_wave"))
            param->setValueNotifyingHost(static_cast<float>(waveIndex) / 6.0f); // 7 wave types (0-6)
    };
    lfo2Panel.onWaveChange = [this](int waveIndex) {
        if (auto* param = processor.getAPVTS().getParameter("lfo2_wave"))
            param->setValueNotifyingHost(static_cast<float>(waveIndex) / 6.0f);
    };

    // Envelope touchpoint callbacks - connect to amp envelope parameters
    ampEnvelopeDisplay.onAttackChanged = [this](float value) {
        if (auto* param = processor.getAPVTS().getParameter("amp_attack"))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    };
    ampEnvelopeDisplay.onDecayChanged = [this](float value) {
        if (auto* param = processor.getAPVTS().getParameter("amp_decay"))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    };
    ampEnvelopeDisplay.onSustainChanged = [this](float value) {
        if (auto* param = processor.getAPVTS().getParameter("amp_sustain"))
            param->setValueNotifyingHost(value); // Sustain is already 0-1
    };
    ampEnvelopeDisplay.onReleaseChanged = [this](float value) {
        if (auto* param = processor.getAPVTS().getParameter("amp_release"))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    };

    // Envelope curve callbacks - control curve tension/shape
    ampEnvelopeDisplay.onAttackCurveChanged = [this](float value) {
        if (auto* param = processor.getAPVTS().getParameter("amp_attack_curve"))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    };
    ampEnvelopeDisplay.onDecayCurveChanged = [this](float value) {
        if (auto* param = processor.getAPVTS().getParameter("amp_decay_curve"))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    };
    ampEnvelopeDisplay.onReleaseCurveChanged = [this](float value) {
        if (auto* param = processor.getAPVTS().getParameter("amp_release_curve"))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    };

    // Envelope enable toggle callback
    envelopePanel->onEnableChanged = [this](bool enabled) {
        if (auto* param = processor.getAPVTS().getParameter("amp_env_enabled"))
            param->setValueNotifyingHost(enabled ? 1.0f : 0.0f);
    };

    // Load presets
    loadPresetList();

    // Build flat preset list for prev/next navigation
    auto& sampleManager = processor.getSamplePresetManager();
    const auto& categories = sampleManager.getCategories();
    for (const auto& cat : categories)
    {
        auto presets = sampleManager.getPresetsInCategory(cat);
        for (const auto& preset : presets)
            allPresetsFlat.push_back({cat, preset.name});
    }

    // Restore preset name display from processor state (for when DAW reopens plugin window)
    juce::String currentPreset = processor.getCurrentSamplePresetName();
    bool sampleLoaded = processor.getSampleSynth().hasSampleLoaded();

    DBG("=== PluginEditor constructor ===");
    DBG("Current preset from processor: '" + currentPreset + "'");
    DBG("Sample loaded in synth: " + juce::String(sampleLoaded ? "YES" : "NO"));
    DBG("allPresetsFlat size: " + juce::String(allPresetsFlat.size()));

    // Write to log file for debugging
    juce::File logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("DemonSynth_debug.log");
    logFile.appendText("\n=== PluginEditor constructor at " + juce::Time::getCurrentTime().toString(true, true) + " ===\n");
    logFile.appendText("Current preset from processor: '" + currentPreset + "'\n");
    logFile.appendText("Sample loaded in synth: " + juce::String(sampleLoaded ? "YES" : "NO") + "\n");

    if (currentPreset.isNotEmpty())
    {
        topBar.setCurrentPresetName(currentPreset);
        DBG("Set topBar preset name to: " + currentPreset);

        // Update currentPresetIndex to match restored preset
        for (size_t i = 0; i < allPresetsFlat.size(); ++i)
        {
            if (allPresetsFlat[i].second == currentPreset)
            {
                currentPresetIndex = static_cast<int>(i);
                DBG("Found preset at index: " + juce::String(currentPresetIndex));
                break;
            }
        }
    }
    else if (sampleLoaded)
    {
        // Sample is loaded but preset name is empty - this shouldn't happen!
        // Try to get the sample name from the synth directly
        juce::String sampleName = processor.getSampleSynth().getCurrentSampleName();
        DBG("WARNING: Sample loaded but no preset name! Sample name: " + sampleName);
        if (sampleName.isNotEmpty())
        {
            topBar.setCurrentPresetName(sampleName);
        }
    }
    else
    {
        DBG("No preset name stored and no sample loaded - UI will show default");
    }

    // Start timer for updates
    startTimerHz(30);
}

PluginEditor::~PluginEditor()
{
    keyboardState.removeListener(this);
    setLookAndFeel(nullptr);
}

void PluginEditor::paint(juce::Graphics& g)
{
    // Fill background
    g.fillAll(HellcatColors::background);
}

void PluginEditor::resized()
{
    auto bounds = getLocalBounds();

    // Top bar - 60px
    topBar.setBounds(bounds.removeFromTop(60));

    // Bottom - keyboard 50px
    auto keyboardBounds = bounds.removeFromBottom(50);
    keyboardComponent.setBounds(keyboardBounds.reduced(20, 0));

    // Bottom section - 110px (XY pad, macros, buttons)
    auto bottomSection = bounds.removeFromBottom(110);
    bottomSection.reduce(20, 5);

    // XY Pad - left side
    xyPad.setBounds(bottomSection.removeFromLeft(140));

    // Engine Start button - right side (circular, needs square bounds)
    auto buttonBounds = bottomSection.removeFromRight(110);
    buttonBounds.reduce(5, 5);
    // Make it square and centered
    int buttonSize = std::min(buttonBounds.getWidth(), buttonBounds.getHeight());
    engineStartButton.setBounds(
        buttonBounds.getCentreX() - buttonSize / 2,
        buttonBounds.getCentreY() - buttonSize / 2,
        buttonSize, buttonSize);

    // Macro knobs - center
    auto macrosBounds = bottomSection;
    macrosBounds.reduce(20, 0);
    int macroWidth = macrosBounds.getWidth() / 4;
    boostKnob.setBounds(macrosBounds.removeFromLeft(macroWidth));
    airKnob.setBounds(macrosBounds.removeFromLeft(macroWidth));
    bodyKnob.setBounds(macrosBounds.removeFromLeft(macroWidth));
    warpKnob.setBounds(macrosBounds);

    // Main content area
    auto mainArea = bounds.reduced(15, 10);

    // Left panel - Oscillator (300px wide)
    oscillatorPanel.setBounds(mainArea.removeFromLeft(300));

    // Right panel - Filter (300px wide)
    filterPanel.setBounds(mainArea.removeFromRight(300));

    // Center tabbed panel
    tabbedPanel.setBounds(mainArea.reduced(15, 0));

    // Envelope and LFO panels now handle their own layout via their resized() methods
}

void PluginEditor::timerCallback()
{
    auto& apvts = processor.getAPVTS();

    // Update oscillator panel with unison voices
    float unisonVoices = apvts.getRawParameterValue("unison_voices")->load();
    oscillatorPanel.setValue(unisonVoices);

    // Update oscillator waveform display from parameter
    int oscWave = static_cast<int>(apvts.getRawParameterValue("osc1_wave")->load());
    // Map param value to UI: 0=Sine->3, 1=Saw->0, 2=Square->1, 3=Triangle->2
    int uiWave = 0;
    switch (oscWave) {
        case 0: uiWave = 3; break; // Sine -> SIN button
        case 1: uiWave = 0; break; // Saw -> SAW button
        case 2: uiWave = 1; break; // Square -> SQR button
        case 3: uiWave = 2; break; // Triangle -> TRI button
    }
    oscillatorPanel.setWaveform(uiWave);

    // Update filter panel with cutoff (convert Hz to kHz)
    float cutoffHz = apvts.getRawParameterValue("filter_cutoff")->load();
    filterPanel.setValue(cutoffHz / 1000.0f);

    // Update filter type display from parameter
    int filterType = static_cast<int>(apvts.getRawParameterValue("filter_type")->load());
    // Map param value to UI: 0=LP->0, 1=HP->3, 2=BP->2, 3=Notch->?
    int uiFilter = 0;
    switch (filterType) {
        case 0: uiFilter = 0; break; // LP -> LP12 button
        case 1: uiFilter = 3; break; // HP -> HP button
        case 2: uiFilter = 2; break; // BP -> BP button
        case 3: uiFilter = 0; break; // Notch -> LP12 (fallback)
    }
    filterPanel.setFilterType(uiFilter);

    // Update XY Pad from parameters
    float spread = apvts.getRawParameterValue("unison_spread")->load();
    float reverbMix = apvts.getRawParameterValue("reverb_mix")->load();
    xyPad.setValues(spread, reverbMix);

    // Update envelope display
    float ampA = apvts.getRawParameterValue("amp_attack")->load();
    float ampD = apvts.getRawParameterValue("amp_decay")->load();
    float ampS = apvts.getRawParameterValue("amp_sustain")->load();
    float ampR = apvts.getRawParameterValue("amp_release")->load();
    ampEnvelopeDisplay.setADSR(ampA, ampD, ampS, ampR);

    // Update envelope curves
    float atkCurve = apvts.getRawParameterValue("amp_attack_curve")->load();
    float decCurve = apvts.getRawParameterValue("amp_decay_curve")->load();
    float relCurve = apvts.getRawParameterValue("amp_release_curve")->load();
    ampEnvelopeDisplay.setCurves(atkCurve, decCurve, relCurve);

    // Update envelope enable button state
    bool envEnabled = apvts.getRawParameterValue("amp_env_enabled")->load() > 0.5f;
    envelopePanel->setEnvelopeEnabled(envEnabled);

    // Update engine start button state from flanger parameter
    bool flangerEnabled = apvts.getRawParameterValue("flanger_enabled")->load() > 0.5f;
    if (engineStartButton.getToggleState() != flangerEnabled)
        engineStartButton.setToggleState(flangerEnabled, juce::dontSendNotification);
}

// Computer keyboard -> MIDI note mapping
int PluginEditor::getNoteFroKeyCode(int keyCode) const
{
    int note = -1;
    int octaveOffset = baseOctave * 12;

    // Bottom row (Z-M) - Lower octave white keys
    switch (keyCode)
    {
        case 'Z': note = 0; break;  // C
        case 'X': note = 2; break;  // D
        case 'C': note = 4; break;  // E
        case 'V': note = 5; break;  // F
        case 'B': note = 7; break;  // G
        case 'N': note = 9; break;  // A
        case 'M': note = 11; break; // B
        case 'S': note = 1; break;  // C#
        case 'D': note = 3; break;  // D#
        case 'G': note = 6; break;  // F#
        case 'H': note = 8; break;  // G#
        case 'J': note = 10; break; // A#
    }

    if (note >= 0)
        return note + octaveOffset;

    // Top row (Q-P) - Upper octave
    octaveOffset = (baseOctave + 1) * 12;
    switch (keyCode)
    {
        case 'Q': note = 0; break;
        case 'W': note = 2; break;
        case 'E': note = 4; break;
        case 'R': note = 5; break;
        case 'T': note = 7; break;
        case 'Y': note = 9; break;
        case 'U': note = 11; break;
        case 'I': note = 12; break;
        case '2': note = 1; break;
        case '3': note = 3; break;
        case '5': note = 6; break;
        case '6': note = 8; break;
        case '7': note = 10; break;
    }

    if (note >= 0)
        return note + octaveOffset;

    return -1;
}

bool PluginEditor::keyPressed(const juce::KeyPress& key)
{
    int keyCode = key.getKeyCode();

    if (keyCode == juce::KeyPress::leftKey)
    {
        baseOctave = std::max(0, baseOctave - 1);
        return true;
    }
    if (keyCode == juce::KeyPress::rightKey)
    {
        baseOctave = std::min(8, baseOctave + 1);
        return true;
    }

    return false;
}

bool PluginEditor::keyStateChanged(bool isKeyDown)
{
    bool handled = false;

    const int keyCodes[] = {
        'Z', 'S', 'X', 'D', 'C', 'V', 'G', 'B', 'H', 'N', 'J', 'M',
        'Q', '2', 'W', '3', 'E', 'R', '5', 'T', '6', 'Y', '7', 'U', 'I'
    };

    for (int keyCode : keyCodes)
    {
        bool isPressed = juce::KeyPress::isKeyCurrentlyDown(keyCode);
        bool wasPressed = heldKeys.count(keyCode) > 0;

        if (isPressed && !wasPressed)
        {
            int note = getNoteFroKeyCode(keyCode);
            if (note >= 0 && note < 128)
            {
                heldKeys.insert(keyCode);
                keyboardState.noteOn(1, note, 0.8f);
                handled = true;
            }
        }
        else if (!isPressed && wasPressed)
        {
            int note = getNoteFroKeyCode(keyCode);
            if (note >= 0 && note < 128)
            {
                heldKeys.erase(keyCode);
                keyboardState.noteOff(1, note, 0.0f);
                handled = true;
            }
        }
    }

    return handled;
}

void PluginEditor::handleNoteOn(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    auto msg = juce::MidiMessage::noteOn(midiChannel, midiNoteNumber, velocity);
    processor.getVoiceManager().handleMidiMessage(msg);
    processor.getSampleSynth().noteOn(midiChannel, midiNoteNumber, velocity);
}

void PluginEditor::handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    auto msg = juce::MidiMessage::noteOff(midiChannel, midiNoteNumber, velocity);
    processor.getVoiceManager().handleMidiMessage(msg);
    processor.getSampleSynth().noteOff(midiChannel, midiNoteNumber, velocity);
}

void PluginEditor::loadPresetList()
{
    // Populate TopBar preset combo
    auto& topBarCombo = topBar.getPresetCombo();
    topBarCombo.clear();

    int id = 1;

    // Add sample presets by category
    auto& sampleManager = processor.getSamplePresetManager();
    const auto& categories = sampleManager.getCategories();

    for (const auto& category : categories)
    {
        topBarCombo.addSectionHeading(category);

        auto categoryPresets = sampleManager.getPresetsInCategory(category);
        for (const auto& preset : categoryPresets)
        {
            topBarCombo.addItem(preset.name, id++);
        }
    }

    // DON'T auto-select first preset here!
    // The preset restoration happens later in the constructor,
    // and we don't want to overwrite a restored preset.
}

void PluginEditor::loadPreset()
{
    int selected = presetBox.getSelectedId();

    if (selected >= 2000)
    {
        auto presetName = presetBox.getText();
        processor.loadSamplePreset(presetName);
    }
}

void PluginEditor::showPresetBrowser()
{
    presetBrowserVisible = true;
    updatePresetBrowserCategories();

    // Position the browser in center of plugin, overlaying the content
    auto browserBounds = getLocalBounds().reduced(100, 80);
    presetBrowser.setBounds(browserBounds);
    presetBrowser.setVisible(true);
    presetBrowser.toFront(true);
}

void PluginEditor::hidePresetBrowser()
{
    presetBrowserVisible = false;
    presetBrowser.setVisible(false);
}

void PluginEditor::updatePresetBrowserCategories()
{
    auto& sampleManager = processor.getSamplePresetManager();
    const auto& categories = sampleManager.getCategories();

    std::vector<juce::String> categoryVec;
    for (const auto& cat : categories)
        categoryVec.push_back(cat);

    presetBrowser.setCategories(categoryVec);

    // Rebuild flat list if it's empty (shouldn't normally happen as it's built in constructor)
    if (allPresetsFlat.empty())
    {
        for (const auto& cat : categories)
        {
            auto presets = sampleManager.getPresetsInCategory(cat);
            for (const auto& preset : presets)
                allPresetsFlat.push_back({cat, preset.name});
        }
    }

    // If categories exist, update presets for first category
    if (!categories.empty())
        updatePresetBrowserPresets(categories[0]);
}

void PluginEditor::updatePresetBrowserPresets(const juce::String& category)
{
    auto& sampleManager = processor.getSamplePresetManager();
    auto presets = sampleManager.getPresetsInCategory(category);

    std::vector<HellcatPresetList::PresetInfo> presetInfos;
    int id = 1;
    for (const auto& preset : presets)
    {
        presetInfos.push_back({preset.name, id++});
    }

    presetBrowser.setPresetsForCategory(presetInfos);
}

void PluginEditor::selectNextPreset()
{
    if (allPresetsFlat.empty())
    {
        // Initialize the flat list if needed
        auto& sampleManager = processor.getSamplePresetManager();
        const auto& categories = sampleManager.getCategories();
        for (const auto& cat : categories)
        {
            auto presets = sampleManager.getPresetsInCategory(cat);
            for (const auto& preset : presets)
                allPresetsFlat.push_back({cat, preset.name});
        }
    }

    if (allPresetsFlat.empty())
        return;

    currentPresetIndex = (currentPresetIndex + 1) % static_cast<int>(allPresetsFlat.size());
    auto& [category, name] = allPresetsFlat[static_cast<size_t>(currentPresetIndex)];
    processor.loadSamplePreset(name);
    topBar.setCurrentPresetName(name);
}

void PluginEditor::selectPrevPreset()
{
    if (allPresetsFlat.empty())
    {
        // Initialize the flat list if needed
        auto& sampleManager = processor.getSamplePresetManager();
        const auto& categories = sampleManager.getCategories();
        for (const auto& cat : categories)
        {
            auto presets = sampleManager.getPresetsInCategory(cat);
            for (const auto& preset : presets)
                allPresetsFlat.push_back({cat, preset.name});
        }
    }

    if (allPresetsFlat.empty())
        return;

    currentPresetIndex = currentPresetIndex - 1;
    if (currentPresetIndex < 0)
        currentPresetIndex = static_cast<int>(allPresetsFlat.size()) - 1;

    auto& [category, name] = allPresetsFlat[static_cast<size_t>(currentPresetIndex)];
    processor.loadSamplePreset(name);
    topBar.setCurrentPresetName(name);
}

} // namespace NulyBeats
