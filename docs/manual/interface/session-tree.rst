.. include:: /shortcuts.rstext

.. _interface-session-tree:

Session Tree and Data Path
==========================

Two sidebar panels deal with files and structure: the :guilabel:`Session`
panel shows what is in the open session, and the :guilabel:`Data Path` panel
shows what is in your |El| library on disk.

The Session panel
-----------------

The tree lists every root graph, and under each graph its nodes. Subgraphs
expand to show their own nodes, and Script nodes show :guilabel:`DSP` and
:guilabel:`UI` children that open the script editor. The active graph is
highlighted in green, and a graph that has a MIDI program assigned shows the
program number at the right. The expanded and collapsed state of the tree is
saved with the session.

.. SCREENSHOT interface/session-tree-01: the Session panel with two graphs,
   the active one highlighted, one expanded with nodes including a subgraph
   and a script node showing DSP and UI children
.. .. figure:: /images/interface/session-tree-01.png
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

The Data Path panel
-------------------

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
