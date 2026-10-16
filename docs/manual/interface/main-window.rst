.. include:: /shortcuts.rstext

.. _interface-main-window:

The Main Window
===============

|El| uses a single main window. From top to bottom it holds the toolbar, the
main area, and the status bar. The main area is split by a draggable divider
into the sidebar on the left and the content area on the right.

.. SCREENSHOT interface/main-window-01: the whole main window with a small
   session loaded, sidebar showing Session and Node panels, graph editor in
   the main view, Graph Mixer as the accessory view, virtual keyboard shown
.. .. figure:: /images/interface/main-window-01.png
..    :alt: The main window with every area visible
..
..    The main window: toolbar (1), sidebar (2), main view (3), accessory
..    view (4), bottom strip (5), status bar (6).

The window title reads ``Element - <session>: <graph>`` for the open session
and the graph shown in the editor.

The toolbar
-----------

From left to right:

:guilabel:`EXT`
   Follow the external MIDI clock. The button only appears when
   :guilabel:`Clock Source` is set to :guilabel:`MIDI Clock` in the
   preferences, or when |El| runs as a plugin, where it follows the host.
   While it is on, the tempo box and :guilabel:`TAP` are locked.

Tempo
   The session tempo in beats per minute, from 20 to 999. Drag up or down to
   change it, or double-click to type a value.

:guilabel:`TAP`
   Tap tempo. Click it in time with the music to set the tempo. Right-click
   it for :guilabel:`MIDI Learn Tap Tempo`, and afterwards for
   :guilabel:`Re-learn MIDI Mapping` and :guilabel:`Clear MIDI Mapping`.

Time signature
   Drag the left number to change the beats per bar (1 to 99) and the right
   number to change the note value.

Transport
   The position readout shows bar, beat and sub-beat; double-click the bar
   number to return to the start. The buttons are Play, Stop, Record and Seek
   to start. Right-click any of them to learn a MIDI mapping for it.
   :ref:`using-transport-and-clock` covers the transport in detail.

:guilabel:`learn`
   Turns MIDI learn mode on and off. The tooltip shows the current keyboard
   shortcut, :kbd:`Cmd+M` by default. See :ref:`using-midi-mapping`.

:guilabel:`view`
   Switches the main view between the graph editor and the patch bay.

MIDI indicator
   Blinks when MIDI arrives at or leaves the engine.

The sidebar
-----------

The sidebar is a stack of collapsible panels. Click a panel's header to open
or close it, and drag the divider between the sidebar and the content area to
resize it. Each panel is described in :ref:`interface-sidebar-panels`:

- :guilabel:`Session`: the graphs and nodes of the open session
  (:ref:`interface-session-panel`).
- :guilabel:`Graph`: the settings of the graph in the editor
  (:ref:`interface-graph-panel`).
- :guilabel:`Node`: properties of the selected node
  (:ref:`interface-node-panel`).
- :guilabel:`Editor`: the selected node's editor shown inline
  (:ref:`interface-editor-panel`).
- :guilabel:`Plugins`: a searchable plugin browser
  (:ref:`interface-plugins-panel`).
- :guilabel:`Data Path`: a file browser for your |El| library
  (:ref:`interface-data-path-panel`).

The content area
----------------

The content area has three parts, top to bottom:

The main view
   One of :guilabel:`Graph Editor` (the default), :guilabel:`Patch Bay`,
   :guilabel:`Plugin Manager`, :guilabel:`Session Properties`,
   :guilabel:`Graph Settings`, :guilabel:`Key Mappings` or
   :guilabel:`MIDI Mappings`, chosen from the :guilabel:`View` menu. In the
   settings, plugin manager and mapping views, :kbd:`Esc` returns to the
   previous view.

The accessory view
   An optional pane under the main view, separated by a draggable divider:
   the :guilabel:`Graph Mixer` (:ref:`interface-mixer-strip-meters`) or the
   :guilabel:`Console` (:ref:`scripting-console`).

The bottom strip
   The :guilabel:`Virtual Keyboard` (:ref:`using-virtual-keyboard`) and the
   :guilabel:`Meter Bridge` (:ref:`interface-mixer-strip-meters`), each
   toggled from the :guilabel:`View` menu.

The :guilabel:`Channel Strip` is a further optional column at the right edge
of the window.

The status bar
--------------

The status bar has three fields.

Device
   The current audio device. It reads :guilabel:`No Device` when none is
   open and :guilabel:`Disconnected:` followed by the device name when a
   device has been unplugged. |El| keeps the device closed and reopens it
   automatically when it reappears. Double-click this field to open the
   :guilabel:`Audio` preferences.

Engine
   :guilabel:`Engine: Running` and the CPU load of the audio thread. While a
   plugin scan is in progress the field shows :guilabel:`Scanning:` and the
   plugin being scanned.

Sample rate and buffer
   The sample rate in kHz and the buffer size in samples.

In the plugin editions the first field reads :guilabel:`Host` and the last
shows the latency reported to the host.

Dropping files on the window
----------------------------

Files dropped anywhere on the main window are opened according to type:

- ``.els`` opens the session.
- ``.elg`` imports the graph into the session.
- ``.eln`` and ``.elpreset`` add the saved node to the graph in the editor.
- Plugin files (``.vst3``, ``.vst``, ``.dll``) add that plugin. Hold
  :kbd:`Alt` while dropping to connect it to the graph's inputs, or
  :kbd:`Cmd` to connect it to the outputs.

The system tray
---------------

With :guilabel:`Show system tray` enabled in the preferences (on by default,
except on Linux), |El| adds an icon to the system tray or menu bar with
:guilabel:`Show/Hide` and :guilabel:`Exit` items. Minimising the window
hides it to the tray; on Windows and Linux a left click on the icon shows or
hides the window. :guilabel:`Start hidden in system tray` and the
``--hidden`` command-line flag start |El| in the tray. Closing the main window
always quits the application.

What is remembered
------------------

|El| stores in its settings the window position and full-screen state, the
sidebar width and each panel's height, which accessory view is open and its
height, and whether the virtual keyboard, channel strip and meter bridge are
shown. The session file stores its own panel state, each graph's editor
orientation, and each block's display mode, port alignment, size and colour.
