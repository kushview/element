.. include:: /shortcuts.rstext

.. _using-osc:

OSC
===

|El| speaks Open Sound Control in two places: an OSC host that accepts a
small set of engine commands, and a pair of nodes that translate between
OSC and MIDI inside a graph.

The OSC host
------------

On the :guilabel:`OSC` page of the preferences, :guilabel:`OSC Host Enabled?`
starts a UDP server on :guilabel:`OSC Host Port` (9000 by default);
:guilabel:`OSC Host` shows the address clients should send to. If the port
cannot be opened, the status bar reports it.

The host currently understands one address:

.. list-table::
   :header-rows: 1
   :widths: 45 55

   * - Address and arguments
     - Action
   * - ``/element/engine samplerate <rate>``
     - Changes the audio device's sample rate. ``<rate>`` is an integer or
       float.

OSC Receiver and OSC Sender nodes
---------------------------------

The two nodes, under :guilabel:`Element` in the add-node menu, connect any
OSC-capable software or hardware to the MIDI side of a graph.

OSC Receiver
   Listens on a UDP port (9001 by default) and turns incoming ``/midi/...``
   messages into MIDI on its :guilabel:`MIDI Out` port. Its editor has the
   port, :guilabel:`Connect` / :guilabel:`Disconnect`, :guilabel:`Pause` /
   :guilabel:`Resume`, and a log of received messages with
   :guilabel:`Clear`.

OSC Sender
   Turns MIDI arriving at its :guilabel:`MIDI In` port into ``/midi/...``
   messages sent to a host and port. Its editor has the host, the port,
   :guilabel:`Connect` / :guilabel:`Disconnect` and :guilabel:`Pause` /
   :guilabel:`Resume`.

Messages use the address ``/midi/<command>`` or, to name a device,
``/midi/<device>/<command>``. :ref:`appendix-osc-commands` lists every
command and its arguments. A note-on, for example, is::

   /midi/noteOn 1 60 0.8

for channel 1, middle C, velocity 0.8. Messages the sender cannot translate
are sent as ``/midi/unknown``.
