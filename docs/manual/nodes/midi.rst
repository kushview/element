.. include:: /shortcuts.rstext

.. _nodes-midi:

MIDI Nodes
==========

The nodes in this chapter filter, route and generate MIDI. All have a
:guilabel:`MIDI In` and a :guilabel:`MIDI Out` port unless stated otherwise.

MIDI Channelize
---------------

Moves every incoming message to one channel.

Parameters
   :guilabel:`Out Channel`, 1 to 16.

MIDI Channel Map
----------------

Remaps channels individually: each of the sixteen input channels can be sent
to any output channel.

Parameters
   :guilabel:`Channel 1` to :guilabel:`Channel 16`, each set to the output
   channel for that input channel.

MIDI Channel Splitter
---------------------

Splits a MIDI stream by channel onto separate outputs.

Ports
   :guilabel:`MIDI In`; sixteen outputs :guilabel:`Ch. 1` to
   :guilabel:`Ch. 16`. Hide the unused ones with :guilabel:`Ports...`.

MIDI Router
-----------

A MIDI patch grid with four inputs and four outputs.

Ports
   :guilabel:`Input 1` to :guilabel:`Input 4`, :guilabel:`Output 1` to
   :guilabel:`Output 4`.

Editor
   A grid of inputs against outputs; click a cell to connect.

MIDI Program Map
----------------

Translates program change numbers. Program changes listed in the table are
replaced by their output program; everything else passes through unchanged.

Editor
   A table with :guilabel:`Name`, :guilabel:`Input` and :guilabel:`Output`
   columns, :guilabel:`+` and :guilabel:`-` to add and remove rows, and a
   font size control. Selecting a row and clicking the send button emits
   that row's output program, which is a convenient way to test a
   downstream instrument.

Use it in front of a plugin whose programs do not line up with the numbers
your controller sends.

MIDI Set List
-------------

A song list driven by program changes. Each entry maps an incoming program
to an outgoing one and can set the session tempo and time signature when it
is selected, so one program change from a foot controller reconfigures the
whole session.

Editor
   A table with :guilabel:`IN`, :guilabel:`NAME`, :guilabel:`TEMPO`,
   :guilabel:`SIG` and :guilabel:`OUT` columns, and :guilabel:`+` and
   :guilabel:`-` buttons. Leave the tempo or signature at :guilabel:`N/A`
   to keep the current value.

.. SCREENSHOT nodes/midi-01: the MIDI Set List editor with three songs
.. .. figure:: /images/nodes/midi-01.png
..    :alt: The MIDI Set List editor
..
..    The MIDI Set List editor.

Combine it with the :guilabel:`MIDI Program` of root graphs
(:ref:`using-graphs`) to switch graphs and tempo together.

MIDI Monitor
------------

Logs the messages passing through it and passes them on unchanged. The
block shows a MIDI activity indicator.

Editor
   The log, with a :guilabel:`Clear` button. Notes are shown with their
   names in scientific pitch notation; Start, Stop and Continue are named.

MIDI Input Device and MIDI Output Device
----------------------------------------

Bind a graph to one specific MIDI port instead of the global inputs and
output. Add them from the :guilabel:`MIDI Input Device` and
:guilabel:`MIDI Output Device` submenus of the graph editor's background
menu, which list the ports currently available.

Ports
   :guilabel:`MIDI In Device` has a :guilabel:`MIDI Out` port; :guilabel:`MIDI
   Out Device` has a :guilabel:`MIDI In` port.

Editor
   The device selector, and for an output device :guilabel:`Output latency
   (ms)`.

A device node keeps its device name in the session and reconnects when the
device is available again after being unplugged.
