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
