; Demon Synth Windows Installer Script
; Requires Inno Setup 6.x (https://jrsoftware.org/isinfo.php)
;
; © 2024 Nolan Griffis p/k/a Nully Beats
; Nully Beats LLC / Producer Tour Publishing LLC

#define MyAppName "Demon Synth"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Nully Beats LLC"
#define MyAppURL "https://nullybeats.com"
#define MyAppCopyright "© 2024 Nolan Griffis p/k/a Nully Beats - Nully Beats LLC / Producer Tour Publishing LLC"

[Setup]
AppId={{8F4B3A2E-1D5C-4E7F-9A8B-2C3D4E5F6A7B}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
AppCopyright={#MyAppCopyright}
DefaultDirName={autopf}\{#MyAppPublisher}\{#MyAppName}
DefaultGroupName={#MyAppPublisher}
DisableProgramGroupPage=yes
LicenseFile=..\LICENSE.txt
OutputDir=..\Output
OutputBaseFilename=DemonSynth_v{#MyAppVersion}_Windows
; Note: Using default Inno Setup icon and wizard images
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayName={#MyAppName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Messages]
WelcomeLabel1=Welcome to the {#MyAppName} Setup
WelcomeLabel2=This will install {#MyAppName} {#MyAppVersion} on your computer.%n%n{#MyAppCopyright}%n%nIt is recommended that you close all other applications before continuing.
FinishedHeadingLabel=Completing the {#MyAppName} Setup
FinishedLabelNoIcons=Setup has finished installing {#MyAppName} on your computer.%n%nPlease restart your DAW (FL Studio, Ableton, etc.) to use the plugin.
FinishedLabel=Setup has finished installing {#MyAppName} on your computer.%n%nPlease restart your DAW (FL Studio, Ableton, etc.) to use the plugin.

[Types]
Name: "full"; Description: "Full installation (VST3 + Standalone + Sample Presets)"
Name: "vst3only"; Description: "VST3 plugin only"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 Plugin (for FL Studio, Ableton, etc.)"; Types: full vst3only custom; Flags: fixed
Name: "standalone"; Description: "Standalone Application"; Types: full custom
Name: "samples"; Description: "Sample Presets (108 sounds)"; Types: full custom

[Tasks]
Name: "desktopicon"; Description: "Create desktop shortcut"; GroupDescription: "Additional options:"; Components: standalone
Name: "startmenuicon"; Description: "Create Start Menu shortcut"; GroupDescription: "Additional options:"; Components: standalone

[Dirs]
; Sample presets directory - user can customize this
Name: "{code:GetSamplesDir}"; Components: samples

[Files]
; VST3 Plugin - Install to Common Files VST3 folder
Source: "..\..\build\NulyBeatsPlugin_artefacts\Release\VST3\Demon Synth.vst3\*"; DestDir: "{commoncf64}\VST3\Demon Synth.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs

; Standalone Application
Source: "..\..\build\NulyBeatsPlugin_artefacts\Release\Standalone\Demon Synth.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

; Sample Presets - Install to user-selected directory
Source: "..\..\Resources\Samples\*"; DestDir: "{code:GetSamplesDir}"; Components: samples; Flags: ignoreversion recursesubdirs createallsubdirs

; License file
Source: "..\LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\Demon Synth.exe"; Components: standalone
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\Demon Synth.exe"; Components: standalone; Tasks: desktopicon
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\Demon Synth.exe"; Components: standalone; Tasks: startmenuicon

[Registry]
; Store samples path in registry for plugin to read
Root: HKCU; Subkey: "Software\NullyBeats\Demon Synth"; ValueType: string; ValueName: "SamplesPath"; ValueData: "{code:GetSamplesDir}"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\NullyBeats\Demon Synth"; ValueType: string; ValueName: "Version"; ValueData: "{#MyAppVersion}"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\NullyBeats\Demon Synth"; ValueType: string; ValueName: "InstallDate"; ValueData: "{code:GetCurrentDate}"; Flags: uninsdeletekey

[Run]
Filename: "{app}\Demon Synth.exe"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent; Components: standalone

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Demon Synth.vst3"
Type: dirifempty; Name: "{code:GetSamplesDir}"

; Clean up FL Studio plugin database cache (if FL Studio is installed)
; Generators - VST3
Type: files; Name: "{userdocs}\Image-Line\FL Studio\Presets\Plugin database\Installed\Generators\VST3\Demon Synth.fst"
Type: files; Name: "{userdocs}\Image-Line\FL Studio\Presets\Plugin database\Installed\Generators\VST3\Demon Synth.nfo"
Type: files; Name: "{userdocs}\Image-Line\FL Studio\Presets\Plugin database\Installed\Generators\VST3\NulyBeats Synth.fst"
Type: files; Name: "{userdocs}\Image-Line\FL Studio\Presets\Plugin database\Installed\Generators\VST3\NulyBeats Synth.nfo"
Type: files; Name: "{userdocs}\Image-Line\FL Studio\Presets\Plugin database\Installed\Generators\VST3\Nuly Beats.fst"
Type: files; Name: "{userdocs}\Image-Line\FL Studio\Presets\Plugin database\Installed\Generators\VST3\Nuly Beats.nfo"

; Generators - New (FL Studio puts newly scanned plugins here)
Type: files; Name: "{userdocs}\Image-Line\FL Studio\Presets\Plugin database\Installed\Generators\New\Demon Synth.fst"
Type: files; Name: "{userdocs}\Image-Line\FL Studio\Presets\Plugin database\Installed\Generators\New\Demon Synth.nfo"
Type: files; Name: "{userdocs}\Image-Line\FL Studio\Presets\Plugin database\Installed\Generators\New\NulyBeats Synth.fst"
Type: files; Name: "{userdocs}\Image-Line\FL Studio\Presets\Plugin database\Installed\Generators\New\NulyBeats Synth.nfo"

; Effects - VST3 (in case plugin was miscategorized)
Type: files; Name: "{userdocs}\Image-Line\FL Studio\Presets\Plugin database\Installed\Effects\VST3\Demon Synth.fst"
Type: files; Name: "{userdocs}\Image-Line\FL Studio\Presets\Plugin database\Installed\Effects\VST3\Demon Synth.nfo"

; Non-Installed path (FL Studio sometimes puts entries here too)
Type: files; Name: "{userdocs}\Image-Line\FL Studio\Presets\Plugin database\Generators\Demon Synth.fst"
Type: files; Name: "{userdocs}\Image-Line\FL Studio\Presets\Plugin database\Generators\Demon Synth.nfo"
Type: files; Name: "{userdocs}\Image-Line\FL Studio\Presets\Plugin database\Generators\NulyBeats Synth.fst"
Type: files; Name: "{userdocs}\Image-Line\FL Studio\Presets\Plugin database\Generators\NulyBeats Synth.nfo"
Type: files; Name: "{userdocs}\Image-Line\FL Studio\Presets\Plugin database\Generators\Nuly Beats.fst"
Type: files; Name: "{userdocs}\Image-Line\FL Studio\Presets\Plugin database\Generators\Nuly Beats.nfo"
Type: files; Name: "{userdocs}\Image-Line\FL Studio\Presets\Plugin database\Effects\Demon Synth.fst"
Type: files; Name: "{userdocs}\Image-Line\FL Studio\Presets\Plugin database\Effects\Demon Synth.nfo"

[Code]
var
  SamplesDirPage: TInputDirWizardPage;
  SamplesDir: String;

function GetSamplesDir(Param: String): String;
begin
  if SamplesDir = '' then
    SamplesDir := ExpandConstant('{userappdata}\NullyBeats\Demon Synth\Samples');
  Result := SamplesDir;
end;

function GetCurrentDate(Param: String): String;
begin
  Result := GetDateTimeString('yyyy-mm-dd', '-', ':');
end;

procedure InitializeWizard;
begin
  // Create custom page for samples directory selection
  SamplesDirPage := CreateInputDirPage(
    wpSelectComponents,
    'Sample Presets Location',
    'Where should the sample presets be installed?',
    'The plugin will look for sample presets in this folder.' + #13#10 + #13#10 +
    'You can change this location if you want to store samples on a different drive.' + #13#10 +
    'Click Next to use the default folder, or click Browse to choose a different folder.',
    False,
    'New Folder'
  );

  // Set default samples directory
  SamplesDirPage.Add('');
  SamplesDirPage.Values[0] := ExpandConstant('{userappdata}\NullyBeats\Demon Synth\Samples');
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  // Skip samples directory page if samples component is not selected
  if PageID = SamplesDirPage.ID then
    Result := not WizardIsComponentSelected('samples')
  else
    Result := False;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;

  // Save samples directory when leaving the custom page
  if CurPageID = SamplesDirPage.ID then
  begin
    SamplesDir := SamplesDirPage.Values[0];

    // Validate directory
    if SamplesDir = '' then
    begin
      MsgBox('Please select a valid directory for sample presets.', mbError, MB_OK);
      Result := False;
    end;
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ConfigDir: String;
  ConfigFile: String;
  ConfigContent: String;
begin
  if CurStep = ssPostInstall then
  begin
    // Create config file for plugin to read
    ConfigDir := ExpandConstant('{userappdata}\NullyBeats\Demon Synth');
    ForceDirectories(ConfigDir);

    ConfigFile := ConfigDir + '\config.json';
    ConfigContent := '{' + #13#10 +
      '    "version": "' + ExpandConstant('{#MyAppVersion}') + '",' + #13#10 +
      '    "samplesPath": "' + StringReplace(GetSamplesDir(''), '\', '\\', [rfReplaceAll]) + '",' + #13#10 +
      '    "installedDate": "' + GetCurrentDate('') + '",' + #13#10 +
      '    "licenseAccepted": true' + #13#10 +
      '}';

    SaveStringToFile(ConfigFile, ConfigContent, False);
  end;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  // Update samples directory from page when changed
  if CurPageID = SamplesDirPage.ID then
  begin
    // Pre-populate with current value
    if SamplesDir <> '' then
      SamplesDirPage.Values[0] := SamplesDir;
  end;
end;

function UpdateReadyMemo(Space, NewLine, MemoUserInfoInfo, MemoDirInfo, MemoTypeInfo, MemoComponentsInfo, MemoGroupInfo, MemoTasksInfo: String): String;
var
  S: String;
begin
  S := '';

  S := S + 'Installation Summary:' + NewLine + NewLine;

  if WizardIsComponentSelected('vst3') then
    S := S + Space + 'VST3 Plugin: ' + ExpandConstant('{commoncf64}\VST3\Demon Synth.vst3') + NewLine;

  if WizardIsComponentSelected('standalone') then
    S := S + Space + 'Standalone App: ' + ExpandConstant('{app}\Demon Synth.exe') + NewLine;

  if WizardIsComponentSelected('samples') then
    S := S + Space + 'Sample Presets: ' + GetSamplesDir('') + NewLine;

  S := S + NewLine;
  S := S + 'After installation, restart your DAW to see the plugin.' + NewLine;

  Result := S;
end;
