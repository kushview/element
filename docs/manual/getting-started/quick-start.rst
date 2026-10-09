.. include:: /shortcuts.rstext

.. _getting-started-quick-start:

Quick Start
===========

This chapter takes an empty session to a playable instrument in a few
minutes. It assumes |El| is installed (:ref:`getting-started-installation`) and
that you have at least one instrument plugin on your computer.

1. Launch and set up audio
--------------------------

Start |El|. On the first launch it opens a new session containing one empty
graph, and the graph editor fills the main area.

.. SCREENSHOT getting-started/quick-start-01: the main window right after
   the first launch, empty graph in the graph editor, sidebar visible
.. .. figure:: /images/getting-started/quick-start-01.png
..    :alt: The main window after the first launch
..
..    The main window after the first launch.

Open the preferences with :menuselection:`File --> Preferences...`
(:menuselection:`Element --> Preferences...` on macOS, or :kbd:`Cmd+,`).

- On the :guilabel:`Audio` page choose your audio device type, the output
  device, and a buffer size. The status bar at the bottom of the window shows
  the device, sample rate and buffer size once the engine is running.
- On the :guilabel:`MIDI` page turn on the MIDI keyboard or controller you
  want to play with under :guilabel:`Active MIDI Inputs`. If you have no
  hardware, skip this step and use the virtual keyboard below.

Close the preferences.

2. Scan for plugins
-------------------

Open :menuselection:`View --> Plugin Manager` and click :guilabel:`Scan`. |El|
scans the default folders for every plugin format it supports and lists what
it finds. The scan runs in a separate process, so a plugin that crashes while
scanning cannot take |El| down with it. Press :kbd:`Esc` to return to the
graph editor when the scan is complete.

.. tip::

   You can skip scanning: the :guilabel:`Unverified` submenu of the plugin
   list offers plugins found on disk that have not been scanned yet.

3. Add an instrument
--------------------

Right-click an empty part of the graph editor. The menu ends with a
:guilabel:`Plugins` section that lists every scanned plugin grouped by
manufacturer, with |El|'s own nodes under :guilabel:`Element`. Choose an
instrument. A block appears in the editor.

Alternatively, open the :guilabel:`Plugins` panel in the sidebar, type part of
the plugin's name in :guilabel:`Search...`, and drag the plugin onto the graph.
Hold :kbd:`Alt` while dropping it to connect its inputs to the graph's inputs,
or :kbd:`Cmd` to connect its outputs to the graph's outputs.

4. Connect it
-------------

Every new graph starts with four input/output nodes: :guilabel:`Audio In`,
:guilabel:`Audio Out`, :guilabel:`MIDI In` and :guilabel:`MIDI Out`. Ports are
the small squares on a block: green for audio, orange for MIDI.

- Drag from the :guilabel:`MIDI In` node's port to the instrument's MIDI
  input port.
- Drag from each of the instrument's audio output ports to the matching
  port on :guilabel:`Audio Out`.

.. SCREENSHOT getting-started/quick-start-02: an instrument block connected
   from MIDI In and to Audio Out, vertical orientation, Normal display mode
.. .. figure:: /images/getting-started/quick-start-02.png
..    :alt: An instrument connected to the graph's MIDI input and audio output
..
..    An instrument connected to the graph's MIDI input and audio output.

5. Play
-------

Play your MIDI keyboard. If you have none, show the virtual keyboard with
:menuselection:`View --> Virtual Keyboard` (:kbd:`Alt+K`), turn on
:kbd:`Caps Lock`, and play the computer keyboard. The MIDI indicator at the
right end of the toolbar blinks as messages arrive.

Double-click the instrument's block to open its own editor window.

6. Save the session
-------------------

Choose :menuselection:`File --> Save Session` (:kbd:`Cmd+S`) and pick a name.
Sessions are ``.els`` files; the default location is the ``Sessions`` folder
of your |El| library in your Music folder. The next launch reopens the last
session automatically.

Where to go next
----------------

- :ref:`getting-started-concepts` explains sessions, graphs and nodes.
- :ref:`interface-graph-editor` covers everything the graph editor can do.
- :ref:`using-midi-mapping` shows how to control any parameter from a MIDI
  controller.
