#pragma once
#include <JuceHeader.h>
#include "../HellcatLookAndFeel.h"

/**
 * Custom tab button that draws like the original HellcatLookAndFeel tab styling
 */
class HellcatTabButton : public juce::Component
{
public:
    HellcatTabButton(const juce::String& text) : buttonText(text) {}

    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds();

        // Background - ALL tabs get a visible background
        if (isActive)
        {
            // Active tab - red tinted background
            g.setColour(HellcatColors::hellcatRed.withAlpha(0.15f));
            g.fillRect(area);
        }
        else
        {
            // Inactive tabs - visible grey background
            g.setColour(juce::Colour(0xff252830));
            g.fillRect(area);

            if (isMouseOver())
            {
                g.setColour(HellcatColors::hellcatRed.withAlpha(0.1f));
                g.fillRect(area);
            }
        }

        // Border around each tab for definition
        g.setColour(HellcatColors::panelLight);
        g.drawRect(area, 1);

        // Text - Orbitron font, high contrast colors
        g.setColour(isActive ? juce::Colours::white : juce::Colour(0xffcccccc));
        if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
            g.setFont(lf->getOrbitronFont(11.0f));
        else
            g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(buttonText, area, juce::Justification::centred);

        // Active indicator line - glowing red at bottom
        if (isActive)
        {
            g.setColour(HellcatColors::hellcatRed);
            g.fillRect(area.getX(), area.getBottom() - 3, area.getWidth(), 3);
        }
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        if (onClick)
            onClick();
    }

    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&) override { repaint(); }

    void setActive(bool active)
    {
        isActive = active;
        repaint();
    }

    std::function<void()> onClick;

private:
    juce::String buttonText;
    bool isActive = false;
};

/**
 * Custom tabbed panel that doesn't hide tabs - all tabs are always visible.
 * This avoids JUCE's TabbedButtonBar overflow behavior that hides tabs.
 */
class HellcatTabbedPanel : public juce::Component
{
public:
    HellcatTabbedPanel()
    {
        // Create default tabs
        addTab("MOD MATRIX");
        addTab("ENVELOPES");
        addTab("LFOs");
        addTab("FX");

        // Set ENVELOPES as default
        tabButtons[1]->setActive(true);
    }

    void addTab(const juce::String& name, juce::Component* content = nullptr)
    {
        auto* btn = new HellcatTabButton(name);
        int idx = tabButtons.size();
        btn->onClick = [this, idx]() { setCurrentTab(idx); };
        addAndMakeVisible(btn);
        tabButtons.add(btn);
        tabContents.push_back(content);

        if (content != nullptr)
        {
            addAndMakeVisible(content);
            content->setVisible(idx == currentTabIndex);
        }
    }

    void setTabContent(int index, juce::Component* content)
    {
        if (index >= 0 && index < static_cast<int>(tabContents.size()) && content != nullptr)
        {
            tabContents[static_cast<size_t>(index)] = content;
            addAndMakeVisible(content);
            content->setVisible(index == currentTabIndex);
        }
    }

    void setCurrentTab(int index)
    {
        if (index < 0 || index >= static_cast<int>(tabContents.size())) return;

        currentTabIndex = index;

        // Update button states
        for (int i = 0; i < tabButtons.size(); ++i)
            tabButtons[i]->setActive(i == index);

        // Show only current tab content
        for (int i = 0; i < static_cast<int>(tabContents.size()); ++i)
        {
            if (tabContents[static_cast<size_t>(i)] != nullptr)
                tabContents[static_cast<size_t>(i)]->setVisible(i == index);
        }

        repaint();
    }

    int getCurrentTabIndex() const { return currentTabIndex; }

    void paint(juce::Graphics& g) override
    {
        // Background
        g.fillAll(HellcatColors::background);

        // Tab bar background
        g.setColour(HellcatColors::panelDark);
        g.fillRect(0, 0, getWidth(), tabBarHeight);

        // Bottom border of tab bar
        g.setColour(HellcatColors::panelLight);
        g.drawLine(0, static_cast<float>(tabBarHeight - 1),
                   static_cast<float>(getWidth()), static_cast<float>(tabBarHeight - 1), 1.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // Tab bar at top
        auto tabBar = bounds.removeFromTop(tabBarHeight);

        // Distribute tabs evenly
        int numTabs = tabButtons.size();
        int tabWidth = numTabs > 0 ? tabBar.getWidth() / numTabs : tabBar.getWidth();
        for (int i = 0; i < numTabs; ++i)
        {
            tabButtons[i]->setBounds(tabBar.removeFromLeft(tabWidth));
        }

        // Content area - set bounds for ALL content panels
        for (auto* content : tabContents)
        {
            if (content != nullptr)
                content->setBounds(bounds);
        }
    }

    // Force initial layout when component becomes visible
    void parentHierarchyChanged() override
    {
        resized();
    }

private:
    static constexpr int tabBarHeight = 45;
    int currentTabIndex = 1; // ENVELOPES by default

    juce::OwnedArray<HellcatTabButton> tabButtons;
    std::vector<juce::Component*> tabContents;
};
