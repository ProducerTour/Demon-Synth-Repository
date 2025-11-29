-- Demon Synth Installer
-- © 2024 Nolan Griffis p/k/a Nully Beats
-- Nully Beats LLC / Producer Tour Publishing LLC

use AppleScript version "2.4"
use scripting additions

-- Get the path to this app bundle
set myPath to (path to me) as text
set installerFolder to (do shell script "dirname " & quoted form of POSIX path of myPath)

-- Default paths
set defaultVST3Path to (POSIX path of (path to home folder)) & "Library/Audio/Plug-Ins/VST3"
set defaultAUPath to (POSIX path of (path to home folder)) & "Library/Audio/Plug-Ins/Components"
set defaultAppPath to "/Applications"
set defaultSamplesPath to (POSIX path of (path to home folder)) & "Library/Application Support/NullyBeats/Demon Synth/Samples"

-- License text
set licenseText to "DEMON SYNTH END USER LICENSE AGREEMENT

Copyright © 2024 Nolan Griffis p/k/a Nully Beats
Nully Beats LLC and Producer Tour Publishing LLC
All Rights Reserved.

IMPORTANT: By installing this software, you agree to be bound by the terms of this agreement.

1. GRANT OF LICENSE
You are granted a non-exclusive, non-transferable license to use this software for personal or commercial music production.

2. SAMPLE CONTENT LICENSE
You may use the included samples in your original compositions and distribute them commercially. You may NOT redistribute the samples as standalone content.

3. RESTRICTIONS
You may not copy, modify, reverse engineer, or redistribute this software.

4. DISCLAIMER
THE SOFTWARE IS PROVIDED \"AS IS\" WITHOUT WARRANTY OF ANY KIND.

5. CONTACT
Website: https://nullybeats.com
Email: support@nullybeats.com

Full license available at: https://nullybeats.com/license"

-- ====================================
-- STEP 1: Welcome Dialog
-- ====================================
set welcomeResult to display dialog "Welcome to the Demon Synth Installer

Version 1.0.0

© 2024 Nolan Griffis p/k/a Nully Beats
Nully Beats LLC / Producer Tour Publishing LLC

This installer will guide you through the installation process." buttons {"Cancel", "Continue"} default button "Continue" with title "Demon Synth Installer" with icon note

if button returned of welcomeResult is "Cancel" then
	return
end if

-- ====================================
-- STEP 2: License Agreement
-- ====================================
set licenseResult to display dialog licenseText buttons {"Decline", "I Agree"} default button "I Agree" with title "License Agreement" with icon note

if button returned of licenseResult is "Decline" then
	display dialog "You must accept the license agreement to install Demon Synth." buttons {"OK"} default button "OK" with title "Installation Cancelled" with icon stop
	return
end if

-- ====================================
-- STEP 3: Installation Type
-- ====================================
set installType to choose from list {"Full Installation (Recommended)", "VST3 Plugin Only", "Custom Installation"} with title "Installation Type" with prompt "Choose installation type:" default items {"Full Installation (Recommended)"}

if installType is false then
	return
end if

set installType to item 1 of installType

-- ====================================
-- STEP 4: Choose Sample Presets Location
-- ====================================
set samplesPath to defaultSamplesPath

if installType is not "VST3 Plugin Only" then
	set samplesDialog to display dialog "Choose where to install Sample Presets:

The plugin will look for samples in this folder.

Default: " & defaultSamplesPath buttons {"Use Default", "Choose Folder...", "Cancel"} default button "Use Default" with title "Sample Presets Location" with icon note

	if button returned of samplesDialog is "Cancel" then
		return
	else if button returned of samplesDialog is "Choose Folder..." then
		try
			set samplesFolder to choose folder with prompt "Select folder for Sample Presets:"
			set samplesPath to POSIX path of samplesFolder
		on error
			return
		end try
	end if
end if

-- ====================================
-- STEP 5: Confirmation
-- ====================================
set confirmText to "Ready to Install Demon Synth

The following will be installed:
"

if installType is "Full Installation (Recommended)" or installType is "Custom Installation" then
	set confirmText to confirmText & "
• VST3 Plugin: " & defaultVST3Path & "
• AU Plugin: " & defaultAUPath & "
• Standalone App: " & defaultAppPath & "
• Uninstaller: " & defaultAppPath & "
• Sample Presets: " & samplesPath
else
	set confirmText to confirmText & "
• VST3 Plugin: " & defaultVST3Path & "
• Uninstaller: " & defaultAppPath
end if

set confirmResult to display dialog confirmText buttons {"Cancel", "Install"} default button "Install" with title "Confirm Installation" with icon note

if button returned of confirmResult is "Cancel" then
	return
end if

-- ====================================
-- STEP 6: Installation
-- ====================================
set progressInfo to display dialog "Installing Demon Synth...

Please wait while files are copied." buttons {} giving up after 1 with title "Installing..." with icon note

try
	-- Create directories
	do shell script "mkdir -p " & quoted form of defaultVST3Path
	do shell script "mkdir -p " & quoted form of defaultAUPath
	do shell script "mkdir -p " & quoted form of samplesPath
	do shell script "mkdir -p " & quoted form of ((POSIX path of (path to home folder)) & "Library/Application Support/NullyBeats/Demon Synth")

	-- Install VST3
	if (do shell script "test -d " & quoted form of (installerFolder & "/Demon Synth.vst3") & " && echo 'yes' || echo 'no'") is "yes" then
		do shell script "rm -rf " & quoted form of (defaultVST3Path & "/Demon Synth.vst3")
		do shell script "cp -R " & quoted form of (installerFolder & "/Demon Synth.vst3") & " " & quoted form of defaultVST3Path
		do shell script "xattr -rd com.apple.quarantine " & quoted form of (defaultVST3Path & "/Demon Synth.vst3") & " 2>/dev/null || true"
	end if

	-- Install AU
	if installType is not "VST3 Plugin Only" then
		if (do shell script "test -d " & quoted form of (installerFolder & "/Demon Synth.component") & " && echo 'yes' || echo 'no'") is "yes" then
			do shell script "rm -rf " & quoted form of (defaultAUPath & "/Demon Synth.component")
			do shell script "cp -R " & quoted form of (installerFolder & "/Demon Synth.component") & " " & quoted form of defaultAUPath
			do shell script "xattr -rd com.apple.quarantine " & quoted form of (defaultAUPath & "/Demon Synth.component") & " 2>/dev/null || true"
			do shell script "killall -9 AudioComponentRegistrar 2>/dev/null || true"
		end if
	end if

	-- Install Standalone App
	if installType is not "VST3 Plugin Only" then
		if (do shell script "test -d " & quoted form of (installerFolder & "/Demon Synth.app") & " && echo 'yes' || echo 'no'") is "yes" then
			do shell script "rm -rf " & quoted form of (defaultAppPath & "/Demon Synth.app")
			do shell script "cp -R " & quoted form of (installerFolder & "/Demon Synth.app") & " " & quoted form of defaultAppPath
			do shell script "xattr -rd com.apple.quarantine " & quoted form of (defaultAppPath & "/Demon Synth.app") & " 2>/dev/null || true"
		end if
	end if

	-- Install Uninstaller
	if (do shell script "test -d " & quoted form of (installerFolder & "/Uninstall Demon Synth.app") & " && echo 'yes' || echo 'no'") is "yes" then
		do shell script "rm -rf " & quoted form of (defaultAppPath & "/Uninstall Demon Synth.app")
		do shell script "cp -R " & quoted form of (installerFolder & "/Uninstall Demon Synth.app") & " " & quoted form of defaultAppPath
		do shell script "xattr -rd com.apple.quarantine " & quoted form of (defaultAppPath & "/Uninstall Demon Synth.app") & " 2>/dev/null || true"
	end if

	-- Install Samples
	if installType is not "VST3 Plugin Only" then
		if (do shell script "test -d " & quoted form of (installerFolder & "/Samples") & " && echo 'yes' || echo 'no'") is "yes" then
			do shell script "cp -R " & quoted form of (installerFolder & "/Samples/") & "* " & quoted form of samplesPath & " 2>/dev/null || true"
		end if
	end if

	-- Write config file
	set configDir to (POSIX path of (path to home folder)) & "Library/Application Support/NullyBeats/Demon Synth"
	set configContent to "{
    \"version\": \"1.0.0\",
    \"samplesPath\": \"" & samplesPath & "\",
    \"installedDate\": \"" & (do shell script "date -u +\"%Y-%m-%dT%H:%M:%SZ\"") & "\",
    \"licenseAccepted\": true
}"
	do shell script "echo " & quoted form of configContent & " > " & quoted form of (configDir & "/config.json")

	-- Success!
	display dialog "Installation Complete!

Demon Synth has been successfully installed.

Please restart your DAW (FL Studio, Logic Pro, etc.) to use the plugin.

Sample presets are located at:
" & samplesPath & "

To uninstall, run 'Uninstall Demon Synth' from Applications.

Thank you for choosing Demon Synth!

© 2024 Nully Beats LLC / Producer Tour Publishing LLC" buttons {"OK"} default button "OK" with title "Installation Complete" with icon note

on error errMsg
	display dialog "Installation Error:

" & errMsg & "

Please try running the installer again, or install manually." buttons {"OK"} default button "OK" with title "Installation Failed" with icon stop
end try
