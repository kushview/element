-- SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
-- SPDX-License-Identifier: GPL-3.0-or-later

-- Display confirmation dialog
set dialogResult to display dialog "This will uninstall Element and all its plugins from your system." & return & return & "Continue?" buttons {"Cancel", "Uninstall"} default button "Cancel" with icon caution

if button returned of dialogResult is "Uninstall" then
	try
		-- Run uninstall commands with admin privileges
		do shell script "
PLUGIN_NAME='KV-Element'
PATTERN=\"${PLUGIN_NAME}*.*\"

# Remove plugins from user directory
find \"$HOME/Library/Audio/Plug-Ins\" -maxdepth 3 -type d -name \"$PATTERN\" -print -exec rm -rf {} \\; 2>/dev/null || true

# Remove plugins from system directory (requires admin)
find '/Library/Audio/Plug-Ins' -maxdepth 3 -type d -name \"$PATTERN\" -print -exec rm -rf {} \\; 2>/dev/null || true

# Remove applications
rm -rf '/Applications/Element.app'
rm -rf \"$HOME/Applications/Element.app\"
" with administrator privileges
		
		display dialog "Element has been successfully uninstalled." buttons {"OK"} default button "OK" with icon note
		
	on error errMsg
		display dialog "An error occurred during uninstallation:" & return & return & errMsg buttons {"OK"} default button "OK" with icon stop
	end try
end if
