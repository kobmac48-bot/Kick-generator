#define AppVersion "1.2.0"

#ifndef PluginBundle
  #error "Pass /DPluginBundle=<path to KickForge.vst3>"
#endif

#ifndef OutputDir
  #error "Pass /DOutputDir=<installer output directory>"
#endif

[Setup]
AppId={{A9E3C07C-4D5C-4D5F-AE3A-515AEF6F15CE}
AppName=KickForge
AppVersion={#AppVersion}
AppPublisher=KickForge
DefaultDirName={commoncf}\VST3\KickForge.vst3
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
CloseApplications=yes
OutputDir={#OutputDir}
OutputBaseFilename=KickForge-Setup-{#AppVersion}-x64
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=KickForge VST3
UninstallDisplayIcon={app}\Contents\x86_64-win\KickForge.vst3

[Files]
Source: "{#PluginBundle}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
