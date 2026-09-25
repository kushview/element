.. include:: /shortcuts.rstext

.. _interface-menus-and-commands:

Menus and Commands
==================

This chapter lists every menu item with its default keyboard shortcut.
:kbd:`Cmd` means the Command key on macOS and :kbd:`Ctrl` on Windows and
Linux. Every shortcut can be changed in the :ref:`Key Mappings editor
<interface-key-mappings>`.

On macOS the application menu holds :guilabel:`About Element`,
:guilabel:`Check For Updates...` and :guilabel:`Preferences...`
(:kbd:`Cmd+,`). On Windows and Linux those items are in the :guilabel:`File`
and :guilabel:`Help` menus instead.

File
----

.. list-table::
   :header-rows: 1
   :widths: 32 28 40

   * - Item
     - Shortcut
     - Action
   * - :guilabel:`New Session`
     - :kbd:`Cmd+N`
     - Creates a new session, from the default session file if one is set.
   * - :guilabel:`Close Session`
     -
     - Closes the session, offering to save it first.
   * - :guilabel:`Open Session...`
     - :kbd:`Cmd+O`
     - Opens an ``.els`` file.
   * - :guilabel:`Open Recent`
     -
     - Recently opened sessions, and :guilabel:`Clear Recent Files`.
   * - :guilabel:`Save Session`
     - :kbd:`Cmd+S`
     - Saves the session, asking for a name the first time.
   * - :guilabel:`Save Session As...`
     - :kbd:`Shift+Cmd+S`
     - Saves the session under a new name.
   * - :guilabel:`Import...`
     -
     - Imports an ``.elg`` graph file as a new root graph.
   * - :guilabel:`Export graph...`
     -
     - Saves the active graph as an ``.elg`` file.
   * - :guilabel:`Check For Updates..`
     -
     - Windows and Linux, updater builds only.
   * - :guilabel:`Preferences..`
     - :kbd:`Cmd+,`
     - Windows and Linux.
   * - :guilabel:`Quit`
     - :kbd:`Cmd+Q`
     - Windows and Linux.

Edit
----

.. list-table::
   :header-rows: 1
   :widths: 32 28 40

   * - Item
     - Shortcut
     - Action
   * - :guilabel:`New graph`
     - :kbd:`Shift+Cmd+N`
     - Adds an empty root graph to the session.
   * - :guilabel:`Duplicate current graph`
     - :kbd:`Shift+Cmd+D`
     - Copies the active graph, nodes and connections included.
   * - :guilabel:`Delete current graph`
     - :kbd:`Cmd+Backspace`
     - Removes the active graph.
   * - :guilabel:`Undo`
     - :kbd:`Cmd+Z`
     - Undoes the last change to the session.
   * - :guilabel:`Redo`
     - :kbd:`Shift+Cmd+Z`
     - Redoes the last undone change.

View
----

.. list-table::
   :header-rows: 1
   :widths: 32 28 40

   * - Item
     - Shortcut
     - Action
   * - :guilabel:`Patch Bay`
     - :kbd:`F1`
     - Shows the patch bay in the main view.
   * - :guilabel:`Graph Editor`
     - :kbd:`F2`
     - Shows the graph editor in the main view.
   * - :guilabel:`Graph Mixer`
     -
     - Shows or hides the mixer in the accessory view.
   * - :guilabel:`Console`
     - :kbd:`F3`
     - Shows or hides the Lua console in the accessory view.
   * - :guilabel:`Rotate View...`
     - :kbd:`Cmd+Alt+R`
     - Switches between the graph editor and the patch bay.
   * - :guilabel:`Channel Strip`
     -
     - Shows or hides the channel strip column.
   * - :guilabel:`Virtual Keyboard`
     - :kbd:`Alt+K`
     - Shows or hides the virtual keyboard.
   * - :guilabel:`Meter Bridge`
     - :kbd:`Alt+M`
     - Shows or hides the meter bridge.
   * - :guilabel:`Session Properties`
     -
     - Shows the session name, tempo and notes.
   * - :guilabel:`Plugin Manager`
     -
     - Shows the plugin manager.
   * - :guilabel:`Key Mappings`
     -
     - Shows the keyboard shortcut editor.
   * - :guilabel:`MIDI Mappings`
     - :kbd:`Shift+Cmd+M`
     - Shows the session's MIDI mappings.

Options
-------

The :guilabel:`Options` menu mirrors the most used preferences so they can be
changed without opening the preferences window.

- :guilabel:`General`: :guilabel:`Check Updates at Startup`,
  :guilabel:`Scan Plugins at Startup`,
  :guilabel:`Automatically Show Plugin Windows`,
  :guilabel:`Plugins On Top By Default`,
  :guilabel:`Hide Plugin Windows When App Inactive`,
  :guilabel:`Open Last Saved Session` and :guilabel:`Ask To Save Session`.
  Each is a check item.
- :guilabel:`MIDI Input Devices`: a check item per input device.
- :guilabel:`MIDI Output Device`: the default output; choose the ticked
  device again to clear it.
- :guilabel:`Audio Input Device` and :guilabel:`Audio Output Device`. Under
  ASIO there is a single :guilabel:`Audio Device` menu.
- :guilabel:`Sample Rate` and :guilabel:`Buffer Size`.

Window
------

.. list-table::
   :header-rows: 1
   :widths: 32 28 40

   * - Item
     - Shortcut
     - Action
   * - :guilabel:`Close plugin windows...`
     - :kbd:`Cmd+Alt+W`
     - Closes every open plugin window of the active graph.
   * - :guilabel:`Show plugin windows...`
     - :kbd:`Shift+Cmd+Alt+W`
     - Opens a window for every plugin in the active graph.
   * - :guilabel:`Reset plugin windows`
     -
     - Moves plugin windows that ended up off screen back on screen.

Help
----

:guilabel:`User's manual...`, :guilabel:`Discussion forum...`,
:guilabel:`Scripting API...`, :guilabel:`Issue tracking...` and
:guilabel:`Change log...` open the corresponding web pages.
:guilabel:`Log files...` opens the folder that holds |El|'s log files (see
:ref:`appendix-troubleshooting`). On Windows and Linux the menu also has
:guilabel:`About Element`.

Commands without a menu item
----------------------------

.. list-table::
   :header-rows: 1
   :widths: 32 28 40

   * - Command
     - Shortcut
     - Action
   * - MIDI Learn
     - :kbd:`Cmd+M`
     - Toggles learn mode, like the :guilabel:`learn` toolbar button.
   * - Play
     - :kbd:`Space`
     - Starts or pauses the transport.
   * - Seek Start
     - :kbd:`/`
     - Returns the transport to the start.
   * - Rewind, Forward
     - :kbd:`J`, :kbd:`L`
     - Move the transport backwards or forwards.
   * - Panic!
     - :kbd:`Cmd+Alt+P`
     - Sends all-notes-off to every node.
   * - Select all
     - :kbd:`Cmd+A`
     - Selects every block in the graph editor.
   * - Delete selection
     - :kbd:`Delete`, :kbd:`Backspace`
     - Removes the selected blocks in the graph editor.

A root graph can also be given its own :guilabel:`Hotkey` in its settings
(:ref:`using-graphs`); pressing it makes that graph active.

.. _interface-key-mappings:

The Key Mappings editor
-----------------------

:menuselection:`View --> Key Mappings` shows every command grouped by
category with its current key. Click the key next to a command to press a new
one, or add a second key with the :guilabel:`+` button. The
:guilabel:`reset to defaults` button restores the shortcuts listed above, and
:guilabel:`Close` returns to the previous view. Custom mappings are stored in
the settings file and apply to the standalone application; the plugin editions
use the default shortcuts.

.. SCREENSHOT interface/menus-and-commands-01: the Key Mappings view with a
   category expanded
.. .. figure:: /images/interface/menus-and-commands-01.png
..    :alt: The Key Mappings editor
..
..    The Key Mappings editor.
