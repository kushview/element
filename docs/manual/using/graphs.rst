.. include:: /shortcuts.rstext

.. _using-graphs:

Graphs
======

A graph is a set of nodes and connections. The graphs listed under the
session are root graphs; a graph inside another graph is a subgraph. This
chapter covers the settings of a root graph, switching between graphs, and
subgraphs. Editing the contents of a graph is covered in
:ref:`interface-graph-editor`.

Adding, duplicating and removing
--------------------------------

- :menuselection:`Edit --> New graph` (:kbd:`Shift+Cmd+N`), or
  :guilabel:`Add Graph` from the :guilabel:`Session` panel menu, adds an
  empty root graph with the four input and output nodes.
- :menuselection:`Edit --> Duplicate current graph` (:kbd:`Shift+Cmd+D`)
  copies the active graph including its nodes, connections and settings.
- :menuselection:`Edit --> Delete current graph` (:kbd:`Cmd+Backspace`)
  removes the active graph. The same items are on a graph's right-click menu
  in the session tree, together with :guilabel:`Move Up` and
  :guilabel:`Move Down`; graphs can also be dragged into a new order.

Graph settings
--------------

The :guilabel:`Graph` panel in the sidebar shows the settings of the graph in
the editor. :guilabel:`View Settings...` on a graph's right-click menu shows
the same settings in the main view.

.. SCREENSHOT using/graphs-01: the Graph panel in the sidebar with every
   property visible, MIDI Program set and a hotkey mapped
.. .. figure:: /images/using/graphs-01.png
..    :alt: Graph settings
..
..    Graph settings.

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

Switching graphs
----------------

The active graph is the one rendering and receiving MIDI. It is highlighted
in green in the session tree. To activate a graph:

- double-click it in the session tree;
- press its hotkey;
- send the MIDI program change assigned to it. |El| scans root graphs in
  session order and activates the first one whose :guilabel:`MIDI Program`
  matches and whose channels include the message's channel. Program changes
  with no matching graph leave the active graph unchanged.

Single-clicking a graph in the tree only shows it in the editor, so you can
edit an inactive graph while another plays.

.. tip::

   For a live set, give each song its own root graph and its own program
   number, then step through them from a controller or a MIDI Set List node
   (:ref:`nodes-midi`).

Subgraphs
---------

A subgraph is a graph node inside a graph. It renders as part of its parent,
appears as one block, and has its own input and output nodes that become
the block's ports. Add one with :guilabel:`Add graph...` on a graph's
right-click menu in the session tree, or from the :guilabel:`Element` group
of the :guilabel:`Plugins` menu in the graph editor.

Double-click the block to edit the subgraph's contents; the breadcrumb bar at
the top of the editor leads back to the parent. Subgraphs can be nested to any
depth, duplicated, and exported like any other node's containing graph. They
have no MIDI program or hotkey of their own but do have their own MIDI channel,
key range and transpose settings in the :guilabel:`Node` panel.
