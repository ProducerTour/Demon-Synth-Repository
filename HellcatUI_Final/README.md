# Hellcat VST UI Components

## What's Included

8 header-only JUCE components for a Hellcat-themed plugin UI:

- `HellcatLookAndFeel.h` - Custom styling
- `HellcatGauge.h` - Circular gauges  
- `HellcatEnvelopeDisplay.h` - ADSR visualization
- `HellcatXYPad.h` - 2D controller
- `HellcatModMatrix.h` - Mod matrix
- `HellcatMacroKnob.h` - Macro knobs
- `HellcatTransportButton.h` - Transport buttons
- `HellcatTopBar.h` - Top bar with logo/preset/modes

## Prompt for Claude Code

Copy this and paste to Claude Code along with your project and this HellcatUI_Final folder:

---

I need to integrate the Hellcat UI components into my JUCE plugin.

**Steps:**
1. Copy the UI folder into my Source/ directory
2. Find all my APVTS parameter IDs in PluginProcessor
3. Integrate components into PluginEditor with this layout:
   - Window: 1280x720 (resizable)
   - Top: Logo, preset browser, mode switches, meter
   - Left gauge: Oscillator voices (connect to voices/unison parameter)
   - Right gauge: Filter cutoff in kHz (connect to cutoff parameter)
   - Center: Tabbed (Mod Matrix / ENVELOPES default / LFOs)
   - Bottom: XY pad, 4 macro knobs (BOOST/AIR/BODY/WARP), 3 transport buttons

**Connections needed:**
- Oscillator gauge → voices/unison parameter
- Filter gauge → filter cutoff parameter (convert to kHz)
- Envelope display → attack, decay, sustain, release parameters
- Macro knobs → create new params if they don't exist (0-100 range)
- XY pad → width and fx_send params (skip if they don't exist)

**Technical:**
- Apply HellcatLookAndFeel in constructor
- Update values at 30Hz in timerCallback()
- Clean up in destructor (setLookAndFeel(nullptr))

---

## That's It!

Claude Code will handle everything - finding your parameters, integrating the components, and wiring it all up.
