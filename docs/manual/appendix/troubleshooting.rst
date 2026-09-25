.. include:: /shortcuts.rstext

.. _appendix-troubleshooting:

Troubleshooting
===============

Log files
---------

:menuselection:`Help --> Log files...` opens the ``log`` folder of the
application data directory (:ref:`appendix-files-and-locations`).
``main.log`` records the application's messages, and if |El| exits because
of an unhandled error on any thread, the error and a backtrace are written
there before it closes. ``scanner.log`` records the plugin scanner. Attach
both to a report on the `issue tracker`_.

|El| does not start, or crashes while opening a session
-------------------------------------------------------

If the previous launch died while opening the last session, the next one
asks :guilabel:`Open Anyway` or :guilabel:`Start Empty`. Choose
:guilabel:`Start Empty`, then open the session's plugins one graph at a time
to find the one at fault, or turn off :guilabel:`Open last used session` in
the preferences. A plugin that crashes reliably can be removed from the
plugin list in the plugin manager and the node replaced.

Unsaved work after a crash
--------------------------

The standalone application autosaves every two minutes. If a recovery file is
newer than the saved session, the next launch offers :guilabel:`Recover`.
See :ref:`using-sessions`.

No sound
--------

- Check the status bar. :guilabel:`No Device` means no audio device is
  open; double-click the field to open the :guilabel:`Audio` preferences and
  choose one. :guilabel:`Disconnected:` means the device was unplugged;
  |El| reopens it when it returns.
- Make sure the graph you are editing is the active graph (green in the
  session tree). In :guilabel:`Single` rendering mode only the active graph
  produces sound.
- Check that the instrument is connected to :guilabel:`Audio Out` and that
  neither it nor the graph is bypassed or muted. The :guilabel:`Graph
  Mixer` and :guilabel:`Meter Bridge` show where signal stops.
- The MIDI indicator in the toolbar blinks when MIDI arrives. If it does
  not, enable the device under :guilabel:`Active MIDI Inputs`, and check the
  node's :guilabel:`MIDI Channel` and key range.

A plugin is missing or shows in red
-----------------------------------

A red block is a placeholder for a plugin that could not be loaded.
Double-click it for the reason. Usually the plugin is not installed on this
computer or is in a folder that is not scanned. Install it and reopen the
session, add the folder in :guilabel:`Search Paths` and rescan, or use
:guilabel:`Replace`.

A plugin does not appear after scanning
---------------------------------------

- It may have crashed the scanner and been blacklisted. Use
  :guilabel:`Clear blacklisted plug-ins` in the plugin manager's
  :guilabel:`Options...` menu and scan again; ``scanner.log`` shows what
  happened.
- Its format may not be enabled in this build, or its folder may not be in
  the search paths.
- Try adding it from the :guilabel:`Unverified` submenu, which loads it
  without a scan.

The plugin scanner could not be started
---------------------------------------

The scanner is |El| itself launched as a helper. If it cannot start, the
scan stops early with a message and no plugin is blamed. Check that the
application has not been moved or partially deleted, and that no security
software is blocking it.

Stuck notes
-----------

:guilabel:`Panic!` (:kbd:`Cmd+Alt+P`) sends all-notes-off to every node. A
CC can be assigned to do the same with :guilabel:`MIDI Panic CC` in the
:guilabel:`MIDI` preferences.

The OSC host will not start
---------------------------

:guilabel:`Could not start OSC host on port` means another program is
using the port. Change :guilabel:`OSC Host Port` in the preferences.

Plugin windows are off screen
-----------------------------

:menuselection:`Window --> Reset plugin windows` moves every plugin window
back onto the screen.

Getting help
------------

The `Discord server`_ is where users and developers talk; bugs and feature
requests go to the `issue tracker`_ with the version from
:guilabel:`About Element` (it has a :guilabel:`Copy` button) and the log
files.
