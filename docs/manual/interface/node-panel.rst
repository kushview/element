.. include:: /shortcuts.rstext

.. _interface-node-panel:

The Node and Editor Panels
==========================

The :guilabel:`Node` panel in the sidebar shows the properties of the
selected node, and the :guilabel:`Editor` panel shows the node's own editor
inline, so a plugin can be adjusted without opening a window. Both follow the
selection in the graph editor and session tree.

Each panel has a drop-down to pick any node of the current graph directly,
and a menu button with a :guilabel:`Sticky` option. A sticky panel stays on
its node instead of following the selection, which is useful for keeping one
plugin's editor in view while editing others. For a Script node the
:guilabel:`Node` panel also has a :guilabel:`Script` button with
:guilabel:`Edit DSP Script` and :guilabel:`Edit UI Script`.

.. SCREENSHOT interface/node-panel-01: the Node panel for an instrument
   plugin with the MIDI section and two saved MIDI programs, and the Editor
   panel below it showing the plugin editor
.. .. figure:: /images/interface/node-panel-01.png
..    :alt: The Node and Editor panels
..
..    The Node panel (top) and Editor panel (bottom).

Node section
------------

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
------------

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
----------------

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
