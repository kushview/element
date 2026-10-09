.. include:: /shortcuts.rstext

.. _using-transport-and-clock:

Transport, Tempo and MIDI Clock
===============================

|El| has a transport with a position, a tempo and a time signature, which it
passes to every plugin that asks for host timing. The transport can run on
its own clock, follow an external MIDI clock, or be the clock master for
other devices.

The toolbar controls
--------------------

The transport lives in the toolbar (:ref:`interface-main-window`):

- The tempo box shows the session tempo, 20 to 999 BPM. Drag it, or
  double-click to type. :guilabel:`TAP` sets the tempo from your clicks; a
  pause of more than two seconds starts a new run of taps.
- The time signature shows beats per bar (1 to 99) and the note value; drag
  each half.
- The position readout shows bar, beat and sub-beat. Double-click the bar
  number to go to the start.
- Play, Stop, Record and Seek-to-start. Play while playing returns to the
  start; Stop while stopped does the same. Record toggles the record state
  reported to plugins.

Keyboard: :kbd:`Space` for Play, :kbd:`/` for Seek Start, :kbd:`J` and
:kbd:`L` to move backwards and forwards. Tap tempo and each transport button
can be mapped to MIDI (:ref:`using-midi-mapping`).

Clock source
------------

:guilabel:`Clock Source` on the :guilabel:`General` page of the preferences
selects :guilabel:`Internal` or :guilabel:`MIDI Clock`.

With :guilabel:`MIDI Clock`, the :guilabel:`EXT` button appears at the left
of the toolbar. While it is on, |El| follows the tempo of the MIDI clock
arriving on the enabled MIDI inputs, and the tempo box and :guilabel:`TAP`
are locked. Turn :guilabel:`EXT` off to go back to the internal tempo
without changing the preference. The choice of external sync is saved with
the session.

To have the transport start and stop with the master as well, enable
:guilabel:`Transport: MIDI Start/Stop` on the :guilabel:`MIDI` page:
incoming MIDI Start plays from the beginning, Stop stops, and Continue
resumes.

Being the clock master
----------------------

On the :guilabel:`MIDI` page of the preferences:

:guilabel:`Generate MIDI Clock`
   Sends MIDI clock, plus Start, Continue and Stop as the transport changes
   state, to the default MIDI output device.

:guilabel:`Send Clock to MIDI Input?`
   Feeds the generated clock into the root graphs instead of the output
   device, so plugins and MIDI nodes inside a graph can follow it, and it
   can be routed to a MIDI Output Device node.

:guilabel:`Output latency (ms)`
   Delays all MIDI output, from -1000 to 1000 ms, to line up external
   hardware with the audio.

Clock generation and output latency are only available in the standalone
application; the plugin editions follow the host.

In the plugin editions
----------------------

Inside a host the :guilabel:`EXT` button is always shown and follows the
host's transport and tempo. Turn it off to run |El|'s transport
independently.

Panic
-----

:guilabel:`Panic!` (:kbd:`Cmd+Alt+P`) sends all-notes-off to every node,
for stuck notes. A MIDI controller can trigger the same thing: enable
:guilabel:`MIDI Panic CC` on the :guilabel:`MIDI` page and choose the
channel (0 for any) and CC number.
