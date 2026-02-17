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
 * Preset list component with favorite star and sorting support
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

    void setFavorites(std::set<juce::String>* favs) { favorites = favs; }

    bool isFavorite(const juce::String& name) const
    {
        return favorites != nullptr && favorites->count(name) > 0;
    }

    int getNumRows() override { return static_cast<int>(presets.size()); }

    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override
    {
        if (rowNumber < 0 || rowNumber >= static_cast<int>(presets.size()))
            return;

        auto area = juce::Rectangle<int>(0, 0, width, height);
        const auto& preset = presets[static_cast<size_t>(rowNumber)];
        bool faved = isFavorite(preset.name);

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

        // Star icon (left side, 24px wide)
        auto starArea = area.removeFromLeft(24);
        drawStar(g, starArea.toFloat().reduced(4, 5), faved);

        // Text
        g.setColour(rowIsSelected ? juce::Colours::white : juce::Colour(0xffcccccc));
        g.setFont(juce::Font(12.0f));
        g.drawText(preset.name, area.reduced(4, 0), juce::Justification::centredLeft);
    }

    void listBoxItemClicked(int row, const juce::MouseEvent& e) override
    {
        // Check if click was in the star area (first 24px)
        if (e.x < 24 && row >= 0 && row < static_cast<int>(presets.size()))
        {
            if (onFavoriteToggled)
                onFavoriteToggled(presets[static_cast<size_t>(row)].name);
            listBox.repaintRow(row);
        }
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
    std::function<void(const juce::String&)> onFavoriteToggled;

private:
    void drawStar(juce::Graphics& g, juce::Rectangle<float> area, bool filled)
    {
        float cx = area.getCentreX();
        float cy = area.getCentreY();
        float outerR = juce::jmin(area.getWidth(), area.getHeight()) * 0.5f;
        float innerR = outerR * 0.4f;

        juce::Path star;
        for (int i = 0; i < 5; ++i)
        {
            float outerAngle = static_cast<float>(i) * juce::MathConstants<float>::twoPi / 5.0f - juce::MathConstants<float>::halfPi;
            float innerAngle = outerAngle + juce::MathConstants<float>::twoPi / 10.0f;

            float ox = cx + std::cos(outerAngle) * outerR;
            float oy = cy + std::sin(outerAngle) * outerR;
            float ix = cx + std::cos(innerAngle) * innerR;
            float iy = cy + std::sin(innerAngle) * innerR;

            if (i == 0)
                star.startNewSubPath(ox, oy);
            else
                star.lineTo(ox, oy);
            star.lineTo(ix, iy);
        }
        star.closeSubPath();

        if (filled)
        {
            g.setColour(HellcatColors::hellcatRed);
            g.fillPath(star);
            g.setColour(HellcatColors::hellcatRed.brighter(0.3f));
            g.strokePath(star, juce::PathStrokeType(1.0f));
        }
        else
        {
            g.setColour(HellcatColors::panelLight.brighter(0.2f));
            g.strokePath(star, juce::PathStrokeType(1.0f));
        }
    }

    juce::ListBox listBox;
    std::vector<PresetInfo> presets;
    std::set<juce::String>* favorites = nullptr;
};

/**
 * Two-panel preset browser with category list, sort buttons, and favorite stars
 */
class HellcatPresetBrowser : public juce::Component
{
public:
    enum class SortMode { AtoZ, ZtoA, FavoritesFirst };

    HellcatPresetBrowser()
    {
        // Load favorites from disk
        loadFavorites();

        // Header label
        headerLabel.setText("PRESET BROWSER", juce::dontSendNotification);
        headerLabel.setFont(juce::Font(14.0f, juce::Font::bold));
        headerLabel.setColour(juce::Label::textColourId, HellcatColors::hellcatRed);
        headerLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(headerLabel);

        // Search box
        searchBox.setTextToShowWhenEmpty("Search...", juce::Colour(0xff666666));
        searchBox.setColour(juce::TextEditor::backgroundColourId, HellcatColors::panelDark);
        searchBox.setColour(juce::TextEditor::textColourId, juce::Colours::white);
        searchBox.setColour(juce::TextEditor::outlineColourId, HellcatColors::panelLight);
        searchBox.setColour(juce::CaretComponent::caretColourId, HellcatColors::hellcatRed);
        searchBox.setFont(juce::Font(12.0f));
        searchBox.onTextChange = [this]() { filterAndSortPresets(); };
        addAndMakeVisible(searchBox);

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

        // Sort buttons — manual toggle state (no radio group, avoids JUCE toggle quirks)
        auto setupSortBtn = [this](juce::TextButton& btn, const juce::String& text) {
            btn.setButtonText(text);
            btn.setColour(juce::TextButton::buttonColourId, HellcatColors::panelDark);
            btn.setColour(juce::TextButton::buttonOnColourId, HellcatColors::hellcatRed.withAlpha(0.6f));
            btn.setColour(juce::TextButton::textColourOffId, HellcatColors::textTertiary);
            btn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
            btn.setClickingTogglesState(false);
            addAndMakeVisible(btn);
        };

        setupSortBtn(sortAZButton, "A-Z");
        setupSortBtn(sortZAButton, "Z-A");
        setupSortBtn(sortFavButton, "FAV");

        // Default: A-Z active
        sortAZButton.setToggleState(true, juce::dontSendNotification);

        sortAZButton.onClick = [this]() { setSortMode(SortMode::AtoZ); };
        sortZAButton.onClick = [this]() { setSortMode(SortMode::ZtoA); };
        sortFavButton.onClick = [this]() { setSortMode(SortMode::FavoritesFirst); };

        // Category list
        addAndMakeVisible(categoryList);
        categoryList.onCategorySelected = [this](const juce::String& category) {
            currentCategory = category;
            if (onCategoryChanged)
                onCategoryChanged(category);
        };

        // Preset list
        addAndMakeVisible(presetList);
        presetList.setFavorites(&favorites);

        presetList.onPresetSelected = [this](int id, const juce::String& name) {
            if (onPresetSelected)
                onPresetSelected(id, name);
        };
        presetList.onPresetDoubleClicked = [this](int id, const juce::String& name) {
            if (onPresetLoaded)
                onPresetLoaded(id, name);
        };
        presetList.onFavoriteToggled = [this](const juce::String& name) {
            toggleFavorite(name);
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

    ~HellcatPresetBrowser() override
    {
        saveFavorites();
    }

    void setCategories(const std::vector<juce::String>& categories)
    {
        categoryList.setCategories(categories);
    }

    void setPresetsForCategory(const std::vector<HellcatPresetList::PresetInfo>& presets)
    {
        allPresets = presets;
        filterAndSortPresets();
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
        int dividerX = 5 + static_cast<int>(getWidth() * 0.28f);
        g.setColour(HellcatColors::panelLight);
        g.drawLine(static_cast<float>(dividerX), 55, static_cast<float>(dividerX), static_cast<float>(getHeight() - 5), 1.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // Header
        auto headerArea = bounds.removeFromTop(35);
        closeButton.setBounds(headerArea.removeFromRight(35).reduced(8));
        headerLabel.setBounds(headerArea.removeFromLeft(static_cast<int>(headerArea.getWidth() * 0.35f)));
        searchBox.setBounds(headerArea.reduced(4, 6));

        bounds.reduce(5, 5);

        // Column headers — proportional widths
        int catWidth = static_cast<int>(bounds.getWidth() * 0.28f);
        auto columnHeaders = bounds.removeFromTop(20);
        categoryHeader.setBounds(columnHeaders.removeFromLeft(catWidth));

        // Sort buttons in the preset header row (right side)
        auto sortArea = columnHeaders.removeFromRight(150);
        int sortBtnWidth = sortArea.getWidth() / 3;
        sortAZButton.setBounds(sortArea.removeFromLeft(sortBtnWidth).reduced(2, 1));
        sortZAButton.setBounds(sortArea.removeFromLeft(sortBtnWidth).reduced(2, 1));
        sortFavButton.setBounds(sortArea.reduced(2, 1));

        presetHeader.setBounds(columnHeaders);

        // Category list (left) — proportional
        categoryList.setBounds(bounds.removeFromLeft(catWidth));

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
    void setSortMode(SortMode newMode)
    {
        sortMode = newMode;

        // Update button toggle states manually
        sortAZButton.setToggleState(newMode == SortMode::AtoZ, juce::dontSendNotification);
        sortZAButton.setToggleState(newMode == SortMode::ZtoA, juce::dontSendNotification);
        sortFavButton.setToggleState(newMode == SortMode::FavoritesFirst, juce::dontSendNotification);

        filterAndSortPresets();
    }

    void toggleFavorite(const juce::String& name)
    {
        if (favorites.count(name) > 0)
            favorites.erase(name);
        else
            favorites.insert(name);

        saveFavorites();
        filterAndSortPresets();
    }

    void filterAndSortPresets()
    {
        juce::String query = searchBox.getText().trim().toLowerCase();

        // Start with all presets
        std::vector<HellcatPresetList::PresetInfo> result;
        for (const auto& preset : allPresets)
        {
            if (query.isEmpty() || preset.name.toLowerCase().contains(query))
                result.push_back(preset);
        }

        // Sort
        switch (sortMode)
        {
            case SortMode::AtoZ:
                std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
                    return a.name.toLowerCase() < b.name.toLowerCase();
                });
                break;

            case SortMode::ZtoA:
                std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
                    return a.name.toLowerCase() > b.name.toLowerCase();
                });
                break;

            case SortMode::FavoritesFirst:
                std::sort(result.begin(), result.end(), [this](const auto& a, const auto& b) {
                    bool aFav = favorites.count(a.name) > 0;
                    bool bFav = favorites.count(b.name) > 0;
                    if (aFav != bFav) return aFav > bFav; // Favorites first
                    return a.name.toLowerCase() < b.name.toLowerCase(); // Then A-Z
                });
                break;
        }

        presetList.setPresets(result);
    }

    juce::File getFavoritesFile() const
    {
        auto appDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getChildFile("NullyBeats")
                          .getChildFile("DemonSynth");
        appDir.createDirectory();
        return appDir.getChildFile("favorites.txt");
    }

    void loadFavorites()
    {
        auto file = getFavoritesFile();
        if (!file.existsAsFile())
            return;

        juce::StringArray lines;
        file.readLines(lines);
        for (const auto& line : lines)
        {
            auto trimmed = line.trim();
            if (trimmed.isNotEmpty())
                favorites.insert(trimmed);
        }
    }

    void saveFavorites()
    {
        auto file = getFavoritesFile();
        juce::String content;
        for (const auto& name : favorites)
            content += name + "\n";
        file.replaceWithText(content);
    }

    SortMode sortMode = SortMode::AtoZ;
    std::set<juce::String> favorites;

    juce::Label headerLabel;
    juce::Label categoryHeader;
    juce::Label presetHeader;
    juce::TextEditor searchBox;
    juce::TextButton sortAZButton;
    juce::TextButton sortZAButton;
    juce::TextButton sortFavButton;
    HellcatCategoryList categoryList;
    HellcatPresetList presetList;
    juce::TextButton closeButton;
    juce::String currentCategory;
    std::vector<HellcatPresetList::PresetInfo> allPresets;
};
