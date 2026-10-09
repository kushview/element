.. include:: /shortcuts.rstext

.. _appendix-osc-commands:

OSC Commands
============

OSC host
--------

Messages accepted by the OSC host when it is enabled in the preferences
(:ref:`using-osc`):

.. list-table::
   :header-rows: 1
   :widths: 45 55

   * - Address and arguments
     - Action
   * - ``/element/engine samplerate <int|float>``
     - Change the audio device's sample rate.

OSC Receiver and OSC Sender nodes
---------------------------------

The receiver turns these messages into MIDI; the sender emits them for MIDI
it receives. The address is ``/midi/<command>`` or
``/midi/<device>/<command>``, where ``<device>`` is a name of your choosing
that the receiver ignores. Channels are 1 to 16; note numbers and controller
values are 0 to 127; velocities are floats from 0 to 1.

.. list-table::
   :header-rows: 1
   :widths: 30 45 25

   * - Command
     - Arguments
     - MIDI message
   * - ``raw``
     - ``blob`` MIDI bytes
     - The bytes as sent
   * - ``noteOn``
     - ``int`` channel, ``int`` note, ``float`` velocity
     - Note on
   * - ``noteOff``
     - ``int`` channel, ``int`` note, ``float`` velocity
     - Note off
   * - ``programChange``
     - ``int`` channel, ``int`` program
     - Program change
   * - ``pitchBend`` or ``pitchWheel``
     - ``int`` channel, ``int`` position (0 to 16383)
     - Pitch bend
   * - ``afterTouch``
     - ``int`` channel, ``int`` note, ``int`` amount
     - Polyphonic aftertouch
   * - ``controlChange``
     - ``int`` channel, ``int`` controller, ``int`` value
     - Control change
   * - ``allNotesOff``
     - ``int`` channel
     - All notes off
   * - ``allSoundOff``
     - ``int`` channel
     - All sound off
   * - ``allControllersOff``
     - ``int`` channel
     - Reset all controllers
   * - ``start``
     -
     - Start
   * - ``continue``
     -
     - Continue
   * - ``stop``
     -
     - Stop
   * - ``clock``
     -
     - Timing clock
   * - ``songPositionPointer``
     - ``int`` position in MIDI beats
     - Song position pointer
   * - ``activeSense``
     -
     - Active sensing

The sender emits ``/midi/unknown`` for MIDI messages that have no command
in this table.
