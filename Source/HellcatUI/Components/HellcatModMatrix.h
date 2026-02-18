#pragma once
#include <JuceHeader.h>
#include "../HellcatLookAndFeel.h"

class HellcatModMatrixRow : public juce::Component
{
public:
    HellcatModMatrixRow(int rowIndex, std::function<void(int, int, int, float)>& parentCallback)
        : index(rowIndex), onRoutingChanged(parentCallback)
    {
        sourceCombo.addItem("None",      1);
        sourceCombo.addItem("LFO 1",     2);
        sourceCombo.addItem("LFO 2",     3);
        sourceCombo.addItem("Env 1",     4);
        sourceCombo.addItem("Env 2",     5);
        sourceCombo.addItem("Velocity",  6);
        sourceCombo.addItem("Mod Wheel", 7);
        sourceCombo.setSelectedId(1, juce::dontSendNotification);
        sourceCombo.onChange = [this]() { notifyParent(); };
        addAndMakeVisible(sourceCombo);

        destCombo.addItem("None",             1);
        destCombo.addItem("Filter Cutoff",    2);
        destCombo.addItem("Filter Resonance", 3);
        destCombo.addItem("Osc Pitch",        4);
        destCombo.addItem("Osc Level",        5);
        destCombo.addItem("Amp Pan",          6);
        destCombo.addItem("Amp Level",        7);
        destCombo.setSelectedId(1, juce::dontSendNotification);
        destCombo.onChange = [this]() { notifyParent(); };
        addAndMakeVisible(destCombo);

        amountSlider.setRange(-1.0, 1.0);
        amountSlider.setValue(0.0, juce::dontSendNotification);
        amountSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        amountSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 45, 16);
        amountSlider.onValueChange = [this]() { notifyParent(); };
        addAndMakeVisible(amountSlider);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        g.setColour(HellcatColors::background);
        g.drawLine(bounds.getX(), bounds.getBottom() - 1,
                   bounds.getRight(), bounds.getBottom() - 1, 1.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10, 4);
        int w = bounds.getWidth();
        sourceCombo.setBounds(bounds.removeFromLeft(w / 3 - 4));
        bounds.removeFromLeft(4);
        destCombo.setBounds(bounds.removeFromLeft(w / 3 - 4));
        bounds.removeFromLeft(4);
        amountSlider.setBounds(bounds);
    }

    void setRouting(int srcId, int dstId, float amount)
    {
        sourceCombo.setSelectedId(srcId, juce::dontSendNotification);
        destCombo.setSelectedId(dstId, juce::dontSendNotification);
        amountSlider.setValue(amount, juce::dontSendNotification);
    }

    int getSourceId()  const { return sourceCombo.getSelectedId(); }
    int getDestId()    const { return destCombo.getSelectedId(); }
    float getAmount()  const { return static_cast<float>(amountSlider.getValue()); }

private:
    void notifyParent()
    {
        if (onRoutingChanged)
            onRoutingChanged(index, getSourceId(), getDestId(), getAmount());
    }

    int index;
    std::function<void(int, int, int, float)>& onRoutingChanged;

    juce::ComboBox sourceCombo;
    juce::ComboBox destCombo;
    juce::Slider   amountSlider;
};

class HellcatModMatrix : public juce::Component
{
public:
    static constexpr int NUM_ROWS = 5;

    HellcatModMatrix()
    {
        for (int i = 0; i < NUM_ROWS; ++i)
        {
            auto* row = new HellcatModMatrixRow(i, onRoutingChanged);
            rows.add(row);
            addAndMakeVisible(row);
        }
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        auto headerBounds = bounds.removeFromTop(34);

        g.setColour(HellcatColors::background.brighter(0.05f));
        g.fillRect(headerBounds);
        g.setColour(HellcatColors::panelLight);
        g.drawLine(headerBounds.getX(), headerBounds.getBottom(),
                   headerBounds.getRight(), headerBounds.getBottom(), 1.0f);

        g.setColour(HellcatColors::textSecondary);
        g.setFont(juce::Font(10.0f, juce::Font::bold));

        int w = headerBounds.getWidth();
        auto col1 = headerBounds.removeFromLeft(w / 3).reduced(10, 0);
        auto col2 = headerBounds.removeFromLeft(w / 3).reduced(10, 0);
        auto col3 = headerBounds.reduced(10, 0);

        g.drawText("SOURCE",      col1, juce::Justification::centredLeft);
        g.drawText("DESTINATION", col2, juce::Justification::centredLeft);
        g.drawText("AMOUNT",      col3, juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(34); // header
        int rowH = bounds.getHeight() / NUM_ROWS;
        for (auto* row : rows)
            row->setBounds(bounds.removeFromTop(rowH));
    }

    // Restore saved routing into row controls (call on setStateInformation)
    void setRowRouting(int row, int srcId, int dstId, float amount)
    {
        if (row >= 0 && row < NUM_ROWS)
            rows[row]->setRouting(srcId, dstId, amount);
    }

    // Called whenever any row changes — wire this in PluginEditor
    std::function<void(int row, int srcId, int dstId, float amount)> onRoutingChanged;

private:
    juce::OwnedArray<HellcatModMatrixRow> rows;
};
