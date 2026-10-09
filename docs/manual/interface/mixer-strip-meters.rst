.. include:: /shortcuts.rstext

.. _interface-mixer-strip-meters:

Graph Mixer, Channel Strip and Meter Bridge
===========================================

Three views give level control and metering for the nodes of a graph. All
three work on the graph shown in the editor (:ref:`interface-graph-editor`),
and the strip colours and selection follow it.

Graph Mixer
-----------

:menuselection:`View --> Graph Mixer` opens the mixer in the accessory view
under the main view. It shows one channel strip per node that produces audio:
a fader, a level meter, a dB readout, and mute and bypass buttons. The strip's
name header takes the colour given to the node's block, and the selected
node shows the same accent outline as in the graph editor.

.. SCREENSHOT interface/mixer-strip-meters-01: the Graph Mixer in the
   accessory view with four or five strips, one selected, one coloured
.. .. figure:: /images/interface/mixer-strip-meters-01.png
..    :alt: The Graph Mixer
..
..    The Graph Mixer.

- Drag a fader to change the node's output gain; double-click it to return to
  0 dB.
- Right-click a strip and choose :guilabel:`Hide from mixer` to remove it
  from view.
- Right-click the mixer background to restore hidden strips: the menu lists
  :guilabel:`Show` followed by each hidden node, :guilabel:`Show all`, or
  :guilabel:`No hidden channels` when nothing is hidden.

The mixer reads :guilabel:`No channels to display` for a graph without audio
nodes.

Channel Strip
-------------

:menuselection:`View --> Channel Strip` adds a single, larger strip at the
right edge of the window for the selected node. It has a fader from -60 to
+6 dB (double-click for 0 dB), a meter, a readout, and power and mute
buttons. Two drop-downs choose what the meter shows: :guilabel:`Signal flow
to monitor` (:guilabel:`Input` or :guilabel:`Output`) and
:guilabel:`Channel(s) to monitor`. The strip follows the selection in the
graph editor and session tree.

Meter Bridge
------------

:menuselection:`View --> Meter Bridge` (:kbd:`Alt+M`) shows a row of meters
along the bottom of the window for the graph's audio inputs and outputs.
Right-click it to choose what it shows, :guilabel:`Audio Ins` and
:guilabel:`Audio Outs`, and the meter size: :guilabel:`Small`,
:guilabel:`Normal`, :guilabel:`Large` or :guilabel:`Extra Large`. The
choices are remembered between sessions.

The meters are RMS meters calibrated so that a -20 dB test tone reads
-20 dB.
