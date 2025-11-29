-- Demon Synth Uninstaller
-- © 2024 Nolan Griffis p/k/a Nully Beats
-- Nully Beats LLC / Producer Tour Publishing LLC

use AppleScript version "2.4"
use scripting additions

-- Default paths
set homeFolder to POSIX path of (path to home folder)
set vst3Path to homeFolder & "Library/Audio/Plug-Ins/VST3/Demon Synth.vst3"
set auPath to homeFolder & "Library/Audio/Plug-Ins/Components/Demon Synth.component"
set appPath to "/Applications/Demon Synth.app"
set configDir to homeFolder & "Library/Application Support/NullyBeats/Demon Synth"
set samplesDir to homeFolder & "Library/Application Support/NullyBeats/Demon Synth/Samples"

-- ====================================
-- STEP 1: Confirmation Dialog
-- ====================================
set confirmResult to display dialog "Demon Synth Uninstaller

This will remove:
• VST3 Plugin
• AU Plugin
• Standalone Application
• Configuration files
• FL Studio plugin cache (if present)

Note: Sample presets will NOT be deleted by default.

© 2024 Nully Beats LLC / Producer Tour Publishing LLC" buttons {"Cancel", "Uninstall"} default button "Cancel" with title "Uninstall Demon Synth" with icon caution

if button returned of confirmResult is "Cancel" then
	return
end if

-- ====================================
-- STEP 2: Ask about samples
-- ====================================
set samplesResult to display dialog "Do you want to also delete the sample presets?

Location: " & samplesDir & "

Choose 'Keep Samples' if you plan to reinstall later." buttons {"Keep Samples", "Delete Everything"} default button "Keep Samples" with title "Sample Presets" with icon note

set deleteSamples to (button returned of samplesResult is "Delete Everything")

-- ====================================
-- STEP 3: Uninstall
-- ====================================
set progressInfo to display dialog "Uninstalling Demon Synth...

Please wait." buttons {} giving up after 1 with title "Uninstalling..." with icon note

try
	-- Remove VST3
	do shell script "rm -rf " & quoted form of vst3Path & " 2>/dev/null || true"

	-- Remove AU
	do shell script "rm -rf " & quoted form of auPath & " 2>/dev/null || true"

	-- Reset AU cache
	do shell script "killall -9 AudioComponentRegistrar 2>/dev/null || true"

	-- Remove Standalone App
	do shell script "rm -rf " & quoted form of appPath & " 2>/dev/null || true"

	-- Remove config (but not samples unless requested)
	do shell script "rm -f " & quoted form of (configDir & "/config.json") & " 2>/dev/null || true"

	-- Remove FL Studio plugin cache entries (if FL Studio is installed)
	-- Clean up all variations of plugin names (Demon Synth, NulyBeats Synth, etc.)
	set flPluginDbPath to homeFolder & "Documents/Image-Line/FL Studio/Presets/Plugin database/Installed/"

	-- Generators - AudioUnit
	do shell script "rm -f " & quoted form of (flPluginDbPath & "Generators/AudioUnit/Demon Synth.fst") & " 2>/dev/null || true"
	do shell script "rm -f " & quoted form of (flPluginDbPath & "Generators/AudioUnit/Demon Synth.nfo") & " 2>/dev/null || true"
	do shell script "rm -f " & quoted form of (flPluginDbPath & "Generators/AudioUnit/NulyBeats Synth.fst") & " 2>/dev/null || true"
	do shell script "rm -f " & quoted form of (flPluginDbPath & "Generators/AudioUnit/NulyBeats Synth.nfo") & " 2>/dev/null || true"
	do shell script "rm -f " & quoted form of (flPluginDbPath & "Generators/AudioUnit/Nuly Beats.fst") & " 2>/dev/null || true"
	do shell script "rm -f " & quoted form of (flPluginDbPath & "Generators/AudioUnit/Nuly Beats.nfo") & " 2>/dev/null || true"

	-- Generators - VST3
	do shell script "rm -f " & quoted form of (flPluginDbPath & "Generators/VST3/Demon Synth.fst") & " 2>/dev/null || true"
	do shell script "rm -f " & quoted form of (flPluginDbPath & "Generators/VST3/Demon Synth.nfo") & " 2>/dev/null || true"
	do shell script "rm -f " & quoted form of (flPluginDbPath & "Generators/VST3/NulyBeats Synth.fst") & " 2>/dev/null || true"
	do shell script "rm -f " & quoted form of (flPluginDbPath & "Generators/VST3/NulyBeats Synth.nfo") & " 2>/dev/null || true"
	do shell script "rm -f " & quoted form of (flPluginDbPath & "Generators/VST3/Nuly Beats.fst") & " 2>/dev/null || true"
	do shell script "rm -f " & quoted form of (flPluginDbPath & "Generators/VST3/Nuly Beats.nfo") & " 2>/dev/null || true"

	-- Effects (in case it was miscategorized)
	do shell script "rm -f " & quoted form of (flPluginDbPath & "Effects/VST3/Demon Synth.fst") & " 2>/dev/null || true"
	do shell script "rm -f " & quoted form of (flPluginDbPath & "Effects/VST3/Demon Synth.nfo") & " 2>/dev/null || true"
	do shell script "rm -f " & quoted form of (flPluginDbPath & "Effects/AudioUnit/Demon Synth.fst") & " 2>/dev/null || true"
	do shell script "rm -f " & quoted form of (flPluginDbPath & "Effects/AudioUnit/Demon Synth.nfo") & " 2>/dev/null || true"

	-- Generators/New (FL Studio puts newly scanned plugins here)
	do shell script "rm -f " & quoted form of (flPluginDbPath & "Generators/New/Demon Synth.fst") & " 2>/dev/null || true"
	do shell script "rm -f " & quoted form of (flPluginDbPath & "Generators/New/Demon Synth.nfo") & " 2>/dev/null || true"
	do shell script "rm -f " & quoted form of (flPluginDbPath & "Generators/New/NulyBeats Synth.fst") & " 2>/dev/null || true"
	do shell script "rm -f " & quoted form of (flPluginDbPath & "Generators/New/NulyBeats Synth.nfo") & " 2>/dev/null || true"

	-- Also clean non-Installed path (FL Studio sometimes puts entries here too)
	set flPluginDbPathAlt to homeFolder & "Documents/Image-Line/FL Studio/Presets/Plugin database/"
	do shell script "rm -f " & quoted form of (flPluginDbPathAlt & "Generators/Demon Synth.fst") & " 2>/dev/null || true"
	do shell script "rm -f " & quoted form of (flPluginDbPathAlt & "Generators/Demon Synth.nfo") & " 2>/dev/null || true"
	do shell script "rm -f " & quoted form of (flPluginDbPathAlt & "Generators/NulyBeats Synth.fst") & " 2>/dev/null || true"
	do shell script "rm -f " & quoted form of (flPluginDbPathAlt & "Generators/NulyBeats Synth.nfo") & " 2>/dev/null || true"
	do shell script "rm -f " & quoted form of (flPluginDbPathAlt & "Generators/Nuly Beats.fst") & " 2>/dev/null || true"
	do shell script "rm -f " & quoted form of (flPluginDbPathAlt & "Generators/Nuly Beats.nfo") & " 2>/dev/null || true"
	do shell script "rm -f " & quoted form of (flPluginDbPathAlt & "Effects/Demon Synth.fst") & " 2>/dev/null || true"
	do shell script "rm -f " & quoted form of (flPluginDbPathAlt & "Effects/Demon Synth.nfo") & " 2>/dev/null || true"

	if deleteSamples then
		-- Remove everything including samples
		do shell script "rm -rf " & quoted form of configDir & " 2>/dev/null || true"
		-- Remove parent NullyBeats folder if empty
		do shell script "rmdir " & quoted form of (homeFolder & "Library/Application Support/NullyBeats") & " 2>/dev/null || true"
	end if

	-- Success message
	if deleteSamples then
		display dialog "Uninstallation Complete!

Demon Synth has been completely removed from your system.

Thank you for trying Demon Synth!

© 2024 Nully Beats LLC" buttons {"OK"} default button "OK" with title "Uninstall Complete" with icon note
	else
		display dialog "Uninstallation Complete!

Demon Synth plugins have been removed.

Your sample presets have been kept at:
" & samplesDir & "

Thank you for trying Demon Synth!

© 2024 Nully Beats LLC" buttons {"OK"} default button "OK" with title "Uninstall Complete" with icon note
	end if

on error errMsg
	display dialog "Uninstall Error:

" & errMsg & "

Some files may not have been removed." buttons {"OK"} default button "OK" with title "Uninstall Error" with icon stop
end try
