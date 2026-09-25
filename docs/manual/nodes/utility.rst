.. include:: /shortcuts.rstext

.. _nodes-utility:

Utility and I/O Nodes
=====================

Audio In, Audio Out, MIDI In, MIDI Out
--------------------------------------

The four input and output nodes stand for the graph's connection to the
outside. In a root graph they are the audio device and the enabled MIDI
devices (:ref:`using-preferences`); in a subgraph they are the ports of the
subgraph's block in the parent.

The number of ports on each is set by :guilabel:`Audio Ins`,
:guilabel:`Audio Outs`, :guilabel:`MIDI Ins` and :guilabel:`MIDI Outs` in
the graph settings (:ref:`using-graphs`). The nodes are added and removed
with the :guilabel:`Graph I/O` items of the graph editor's background menu.
They have no mute or bypass buttons and never open a plugin window.

Graph
-----

A subgraph: a whole graph as a node. Add one with :guilabel:`Add graph...`
in the session tree or from :guilabel:`Element` in the add-node menu, and
double-click its block to edit its contents. See :ref:`using-graphs`.

Placeholder
-----------

Stands in for a plugin that could not be loaded, keeping the original node's
name, ports, connections and saved state so the session opens intact. It is
shown in bold red in the graph editor. Double-click it to see the reason,
and replace it with :guilabel:`Replace` or by dropping a plugin on it. The
node is never added deliberately; |El| creates it when needed.

OSC Receiver and OSC Sender
---------------------------

Translate between OSC messages on a UDP port and MIDI. The receiver has a
:guilabel:`MIDI Out` port; the sender has a :guilabel:`MIDI In` port. Their
editors hold the connection settings, :guilabel:`Connect` /
:guilabel:`Disconnect` and :guilabel:`Pause` / :guilabel:`Resume`. See
:ref:`using-osc` and :ref:`appendix-osc-commands`.

Script
------

Runs a Lua DSP script as a node, with an optional editor also written in
Lua. The node ships with eight programs, selectable from
:menuselection:`Presets --> Factory Presets`: :guilabel:`Amp`,
:guilabel:`Channelizer`, :guilabel:`Spoton Scale Chooser`,
:guilabel:`MIDI Timecode (MTC) Generator`, :guilabel:`Value`,
:guilabel:`MIDI CC`, :guilabel:`Tremolo` and :guilabel:`Test Tone`. Its
ports and parameters are whatever the script declares.
:ref:`scripting-script-node` covers writing and editing scripts for it.
