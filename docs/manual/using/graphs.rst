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

Every root graph has a name, a rendering mode, a velocity curve, a MIDI
channel filter, an optional MIDI program and hotkey that activate it, and a
port count. They are edited in the :guilabel:`Graph` panel of the sidebar, or
in the main view with :guilabel:`View Settings...` on the graph's right-click
menu. Each setting is described in :ref:`interface-graph-panel`.

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
