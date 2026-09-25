.. include:: /shortcuts.rstext

.. _appendix-shortcuts:

Keyboard Shortcuts
==================

Default shortcuts of the standalone application. :kbd:`Cmd` is the Command
key on macOS and :kbd:`Ctrl` on Windows and Linux; :kbd:`Alt` is Option on
macOS. All of them can be changed in :menuselection:`View --> Key Mappings`
(:ref:`interface-key-mappings`).

Session
-------

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Shortcut
     - Command
   * - :kbd:`Cmd+N`
     - New Session
   * - :kbd:`Cmd+O`
     - Open Session...
   * - :kbd:`Cmd+S`
     - Save Session
   * - :kbd:`Shift+Cmd+S`
     - Save Session As...
   * - :kbd:`Shift+Cmd+N`
     - New graph
   * - :kbd:`Shift+Cmd+D`
     - Duplicate current graph
   * - :kbd:`Cmd+Backspace`
     - Delete current graph
   * - :kbd:`Cmd+Z`
     - Undo
   * - :kbd:`Shift+Cmd+Z`
     - Redo
   * - :kbd:`Cmd+,`
     - Preferences
   * - :kbd:`Cmd+Q`
     - Quit

Views
-----

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Shortcut
     - Command
   * - :kbd:`F1`
     - Patch Bay
   * - :kbd:`F2`
     - Graph Editor
   * - :kbd:`F3`
     - Console
   * - :kbd:`Cmd+Alt+R`
     - Rotate View (graph editor / patch bay)
   * - :kbd:`Alt+K`
     - Virtual Keyboard
   * - :kbd:`Alt+M`
     - Meter Bridge
   * - :kbd:`Shift+Cmd+M`
     - MIDI Mappings
   * - :kbd:`Esc`
     - Back to the previous view (settings, plugin manager, mappings)
   * - :kbd:`Cmd+Alt+W`
     - Close plugin windows
   * - :kbd:`Shift+Cmd+Alt+W`
     - Show plugin windows

Engine and transport
--------------------

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Shortcut
     - Command
   * - :kbd:`Space`
     - Play / pause
   * - :kbd:`/`
     - Seek Start
   * - :kbd:`J`, :kbd:`L`
     - Rewind, Forward
   * - :kbd:`Cmd+M`
     - MIDI Learn
   * - :kbd:`Cmd+Alt+P`
     - Panic!
   * - graph hotkey
     - Activate the graph it is assigned to (:ref:`using-graphs`)

Graph editor
------------

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Shortcut
     - Action
   * - :kbd:`Cmd+A`
     - Select all blocks
   * - :kbd:`Delete`, :kbd:`Backspace`
     - Remove the selected blocks
   * - :kbd:`Shift`-click, :kbd:`Cmd`-click
     - Add to the selection
   * - :kbd:`Alt` while dropping a plugin
     - Connect it to the graph's inputs
   * - :kbd:`Cmd` while dropping a plugin
     - Connect it to the graph's outputs

Session tree
------------

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Shortcut
     - Action
   * - :kbd:`Alt+Right`
     - Open the selected node's plugin window
   * - :kbd:`Cmd+A`
     - Select the first item

Virtual keyboard
----------------

With :kbd:`Caps Lock` on, the letter keys play notes. While the keyboard has
focus:

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Shortcut
     - Action
   * - :kbd:`Left`, :kbd:`Right`
     - MIDI channel down, up
   * - :kbd:`Up`, :kbd:`Down`
     - Program up, down (:kbd:`Alt` for steps of ten)
   * - :kbd:`0` to :kbd:`9`
     - Set the octave
   * - keypad :kbd:`-`, :kbd:`+`
     - Octave down, up
   * - :kbd:`-`, :kbd:`+`
     - Narrower, wider keys
   * - :kbd:`Cmd+Space`
     - Sustain
   * - :kbd:`Cmd+Alt+Space`
     - Hold

Console
-------

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Shortcut
     - Action
   * - :kbd:`Up`, :kbd:`Down`
     - Previous, next command
   * - :kbd:`Ctrl+A`, :kbd:`Ctrl+E`
     - Start, end of line
