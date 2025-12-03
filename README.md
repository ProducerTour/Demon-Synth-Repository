# NulyBeats Synth

A professional-grade hybrid ROMpler + VA/Wavetable synthesizer VST3 plugin, inspired by Roland Zenology.

## Features

### Sound Engines
- **Dual Oscillators** - VA synthesis with anti-aliased waveforms (PolyBLEP)
  - Sine, Saw, Square, Triangle, Pulse (with PWM), Noise
  - Per-oscillator octave, semitone, fine tuning
  - Per-oscillator level and pan
- **Wavetable Engine** - 256-slot wavetable with morphing
  - Bandlimited mipmaps for alias-free playback
  - Cubic interpolation
- **PCM Sample Engine** - Full ROMpler capabilities
  - Velocity layers
  - Round-robin samples
  - Multisample zones
  - Loop with crossfade
  - Real-time time-stretch (phase vocoder)

### Filters
- **SVF Filter** - Zero-delay feedback topology
  - Low Pass, High Pass, Band Pass, Notch
  - Resonance to self-oscillation
- **Ladder Filter** - Moog-style 24dB/oct
  - Variable slope (6/12/18/24 dB)
  - Drive/saturation
  - Accurate ZDF modeling

### Modulation
- **4x ADSR Envelopes** - Amp, Filter, Mod 1, Mod 2
  - Adjustable curves
  - Legato mode
- **4x LFOs** - Tempo-syncable
  - Multiple waveforms including S&H
  - Fade-in
- **Full Mod Matrix** - 32 routing slots
  - Any source to any destination
  - Via modulation (modulate the amount)

### Effects Rack
- Distortion (soft clip, hard clip, tube, foldback, bitcrush)
- 3-Band Parametric EQ
- Compressor
- Chorus
- Delay (with ping-pong)
- Reverb

### Performance
- 64-voice polyphony
- Unison with detune and stereo spread
- Portamento/Glide
- SIMD optimizations (AVX2)

## Building

### Prerequisites
- CMake 3.22+
- C++20 compiler (Clang 14+, GCC 11+, MSVC 2022+)
- macOS: Xcode Command Line Tools
- Windows: Visual Studio 2022
- Linux: ALSA development libraries

### Quick Start
```bash
# Clone and setup
cd NulyBeatsPlugin
./setup.sh

# Build
cd build
cmake --build . --config Release
```

### Manual Build
```bash
# Clone JUCE
git clone --depth 1 --branch 7.0.9 https://github.com/juce-framework/JUCE.git

# Configure
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release
```

### Output Locations
- **macOS**: `build/NulyBeatsPlugin_artefacts/Release/VST3/NulyBeats Synth.vst3`
- **macOS AU**: `build/NulyBeatsPlugin_artefacts/Release/AU/NulyBeats Synth.component`
- **Windows**: `build\NulyBeatsPlugin_artefacts\Release\VST3\NulyBeats Synth.vst3`

## Installing Sound Libraries

The sound library samples are distributed separately from the plugin to reduce download size.

### Windows
1. **Download the installer** from the releases page
2. During installation, you'll be prompted to select a location for sample presets
3. Default location: `C:\Users\[YourUsername]\AppData\Roaming\NullyBeats\Demon Synth\Samples`
4. **Extract the sample library** to the chosen directory
5. Restart your DAW to access the samples

### macOS
1. **Download the plugin** and **sample library** from the releases page
2. **Install the plugin**:
   - VST3: Copy `Demon Synth.vst3` to `~/Library/Audio/Plug-Ins/VST3/`
   - AU: Copy `Demon Synth.component` to `~/Library/Audio/Plug-Ins/Components/`
3. **Extract the sample library** to: `~/Library/Application Support/NullyBeats/Demon Synth/Samples`
4. Create the directory if it doesn't exist
5. Restart your DAW to access the samples

### Linux
1. **Download the plugin** and **sample library** from the releases page
2. **Install the plugin**: Copy the VST3 folder to `~/.vst3/`
3. **Extract the sample library** to: `~/.local/share/NullyBeats/Demon Synth/Samples`
4. Restart your DAW to access the samples

### Troubleshooting
- **Samples not loading?** Ensure the samples are extracted to the correct directory path
- **Wrong path?** The plugin stores the samples path in:
  - Windows: Registry at `HKEY_CURRENT_USER\Software\NullyBeats\Demon Synth\SamplesPath`
  - macOS/Linux: Check the plugin's preferences

## Project Structure

```
NulyBeatsPlugin/
├── Source/
│   ├── Core/                    # Plugin processor & editor
│   │   ├── PluginProcessor.cpp
│   │   ├── PluginEditor.cpp
│   │   └── Parameters.cpp
│   ├── DSP/
│   │   ├── Oscillators/         # VA & Wavetable oscillators
│   │   ├── Filters/             # SVF & Ladder filters
│   │   ├── Effects/             # FX rack
│   │   └── Modulators/          # ADSR, LFO
│   ├── Engine/
│   │   ├── PCM/                 # Sample playback engine
│   │   ├── Wavetable/           # Wavetable engine
│   │   └── Voice/               # Voice management
│   ├── Modulation/              # Mod matrix system
│   ├── UI/                      # Custom UI components
│   └── Utils/                   # SIMD helpers
├── Resources/
│   ├── Samples/                 # PCM sample library
│   ├── Wavetables/              # Wavetable files
│   ├── Presets/                 # Factory presets
│   └── Graphics/                # UI graphics
├── Tests/                       # Unit tests
├── CMakeLists.txt
└── setup.sh
```

## Extending

### Adding Wavetables
Place `.wav` files in `Resources/Wavetables/`. Each file should contain a single cycle waveform (recommended: 2048 samples).

### Adding Samples
Create folders in `Resources/Samples/` with the structure:
```
InstrumentName/
├── C2_vel1.wav
├── C2_vel2.wav
├── C2_vel3.wav
├── C3_vel1.wav
...
```

### Creating Presets
Presets are XML files saved via the plugin's preset management.

## License

Copyright (c) 2025 NulyBeats. All rights reserved.
