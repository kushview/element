.. include:: /shortcuts.rstext

.. _interface-graph-editor:

The Graph Editor
================

The graph editor is the default main view. It shows the nodes of one graph as
blocks and the connections between them as cables. :menuselection:`View -->
Graph Editor` (:kbd:`F2`) brings it back when another view is showing.

.. SCREENSHOT interface/graph-editor-01: the graph editor with five or six
   blocks, one in Embed mode, cables between them, breadcrumb visible
.. .. figure:: /images/interface/graph-editor-01.png
..    :alt: The graph editor
..
..    The graph editor.

The breadcrumb bar at the top shows the path from the root graph to the graph
being edited. Click an entry to go back up when you are inside a subgraph.

Blocks
------

Each node is a rounded block showing its name; if the node has been renamed,
the plugin's name appears underneath in smaller text. A coloured bar can be
shown along the top (see :guilabel:`Color` below). The selected block has an
accent outline, a disabled node is dimmed, and a node whose plugin is missing
is drawn in bold red and behaves as a placeholder: double-click it to read
why the plugin could not be loaded.

The bottom row of a block holds up to three buttons:

- a cog that opens the bus configuration window for plugins that support it
  (:guilabel:`Input Configuration` / :guilabel:`Output Configuration`, with a
  :guilabel:`Bus Name`, a :guilabel:`Channel Layout` and buttons to add and
  remove buses);
- :guilabel:`M`, which mutes the node's output;
- a power button, which bypasses the node.

The graph's input and output nodes, subgraphs and MIDI device nodes have no
mute or bypass button. A MIDI Monitor block adds a MIDI activity indicator.

Ports and cables
----------------

Ports are the small squares along the edges of a block: green for audio,
orange for MIDI and light blue for control. Hover a port to see its name.

- Drag from a port to a compatible port on another block to connect them.
- Drag the end of an existing cable away from its port to move the
  connection elsewhere, or drop it on empty space to remove it.
- A cable brightens while the pointer is over it.

Selecting and moving
--------------------

- Click a block to select it. :kbd:`Shift`-click or :kbd:`Cmd`-click adds to
  the selection, and dragging on empty space draws a lasso around several
  blocks. :kbd:`Cmd+A` selects everything.
- Drag a selected block to move the whole selection.
- Double-click a block to open its plugin window, or, for a subgraph, to
  enter it.
- :kbd:`Delete` or :kbd:`Backspace` removes the selected blocks.

Adding nodes
------------

- Right-click empty space and choose from the :guilabel:`Plugins` section of
  the menu (below).
- Drag a plugin from the :guilabel:`Plugins` panel in the sidebar. Hold
  :kbd:`Alt` while dropping to connect its inputs to the graph's inputs, or
  :kbd:`Cmd` to connect its outputs to the graph's outputs.
- Drop ``.elg``, ``.eln`` or ``.elpreset`` files, or plugin files, from your
  file manager.
- Drop a plugin onto a placeholder block to replace the missing plugin with
  it.

Orientation and display
-----------------------

Each graph is laid out either vertically, with signal flowing top to bottom
(the default), or horizontally, left to right. :guilabel:`Change
orientation...` in the background menu switches; the choice is saved with the
graph.

Each block has a display mode, set from its :guilabel:`Display` submenu:

:guilabel:`Compact`, :guilabel:`Small`, :guilabel:`Normal`
   Progressively larger blocks showing the name, ports and buttons.

:guilabel:`Embed`
   Shows the plugin's own editor inside the block. Blocks of |El|'s built-in
   nodes can be resized from their bottom-right corner in this mode. While
   the node's plugin window is open the block temporarily drops to
   :guilabel:`Small` and returns to :guilabel:`Embed` when it is closed.

The same submenu sets the :guilabel:`Port Alignment`: ports sit at the
:guilabel:`Left`, :guilabel:`Middle` or :guilabel:`Right` of a vertical block,
or :guilabel:`Top`, :guilabel:`Middle` or :guilabel:`Bottom` of a horizontal
one. Display mode and alignment apply to every selected block.

The background menu
-------------------

Right-click empty space in the editor:

:guilabel:`Graph I/O`
   :guilabel:`Audio Inputs`, :guilabel:`Audio Outputs`, :guilabel:`MIDI Input`
   and :guilabel:`MIDI Output` are check items that add or remove the
   graph's input and output nodes.

:guilabel:`MIDI Input Device`, :guilabel:`MIDI Output Device`
   Add a node bound to one specific MIDI port (:ref:`nodes-midi`).

:guilabel:`Change orientation...`
   Switches between vertical and horizontal layout.

:guilabel:`Gather nodes...`
   Moves blocks that have drifted off screen back into view.

:guilabel:`Plugins`
   :guilabel:`Favorites` lists plugins you have starred in the plugin
   manager; the manufacturers that follow list every scanned plugin, with
   |El|'s own nodes under :guilabel:`Element`; :guilabel:`Unverified` lists,
   by format, plugins found on disk that have not been scanned yet.

.. SCREENSHOT interface/graph-editor-02: the background context menu open,
   showing Graph I/O, device submenus and the Plugins section
.. .. figure:: /images/interface/graph-editor-02.png
..    :alt: The graph editor background menu
..
..    The background menu.

The block menu
--------------

Right-click a block:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Item
     - Action
   * - :guilabel:`Disable` / :guilabel:`Enable`
     - Turns the node off or on. A disabled node passes nothing.
   * - :guilabel:`Rename`
     - Opens the :guilabel:`Rename Node` dialog.
   * - :guilabel:`Disconnect`
     - :guilabel:`All Ports`, :guilabel:`MIDI Ports`, :guilabel:`Input
       Ports` or :guilabel:`Output Ports` removes those connections.
   * - :guilabel:`Duplicate`
     - Adds a copy of the node. Not available for input and output nodes.
   * - :guilabel:`Remove`
     - Removes the node and every other selected block.
   * - :guilabel:`Replace`
     - Swaps the plugin for another one from the list, keeping the block's
       connections where the ports match. The current plugin is ticked.
   * - :guilabel:`Edit DSP Script`, :guilabel:`Edit UI Script`
     - Script nodes only; open the script editor (:ref:`scripting-script-node`).
   * - :guilabel:`Ports...`
     - A table of the node's ports with a visibility checkbox, name and type
       for each, plus :guilabel:`Show all` and :guilabel:`Hide all`. Hidden
       ports keep their connections but are not drawn.
   * - :guilabel:`Color`
     - A colour picker for the block's bar. Applies to every selected block.
   * - :guilabel:`Display`
     - Display mode and port alignment, described above.
   * - :guilabel:`Options`
     - :guilabel:`Mute input ports` silences what the node receives without
       muting its output. :guilabel:`Oversample` runs the node at
       :guilabel:`Off`, :guilabel:`2x`, :guilabel:`4x` or :guilabel:`8x` the
       session sample rate.
   * - :guilabel:`Presets`
     - Saving and loading the node's state; see
       :ref:`using-nodes-and-plugins`.

.. SCREENSHOT interface/graph-editor-03: the block context menu open on a
   plugin node with the Presets submenu expanded
.. .. figure:: /images/interface/graph-editor-03.png
..    :alt: The block context menu
..
..    The block menu.
