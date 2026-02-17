#pragma once

#include <JuceHeader.h>
#include <map>

namespace NulyBeats {
namespace Modulation {

/**
 * Lightweight MIDI CC learn system.
 * Maps incoming CC messages to plugin parameters.
 */
class MidiLearn
{
public:
    void startLearning(const juce::String& paramId)
    {
        learning = true;
        learningParamId = paramId;
    }

    void stopLearning()
    {
        learning = false;
        learningParamId.clear();
    }

    bool isLearning() const { return learning; }
    const juce::String& getLearningParamId() const { return learningParamId; }

    /**
     * Process an incoming CC message.
     * If in learn mode, maps the CC to the learning parameter.
     * If mapped, updates the parameter value.
     */
    void processMidiCC(int cc, float value, juce::AudioProcessorValueTreeState& apvts)
    {
        // Skip sustain pedal and mod wheel — handled by the engine directly
        if (cc == 1 || cc == 64)
            return;

        if (learning)
        {
            ccToParamMap[cc] = learningParamId;
            learning = false;
            learningParamId.clear();
            // Also apply the value immediately
        }

        auto it = ccToParamMap.find(cc);
        if (it != ccToParamMap.end())
        {
            if (auto* param = apvts.getParameter(it->second))
            {
                param->setValueNotifyingHost(value);
            }
        }
    }

    void clearMapping(const juce::String& paramId)
    {
        for (auto it = ccToParamMap.begin(); it != ccToParamMap.end(); )
        {
            if (it->second == paramId)
                it = ccToParamMap.erase(it);
            else
                ++it;
        }
    }

    void clearAll()
    {
        ccToParamMap.clear();
        stopLearning();
    }

    int getCCForParam(const juce::String& paramId) const
    {
        for (const auto& [cc, param] : ccToParamMap)
        {
            if (param == paramId)
                return cc;
        }
        return -1;
    }

    // Serialization
    void saveToXml(juce::XmlElement& parent) const
    {
        auto* midiLearnXml = parent.createNewChildElement("MidiLearnMappings");
        for (const auto& [cc, paramId] : ccToParamMap)
        {
            auto* mapping = midiLearnXml->createNewChildElement("Mapping");
            mapping->setAttribute("cc", cc);
            mapping->setAttribute("param", paramId);
        }
    }

    void loadFromXml(const juce::XmlElement& parent)
    {
        ccToParamMap.clear();
        if (auto* midiLearnXml = parent.getChildByName("MidiLearnMappings"))
        {
            for (auto* mapping : midiLearnXml->getChildIterator())
            {
                if (mapping->hasTagName("Mapping"))
                {
                    int cc = mapping->getIntAttribute("cc", -1);
                    juce::String paramId = mapping->getStringAttribute("param");
                    if (cc >= 0 && cc < 128 && paramId.isNotEmpty())
                        ccToParamMap[cc] = paramId;
                }
            }
        }
    }

private:
    std::map<int, juce::String> ccToParamMap;
    bool learning = false;
    juce::String learningParamId;
};

} // namespace Modulation
} // namespace NulyBeats
