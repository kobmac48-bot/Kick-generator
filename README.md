# KickForge

KickForge is a MIDI-triggered VST3 kick instrument for hardcore and hard-dance
production. Choose a style and play MIDI notes to generate kicks directly in
your DAW.

## Styles

Gabber, Uptempo, Zaag, Rawstyle, Deutsche Krach, Frenchcore, and Terror each
have a distinct pitch sweep, decay, saturation, punch, and brightness profile.
The Pitch Start, Pitch End, Decay, Drive, Punch, and Body controls shape the
selected style. All controls are host-automatable and saved with the DAW project.
Choose a style and click **Random Kick** to create a fresh variation using ranges
tuned for that genre; the style stays selected while its six controls are randomized
and the kick is previewed. MIDI notes can also trigger the selected kick in your DAW.

## Build

Requirements: CMake 3.22+, a C++17 compiler, and Git. CMake downloads JUCE 8.0.6
on the first configure.

```powershell
cmake -S . -B build
cmake --build build --config Release
```

The VST3 bundle is written under `build/KickForge_artefacts/Release/VST3`.
Copy it to your system's VST3 plug-in folder or point your DAW at that folder.

The instrument has no audio input: route a MIDI track to KickForge and send
notes to trigger one-shot kicks.
