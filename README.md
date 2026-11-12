# KickForge

KickForge is a MIDI-triggered VST3 kick instrument for hardcore and hard-dance
production. Choose a style and play MIDI notes to generate kicks directly in
your DAW.

## Styles

Gabber, Uptempo, Bouncy Uptempo, Zaag, Rawstyle, Deutsche Krach, Frenchcore,
and Terror each have a distinct pitch sweep, decay, saturation, punch, and
brightness profile. Bouncy Uptempo adds a decaying rhythmic tail pulse, while
standard Uptempo stays direct. Deutsche Krach is brighter and crunchier;
Frenchcore has a shorter pitch sweep and a sharper click.
Kicks start with a short click, drop quickly from a high pitch into a solid low
fundamental, then fade through a tone-shaped distorted tail. The Pitch Start,
Pitch End, Decay, Drive, Punch, Body, Distortion, and Soft Clip controls shape
the selected style. Uptempo and Deutsche Krach random kicks emphasize a harder
transient, faster pitch drop, saturated midrange, and tightly clipped tail.
Distortion and Soft Clip are separate, adjustable controls; set either to zero
for a cleaner signal. All controls are host-automatable and saved with the DAW
project.
Choose a style and click **Random Kick** to create and preview a fresh variation
using ranges tuned for that genre; the style stays selected while its controls are
randomized. MIDI notes can also trigger the selected kick in your DAW.

## Build

Requirements: CMake 3.22+, a C++17 compiler, and Git. CMake downloads JUCE 8.0.6
on the first configure.

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The VST3 bundle is written under `build/KickForge_artefacts/Release/VST3`.
Copy it to your system's VST3 plug-in folder or point your DAW at that folder.

On Windows, install Inno Setup 6 and build the installer after building the
VST3. This writes `KickForge-Setup-1.2.0-x64.exe`:

```powershell
& "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe" `
  "/DPluginBundle=$PWD\build\KickForge_artefacts\Release\VST3\KickForge.vst3" `
  "/DOutputDir=$PWD\dist" installer\KickForge.iss
```

The versioned installer is written to `dist`.

The instrument has no audio input: route a MIDI track to KickForge and send
notes to trigger one-shot kicks.

## Use in FL Studio

1. Run `KickForge-Setup-1.1.0-x64.exe` and approve the Windows administrator
   prompt. The installer copies the complete VST3 bundle to
   `C:\Program Files\Common Files\VST3\KickForge.vst3`; uninstall it from
   Windows **Installed apps**.
2. In FL Studio, open **Options > Manage plugins**. Make sure the VST3 folder
   is included in the plugin search paths, then click **Find installed plugins**.
3. Add **KickForge** as an instrument from the plugin database. Create or select
   a Channel Rack pattern and add notes in the Piano roll to trigger kicks.
   Choose a genre and click **Random Kick** to randomize and preview a kick in
   that style.

The `KickForge-VST3.zip` release asset is also available for manual installation:
extract the complete `KickForge.vst3` folder into the shared VST3 folder above.
