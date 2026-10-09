.. include:: /shortcuts.rstext

.. _interface-patch-bay:

The Patch Bay
=============

The patch bay shows the same graph as the graph editor as a grid: every
output port is a row, every input port a column, and a filled cell is a
connection. It suits dense routing where cables become hard to follow. Open
it with :menuselection:`View --> Patch Bay` (:kbd:`F1`), or cycle between it
and the graph editor with the :guilabel:`view` toolbar button.

.. SCREENSHOT interface/patch-bay-01: the patch bay for a graph with several
   nodes, a few cells filled, Route and Clear buttons visible
.. .. figure:: /images/interface/patch-bay-01.png
..    :alt: The patch bay
..
..    The patch bay.

Rows and columns are grouped by node and coloured by port type, so audio can
only be connected to audio and MIDI to MIDI.

- Click a cell to connect or disconnect the row's output from the column's
  input.
- Double-click a node's row header to open its plugin window, or to enter it
  if it is a subgraph.
- Select a row header and press :kbd:`Delete` to remove the node.

Right-click a row or column header for the same menu as a block in the graph
editor (:ref:`interface-graph-editor`), with two additions: a
:guilabel:`Sources` submenu on an input, or :guilabel:`Destinations` on an
output, lists the ports of the other nodes so a connection can be made
without finding the cell. Right-click empty space for the
:guilabel:`Graph I/O` and :guilabel:`Plugins` sections of the background
menu.

Two buttons sit above the grid:

:guilabel:`Route`
   Connects the selected source rows to the selected destination columns,
   pairing them in order and by type. Select two stereo outputs and two
   stereo inputs, click :guilabel:`Route`, and left goes to left and right to
   right.

:guilabel:`Clear`
   Removes every connection in the graph.
