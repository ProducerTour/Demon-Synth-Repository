#pragma once

#include <JuceHeader.h>
#include "SynthVoice.h"
#include <vector>
#include <array>
#include <algorithm>

namespace NulyBeats {
namespace Engine {

/**
 * Voice manager handling polyphony, voice stealing, unison, etc.
 */
class VoiceManager
{
public:
    static constexpr int MAX_VOICES = 64;
    static constexpr int MAX_UNISON = 8;

    enum class VoiceStealingMode
    {
        Oldest,
        Quietest,
        HighestNote,
        LowestNote
    };

    enum class VoiceMode
    {
        Poly,       // Full polyphony
        Mono,       // Monophonic - retrigger envelopes on each note
        Legato      // Monophonic - only retrigger on non-overlapping notes
    };

    VoiceManager()
    {
        voices.resize(MAX_VOICES);
    }

    void prepare(double sampleRate, int samplesPerBlock)
    {
        this->sampleRate = sampleRate;
        this->samplesPerBlock = samplesPerBlock;

        // Pre-allocate voice buffers to avoid real-time allocation
        voiceBufferLeft.resize(static_cast<size_t>(samplesPerBlock));
        voiceBufferRight.resize(static_cast<size_t>(samplesPerBlock));

        for (auto& voice : voices)
            voice.prepare(sampleRate, samplesPerBlock);
    }

    void setPolyphony(int numVoices)
    {
        maxPolyphony = juce::jlimit(1, MAX_VOICES, numVoices);
    }

    void setVoiceParameters(const SynthVoice::Parameters& params)
    {
        voiceParams = params;
        for (auto& voice : voices)
            voice.setParameters(params);
    }

    void setUnison(int numVoices, float detune, float spread)
    {
        unisonVoices = juce::jlimit(1, MAX_UNISON, numVoices);
        unisonDetune = detune;
        unisonSpread = spread;
    }

    void setVoiceMode(VoiceMode mode)
    {
        voiceMode = mode;
    }

    void setVelocityCurve(float curve)
    {
        velocityCurve = curve;
    }

    void setLFOParams(DSP::LFO::Waveform lfo1Wave, float lfo1Rate,
                      DSP::LFO::Waveform lfo2Wave, float lfo2Rate)
    {
        for (auto& voice : voices)
            voice.setLFOParams(lfo1Wave, lfo1Rate, lfo2Wave, lfo2Rate);
    }

    void noteOn(int midiNote, float velocity)
    {
        // Apply velocity curve: < 1.0 = soft response, > 1.0 = hard response
        velocity = std::pow(velocity, velocityCurve);
        // Handle mono/legato modes
        if (voiceMode == VoiceMode::Mono || voiceMode == VoiceMode::Legato)
        {
            handleMonoNoteOn(midiNote, velocity);
            return;
        }

        // Poly mode - handle unison
        float detuneStep = (unisonVoices > 1) ? unisonDetune / (unisonVoices - 1) : 0.0f;
        float spreadStep = (unisonVoices > 1) ? unisonSpread / (unisonVoices - 1) : 0.0f;

        for (int u = 0; u < unisonVoices; ++u)
        {
            SynthVoice* voice = findFreeVoice();
            if (voice == nullptr)
                voice = stealVoice(midiNote);

            if (voice != nullptr)
            {
                // Apply unison detune
                SynthVoice::Parameters p = voiceParams;
                if (unisonVoices > 1)
                {
                    float detune = -unisonDetune / 2.0f + detuneStep * u;
                    p.osc1Fine += detune;
                    p.osc2Fine += detune;

                    // Spread across stereo field
                    float pan = -unisonSpread / 2.0f + spreadStep * u;
                    p.osc1Pan = pan;
                    p.osc2Pan = pan;
                }

                voice->setParameters(p);
                voice->noteOn(midiNote, velocity);
            }
        }

        // Track note for release
        activeNotes[midiNote] = true;
    }

private:
    void handleMonoNoteOn(int midiNote, float velocity)
    {
        // Get current frequency from active voice for glide
        float currentFreq = 0.0f;
        SynthVoice* activeVoice = nullptr;

        for (auto& voice : voices)
        {
            if (voice.isVoiceActive())
            {
                activeVoice = &voice;
                currentFreq = voice.getCurrentFrequency();
                break;
            }
        }

        // Push note to stack for mono mode note priority
        monoNoteStack.push_back(midiNote);

        // Determine if this is a legato transition
        bool isLegato = (voiceMode == VoiceMode::Legato) && (activeVoice != nullptr);

        // In mono/legato, always use first voice (with unison if enabled)
        float detuneStep = (unisonVoices > 1) ? unisonDetune / (unisonVoices - 1) : 0.0f;
        float spreadStep = (unisonVoices > 1) ? unisonSpread / (unisonVoices - 1) : 0.0f;

        for (int u = 0; u < unisonVoices; ++u)
        {
            SynthVoice* voice = &voices[static_cast<size_t>(u)];

            // Apply unison detune
            SynthVoice::Parameters p = voiceParams;
            if (unisonVoices > 1)
            {
                float detune = -unisonDetune / 2.0f + detuneStep * u;
                p.osc1Fine += detune;
                p.osc2Fine += detune;

                // Spread across stereo field
                float pan = -unisonSpread / 2.0f + spreadStep * u;
                p.osc1Pan = pan;
                p.osc2Pan = pan;
            }

            voice->setParameters(p);
            voice->noteOn(midiNote, velocity, isLegato, currentFreq);
        }

        activeNotes[midiNote] = true;
    }

public:

    void noteOff(int midiNote)
    {
        activeNotes[midiNote] = false;

        // If sustain pedal is down, defer the note-off
        if (sustainPedalDown)
        {
            sustainedNotes[midiNote] = true;
            return;
        }

        // Handle mono/legato note stack
        if (voiceMode == VoiceMode::Mono || voiceMode == VoiceMode::Legato)
        {
            handleMonoNoteOff(midiNote);
            return;
        }

        // Poly mode
        for (auto& voice : voices)
        {
            if (voice.isVoiceActive() && voice.getMidiNote() == midiNote)
            {
                voice.noteOff();
            }
        }
    }

private:
    void handleMonoNoteOff(int midiNote)
    {
        // Remove note from stack
        auto it = std::find(monoNoteStack.begin(), monoNoteStack.end(), midiNote);
        if (it != monoNoteStack.end())
        {
            monoNoteStack.erase(it);
        }

        // Check if there are still held notes
        if (!monoNoteStack.empty())
        {
            // Return to previous note (last in stack)
            int prevNote = monoNoteStack.back();

            // Get current frequency for glide
            float currentFreq = 0.0f;
            for (auto& voice : voices)
            {
                if (voice.isVoiceActive())
                {
                    currentFreq = voice.getCurrentFrequency();
                    break;
                }
            }

            // Retrigger to previous note
            bool isLegato = (voiceMode == VoiceMode::Legato);
            float detuneStep = (unisonVoices > 1) ? unisonDetune / (unisonVoices - 1) : 0.0f;
            float spreadStep = (unisonVoices > 1) ? unisonSpread / (unisonVoices - 1) : 0.0f;

            for (int u = 0; u < unisonVoices; ++u)
            {
                SynthVoice* voice = &voices[static_cast<size_t>(u)];

                SynthVoice::Parameters p = voiceParams;
                if (unisonVoices > 1)
                {
                    float detune = -unisonDetune / 2.0f + detuneStep * u;
                    p.osc1Fine += detune;
                    p.osc2Fine += detune;
                    float pan = -unisonSpread / 2.0f + spreadStep * u;
                    p.osc1Pan = pan;
                    p.osc2Pan = pan;
                }

                voice->setParameters(p);
                voice->noteOn(prevNote, voice->getVelocity(), isLegato, currentFreq);
            }
        }
        else
        {
            // No more held notes - release
            for (int u = 0; u < unisonVoices; ++u)
            {
                if (voices[static_cast<size_t>(u)].isVoiceActive())
                {
                    voices[static_cast<size_t>(u)].noteOff();
                }
            }
        }
    }

public:

    void allNotesOff()
    {
        for (auto& voice : voices)
        {
            if (voice.isVoiceActive())
                voice.noteOff();
        }

        activeNotes.fill(false);
        sustainedNotes.fill(false);
        sustainPedalDown = false;
        monoNoteStack.clear();
    }

    void processBlock(juce::AudioBuffer<float>& buffer, bool clearBuffer = true)
    {
        if (clearBuffer)
            buffer.clear();

        float* left = buffer.getWritePointer(0);
        float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : left;

        const int numSamples = buffer.getNumSamples();

        // Use pre-allocated buffers (no real-time allocation)
        for (auto& voice : voices)
        {
            if (voice.isVoiceActive())
            {
                std::fill(voiceBufferLeft.begin(), voiceBufferLeft.begin() + numSamples, 0.0f);
                std::fill(voiceBufferRight.begin(), voiceBufferRight.begin() + numSamples, 0.0f);

                voice.processBlock(voiceBufferLeft.data(), voiceBufferRight.data(), numSamples);

                // Sum into output
                for (int i = 0; i < numSamples; ++i)
                {
                    left[i] += voiceBufferLeft[static_cast<size_t>(i)];
                    right[i] += voiceBufferRight[static_cast<size_t>(i)];
                }
            }
        }
    }

    void handleMidiMessage(const juce::MidiMessage& msg)
    {
        if (msg.isNoteOn())
        {
            noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
        }
        else if (msg.isNoteOff())
        {
            noteOff(msg.getNoteNumber());
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            allNotesOff();
        }
        else if (msg.isPitchWheel())
        {
            float pitchBend = (msg.getPitchWheelValue() - 8192) / 8192.0f;
            for (auto& voice : voices)
            {
                voice.getModMatrix().setSourceValue(
                    Modulation::ModSource::PitchBend, pitchBend);
            }
        }
        else if (msg.isController())
        {
            int cc = msg.getControllerNumber();
            float value = msg.getControllerValue() / 127.0f;

            if (cc == 1) // Mod wheel
            {
                for (auto& voice : voices)
                {
                    voice.getModMatrix().setSourceValue(
                        Modulation::ModSource::ModWheel, value);
                }
            }
            else if (cc == 64) // Sustain pedal
            {
                sustainPedalDown = (msg.getControllerValue() >= 64);

                if (!sustainPedalDown)
                {
                    // Pedal released — release all sustained notes
                    for (int note = 0; note < 128; ++note)
                    {
                        if (sustainedNotes[note])
                        {
                            sustainedNotes[note] = false;
                            // Only release if the key isn't still physically held
                            if (!activeNotes[note])
                            {
                                if (voiceMode == VoiceMode::Mono || voiceMode == VoiceMode::Legato)
                                {
                                    handleMonoNoteOff(note);
                                }
                                else
                                {
                                    for (auto& voice : voices)
                                    {
                                        if (voice.isVoiceActive() && voice.getMidiNote() == note)
                                            voice.noteOff();
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else if (msg.isAftertouch())
        {
            float at = msg.getAfterTouchValue() / 127.0f;
            for (auto& voice : voices)
            {
                voice.getModMatrix().setSourceValue(
                    Modulation::ModSource::Aftertouch, at);
            }
        }
    }

    int getActiveVoiceCount() const
    {
        int count = 0;
        for (const auto& voice : voices)
        {
            if (voice.isVoiceActive())
                ++count;
        }
        return count;
    }

    void reset()
    {
        for (auto& voice : voices)
            voice.reset();
        activeNotes.fill(false);
        sustainedNotes.fill(false);
        sustainPedalDown = false;
        monoNoteStack.clear();
    }

private:
    SynthVoice* findFreeVoice()
    {
        for (int i = 0; i < maxPolyphony * unisonVoices; ++i)
        {
            if (!voices[i].isVoiceActive())
                return &voices[i];
        }
        return nullptr;
    }

    SynthVoice* stealVoice(int newNote)
    {
        switch (stealingMode)
        {
            case VoiceStealingMode::Oldest:
            {
                // Just return the first active voice
                for (int i = 0; i < maxPolyphony * unisonVoices; ++i)
                {
                    if (voices[i].isVoiceActive())
                        return &voices[i];
                }
                break;
            }

            case VoiceStealingMode::Quietest:
            {
                SynthVoice* quietest = nullptr;
                float lowestVelocity = 2.0f;

                for (int i = 0; i < maxPolyphony * unisonVoices; ++i)
                {
                    if (voices[i].isVoiceActive() && voices[i].getVelocity() < lowestVelocity)
                    {
                        lowestVelocity = voices[i].getVelocity();
                        quietest = &voices[i];
                    }
                }
                return quietest;
            }

            case VoiceStealingMode::HighestNote:
            {
                SynthVoice* highest = nullptr;
                int highestNote = -1;

                for (int i = 0; i < maxPolyphony * unisonVoices; ++i)
                {
                    if (voices[i].isVoiceActive() && voices[i].getMidiNote() > highestNote)
                    {
                        highestNote = voices[i].getMidiNote();
                        highest = &voices[i];
                    }
                }
                return highest;
            }

            case VoiceStealingMode::LowestNote:
            {
                SynthVoice* lowest = nullptr;
                int lowestNote = 128;

                for (int i = 0; i < maxPolyphony * unisonVoices; ++i)
                {
                    if (voices[i].isVoiceActive() && voices[i].getMidiNote() < lowestNote)
                    {
                        lowestNote = voices[i].getMidiNote();
                        lowest = &voices[i];
                    }
                }
                return lowest;
            }
        }

        return &voices[0];
    }

    double sampleRate = 44100.0;
    int samplesPerBlock = 512;

    // Pre-allocated voice mixing buffers (avoids real-time allocation)
    std::vector<float> voiceBufferLeft;
    std::vector<float> voiceBufferRight;

    std::vector<SynthVoice> voices;
    SynthVoice::Parameters voiceParams;

    int maxPolyphony = 16;
    VoiceStealingMode stealingMode = VoiceStealingMode::Oldest;

    int unisonVoices = 1;
    float unisonDetune = 10.0f; // cents
    float unisonSpread = 1.0f;  // stereo spread

    VoiceMode voiceMode = VoiceMode::Poly;
    std::vector<int> monoNoteStack;  // For mono/legato note priority

    std::array<bool, 128> activeNotes{};

    // Sustain pedal state
    bool sustainPedalDown = false;
    std::array<bool, 128> sustainedNotes{};

    // Velocity curve: 1.0 = linear, < 1.0 = soft, > 1.0 = hard
    float velocityCurve = 1.0f;
};

} // namespace Engine
} // namespace NulyBeats
