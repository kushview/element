.. include:: /shortcuts.rstext

.. _interface-sidebar-panels:

Sidebar Panels
==============

The sidebar on the left of the main window is a stack of collapsible panels.
Click a panel's header to open or close it, and drag the divider between the
sidebar and the content area to resize it. The panels are described here in
the order they appear.

.. _interface-session-panel:
.. _interface-session-tree:

Session panel
-------------

The tree lists every root graph, and under each graph its nodes. Subgraphs
expand to show their own nodes, and Script nodes show :guilabel:`DSP` and
:guilabel:`UI` children that open the script editor. The active graph is
highlighted in green, and a graph that has a MIDI program assigned shows the
program number at the right. The expanded and collapsed state of the tree is
saved with the session.

.. SCREENSHOT interface/session-panel-01: the Session panel with two graphs,
   the active one highlighted, one expanded with nodes including a subgraph
   and a script node showing DSP and UI children
.. .. figure:: /images/interface/session-panel-01.png
..    :alt: The Session panel
..
..    The Session panel.

Graphs
   Click a graph to show it in the editor; double-click it to make it the
   active graph. Drag graphs to reorder them. Right-click a graph for
   :guilabel:`Add graph...`, :guilabel:`Edit Graph...` (show it in the
   editor), :guilabel:`View Settings...`, :guilabel:`Duplicate`,
   :guilabel:`Move Up`, :guilabel:`Move Down` and :guilabel:`Delete`.

Nodes
   Click a node to select it, which updates the :guilabel:`Node` and
   :guilabel:`Editor` panels and the channel strip. Double-click the icon to
   open or close the node's plugin window; double-click the name to rename
   it. :kbd:`Alt+Right` opens the plugin window of the selected node.
   Right-click a node for :guilabel:`Duplicate` and :guilabel:`Delete`, plus
   :guilabel:`Add graph...` when the node is a subgraph. With several items
   selected the menu offers :guilabel:`Delete Selected`.

The panel header has a menu button with :guilabel:`Session Settings...` and
:guilabel:`Add Graph`.

See :ref:`using-sessions` and :ref:`using-graphs` for what sessions and
graphs are and how to work with them.

.. _interface-graph-panel:

Graph panel
-----------

The :guilabel:`Graph` panel shows the settings of the graph in the editor,
and follows the editor when you switch graphs. :guilabel:`View Settings...`
on a graph's right-click menu in the :guilabel:`Session` panel shows the same
settings in the main view.

.. SCREENSHOT interface/graph-panel-01: the Graph panel in the sidebar with
   every property visible, MIDI Program set and a hotkey mapped
.. .. figure:: /images/interface/graph-panel-01.png
..    :alt: The Graph panel
..
..    The Graph panel.

:guilabel:`Name`
   The graph's name, shown in the session tree and the window title.

:guilabel:`Rendering Mode`
   :guilabel:`Single`: only the active graph renders. A graph that has just
   been deactivated keeps rendering until its output falls silent, so
   reverb tails and held notes finish, and is then bypassed.
   :guilabel:`Parallel`: every root graph renders all the time and all of
   them receive MIDI. Use it for layered rigs where several graphs play at
   once.

:guilabel:`Velocity Curve`
   A curve applied to the velocity of incoming notes before they reach the
   nodes, to suit your keyboard or playing style:

   =========== ===========================================
   Mode        Description
   =========== ===========================================
   Linear      No change
   Soft        Curves velocity down, for heavy hands
   Softer      A stronger version of Soft
   Softest     The strongest downward curve
   Hard        Curves velocity up, for light hands
   Harder      A stronger version of Hard
   Hardest     The strongest upward curve
   Max         Every note plays at maximum velocity (127)
   =========== ===========================================

:guilabel:`MIDI Channel`
   The channels the graph accepts, or :guilabel:`Omni` for all of them.
   Several channels can be ticked. Messages on other channels never enter the
   graph.

:guilabel:`MIDI Program`
   :guilabel:`None`, or a program number from 1 to 128 that makes this graph
   the active graph when a program change with that number arrives on one of
   the graph's channels. The session tree shows the number next to the
   graph.

:guilabel:`Hotkey`
   A computer key that activates the graph. Click :guilabel:`Map`, press the
   key, and it is stored; :guilabel:`Clear` removes it.

:guilabel:`Audio Ins`, :guilabel:`Audio Outs`, :guilabel:`MIDI Ins`, :guilabel:`MIDI Outs`
   The number of ports on the graph's input and output nodes. A root graph
   defaults to two audio channels and one MIDI port each way.

Subgraphs have no MIDI program or hotkey of their own. See
:ref:`using-graphs` for switching between graphs and for subgraphs.

.. _interface-node-panel:

Node panel
----------

The :guilabel:`Node` panel shows the properties of the selected node. It
follows the selection in the graph editor and the :guilabel:`Session` panel.

The header has a drop-down to pick any node of the current graph directly
(input and output nodes are not listed), and a menu button with a
:guilabel:`Sticky` option. A sticky panel stays on its node instead of
following the selection, which is useful for keeping one node's settings in
view while editing others. For a Script node the menu also has
:guilabel:`Edit DSP Script` and :guilabel:`Edit UI Script`.

.. SCREENSHOT interface/node-panel-01: the Node panel for an instrument
   plugin with the MIDI section and two saved MIDI programs
.. .. figure:: /images/interface/node-panel-01.png
..    :alt: The Node panel
..
..    The Node panel.

Node section
^^^^^^^^^^^^

The section is titled with the node's plugin name.

:guilabel:`Name`
   The name shown on the block and in the session tree. Renaming does not
   change the plugin.

:guilabel:`Delay comp.`
   Extra delay applied to the node's output, from -1000 to 1000 ms, on top
   of the latency the plugin reports. Use it to line up parallel paths that
   the automatic compensation cannot measure. Hidden for nodes that carry no
   audio.

MIDI section
^^^^^^^^^^^^

:guilabel:`MIDI Channel`
   The channels the node accepts. Messages on other channels are dropped
   before they reach the node.

:guilabel:`Key Start`, :guilabel:`Key End`
   The note range the node accepts, entered as note numbers or names such as
   ``C2``. Notes outside the range are dropped. Two instruments with
   different ranges make a keyboard split; two with the same range make a
   layer.

:guilabel:`Transpose`
   Shifts incoming notes by up to two octaves in either direction.

Changes in this section take effect immediately.

Programs section
^^^^^^^^^^^^^^^^

A node can store up to 128 snapshots of its state and recall them with MIDI
program change messages.

The :guilabel:`MIDI Program` row has three buttons: a power button that
enables program changes for the node, a globe that uses global programs
(shared by every instance of this plugin in any session, instead of programs
stored in this session), and :guilabel:`+`, which adds a program from the
node's current state.

The :guilabel:`Saved Programs` table lists each program with its number and
name; click either to edit it. The save button on a row overwrites that
program with the node's current state, and the trash button deletes it. The
table reads :guilabel:`No saved programs` until one is added. When a program
change arrives, the matching row is selected.

See :ref:`using-nodes-and-plugins` for how programs relate to presets.

.. _interface-editor-panel:

Editor panel
------------

The :guilabel:`Editor` panel shows the selected node's own editor inline, so
a plugin can be adjusted without opening a window. Plugins without an editor
of their own get a generic editor with a slider per parameter. Like the
:guilabel:`Node` panel, it follows the selection in the graph editor and the
:guilabel:`Session` panel, has a drop-down to pick a node directly, and has a
:guilabel:`Sticky` option on its menu button that keeps one plugin's editor
in view while you work on others.

.. SCREENSHOT interface/editor-panel-01: the Editor panel showing a plugin
   editor inline, node drop-down visible in the header
.. .. figure:: /images/interface/editor-panel-01.png
..    :alt: The Editor panel
..
..    The Editor panel.

To open the editor in its own window instead, see
:ref:`interface-plugin-windows`.

.. _interface-plugins-panel:

Plugins panel
-------------

The :guilabel:`Plugins` panel is a browser of every scanned plugin, grouped
by category. Each entry shows the plugin name and, at the right, its format
(``vst``, ``vst3`` or ``au``). Type in :guilabel:`Search...` to filter the
list by name; the matching folders open as you type.

.. SCREENSHOT interface/plugins-panel-01: the Plugins panel with a search
   term entered and a few matching plugins listed under their categories
.. .. figure:: /images/interface/plugins-panel-01.png
..    :alt: The Plugins panel
..
..    The Plugins panel.

Drag a plugin onto the graph editor to add it to the graph. Hold :kbd:`Alt`
while dropping to connect its inputs to the graph's inputs, or :kbd:`Cmd` to
connect its outputs to the graph's outputs. Dropping a plugin onto a
placeholder block replaces the missing plugin with it.

The list reflects the plugins found by the :guilabel:`Plugin Manager`
(:ref:`using-plugin-manager`); plugins hidden there do not appear.

.. _interface-data-path-panel:

Data Path panel
---------------

The :guilabel:`Data Path` panel is a file browser rooted at your |El| library
(the ``Element`` folder in your Music folder, see
:ref:`appendix-files-and-locations`). It has folders for sessions, graphs,
node presets, scripts and controllers.

- Double-click an ``.els`` file to open that session, or an ``.elg`` file to
  import the graph.
- Double-click an ``.eln`` or ``.elpreset`` file to add that node to the
  active graph, or drag it onto the graph editor.
- Right-click a file for :guilabel:`Delete`.
- The header's :guilabel:`+` button offers :guilabel:`Refresh...` and
  :guilabel:`Show in Finder` (:guilabel:`Show in Explorer` on Windows and
  Linux).
