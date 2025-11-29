#pragma once
#include <JuceHeader.h>
#include "../HellcatLookAndFeel.h"

/**
 * Category list component - displays all available sample categories
 */
class HellcatCategoryList : public juce::Component,
                            public juce::ListBoxModel
{
public:
    HellcatCategoryList()
    {
        listBox.setModel(this);
        listBox.setRowHeight(32);
        listBox.setColour(juce::ListBox::backgroundColourId, HellcatColors::panelDark);
        listBox.setColour(juce::ListBox::outlineColourId, HellcatColors::panelLight);
        addAndMakeVisible(listBox);
    }

    void setCategories(const std::vector<juce::String>& cats)
    {
        categories = cats;
        listBox.updateContent();
        if (!categories.empty())
            listBox.selectRow(0);
    }

    int getNumRows() override { return static_cast<int>(categories.size()); }

    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override
    {
        if (rowNumber < 0 || rowNumber >= static_cast<int>(categories.size()))
            return;

        auto area = juce::Rectangle<int>(0, 0, width, height);

        // Background
        if (rowIsSelected)
        {
            g.setColour(HellcatColors::hellcatRed.withAlpha(0.3f));
            g.fillRect(area);
            g.setColour(HellcatColors::hellcatRed);
            g.fillRect(0, 0, 3, height); // Left accent bar
        }
        else
        {
            g.setColour(HellcatColors::panelDark);
            g.fillRect(area);
        }

        // Text
        g.setColour(rowIsSelected ? juce::Colours::white : juce::Colour(0xffaaaaaa));
        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.drawText(categories[static_cast<size_t>(rowNumber)], area.reduced(12, 0), juce::Justification::centredLeft);

        // Bottom border
        g.setColour(HellcatColors::panelLight.withAlpha(0.3f));
        g.drawLine(0, static_cast<float>(height - 1), static_cast<float>(width), static_cast<float>(height - 1));
    }

    void selectedRowsChanged(int lastRowSelected) override
    {
        if (onCategorySelected && lastRowSelected >= 0 && lastRowSelected < static_cast<int>(categories.size()))
            onCategorySelected(categories[static_cast<size_t>(lastRowSelected)]);
    }

    void resized() override
    {
        listBox.setBounds(getLocalBounds());
    }

    std::function<void(const juce::String&)> onCategorySelected;

private:
    juce::ListBox listBox;
    std::vector<juce::String> categories;
};

/**
 * Preset list component - displays presets in selected category
 */
class HellcatPresetList : public juce::Component,
                          public juce::ListBoxModel
{
public:
    struct PresetInfo
    {
        juce::String name;
        int id;
    };

    HellcatPresetList()
    {
        listBox.setModel(this);
        listBox.setRowHeight(28);
        listBox.setColour(juce::ListBox::backgroundColourId, HellcatColors::background);
        listBox.setColour(juce::ListBox::outlineColourId, HellcatColors::panelLight);
        addAndMakeVisible(listBox);
    }

    void setPresets(const std::vector<PresetInfo>& p)
    {
        presets = p;
        listBox.updateContent();
        if (!presets.empty())
            listBox.selectRow(0);
    }

    int getNumRows() override { return static_cast<int>(presets.size()); }

    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override
    {
        if (rowNumber < 0 || rowNumber >= static_cast<int>(presets.size()))
            return;

        auto area = juce::Rectangle<int>(0, 0, width, height);

        // Background
        if (rowIsSelected)
        {
            g.setColour(HellcatColors::hellcatRed.withAlpha(0.2f));
            g.fillRect(area);
        }
        else if (rowNumber % 2 == 0)
        {
            g.setColour(HellcatColors::background.brighter(0.05f));
            g.fillRect(area);
        }

        // Text
        g.setColour(rowIsSelected ? juce::Colours::white : juce::Colour(0xffcccccc));
        g.setFont(juce::Font(12.0f));
        g.drawText(presets[static_cast<size_t>(rowNumber)].name, area.reduced(10, 0), juce::Justification::centredLeft);
    }

    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override
    {
        if (onPresetDoubleClicked && row >= 0 && row < static_cast<int>(presets.size()))
            onPresetDoubleClicked(presets[static_cast<size_t>(row)].id, presets[static_cast<size_t>(row)].name);
    }

    void selectedRowsChanged(int lastRowSelected) override
    {
        if (onPresetSelected && lastRowSelected >= 0 && lastRowSelected < static_cast<int>(presets.size()))
            onPresetSelected(presets[static_cast<size_t>(lastRowSelected)].id, presets[static_cast<size_t>(lastRowSelected)].name);
    }

    void resized() override
    {
        listBox.setBounds(getLocalBounds());
    }

    std::function<void(int, const juce::String&)> onPresetSelected;
    std::function<void(int, const juce::String&)> onPresetDoubleClicked;

private:
    juce::ListBox listBox;
    std::vector<PresetInfo> presets;
};

/**
 * Two-panel preset browser with category list on left and presets on right
 * Similar to Zenology's category-based browsing
 */
class HellcatPresetBrowser : public juce::Component
{
public:
    HellcatPresetBrowser()
    {
        // Header label
        headerLabel.setText("PRESET BROWSER", juce::dontSendNotification);
        headerLabel.setFont(juce::Font(14.0f, juce::Font::bold));
        headerLabel.setColour(juce::Label::textColourId, HellcatColors::hellcatRed);
        headerLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(headerLabel);

        // Category header
        categoryHeader.setText("CATEGORY", juce::dontSendNotification);
        categoryHeader.setFont(juce::Font(11.0f, juce::Font::bold));
        categoryHeader.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
        categoryHeader.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(categoryHeader);

        // Preset header
        presetHeader.setText("PRESETS", juce::dontSendNotification);
        presetHeader.setFont(juce::Font(11.0f, juce::Font::bold));
        presetHeader.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
        presetHeader.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(presetHeader);

        // Category list
        addAndMakeVisible(categoryList);
        categoryList.onCategorySelected = [this](const juce::String& category) {
            currentCategory = category;
            if (onCategoryChanged)
                onCategoryChanged(category);
        };

        // Preset list
        addAndMakeVisible(presetList);
        presetList.onPresetSelected = [this](int id, const juce::String& name) {
            if (onPresetSelected)
                onPresetSelected(id, name);
        };
        presetList.onPresetDoubleClicked = [this](int id, const juce::String& name) {
            if (onPresetLoaded)
                onPresetLoaded(id, name);
        };

        // Close button
        closeButton.setButtonText("X");
        closeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        closeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff888888));
        closeButton.onClick = [this]() {
            if (onClose)
                onClose();
        };
        addAndMakeVisible(closeButton);
    }

    void setCategories(const std::vector<juce::String>& categories)
    {
        categoryList.setCategories(categories);
    }

    void setPresetsForCategory(const std::vector<HellcatPresetList::PresetInfo>& presets)
    {
        presetList.setPresets(presets);
    }

    void paint(juce::Graphics& g) override
    {
        // Background
        g.fillAll(HellcatColors::background);

        // Border
        g.setColour(HellcatColors::panelLight);
        g.drawRect(getLocalBounds(), 2);

        // Header background
        g.setColour(HellcatColors::panelDark);
        g.fillRect(0, 0, getWidth(), 35);

        // Divider between header and content
        g.setColour(HellcatColors::hellcatRed.withAlpha(0.5f));
        g.drawLine(0, 35, static_cast<float>(getWidth()), 35, 2.0f);

        // Vertical divider between categories and presets
        int dividerX = 150;
        g.setColour(HellcatColors::panelLight);
        g.drawLine(static_cast<float>(dividerX), 55, static_cast<float>(dividerX), static_cast<float>(getHeight() - 5), 1.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // Header
        auto headerArea = bounds.removeFromTop(35);
        closeButton.setBounds(headerArea.removeFromRight(35).reduced(8));
        headerLabel.setBounds(headerArea);

        bounds.reduce(5, 5);

        // Column headers
        auto columnHeaders = bounds.removeFromTop(20);
        categoryHeader.setBounds(columnHeaders.removeFromLeft(145));
        presetHeader.setBounds(columnHeaders);

        // Category list (left) - fixed width
        categoryList.setBounds(bounds.removeFromLeft(145));

        // Preset list (right) - remaining space
        bounds.removeFromLeft(5); // gap
        presetList.setBounds(bounds);
    }

    // Callbacks
    std::function<void(const juce::String&)> onCategoryChanged;
    std::function<void(int, const juce::String&)> onPresetSelected;
    std::function<void(int, const juce::String&)> onPresetLoaded;
    std::function<void()> onClose;

private:
    juce::Label headerLabel;
    juce::Label categoryHeader;
    juce::Label presetHeader;
    HellcatCategoryList categoryList;
    HellcatPresetList presetList;
    juce::TextButton closeButton;
    juce::String currentCategory;
};
