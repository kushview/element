.. include:: /shortcuts.rstext

.. _using-virtual-keyboard:

Virtual Keyboard
================

The virtual keyboard sends MIDI notes from the mouse or the computer keyboard
into the active graph, as if from a MIDI input device. Show it with
:menuselection:`View --> Virtual Keyboard` (:kbd:`Alt+K`); it appears along
the bottom of the window.

.. SCREENSHOT using/virtual-keyboard-01: the virtual keyboard strip with its
   controls, a few keys held
.. .. figure:: /images/using/virtual-keyboard-01.png
..    :alt: The virtual keyboard
..
..    The virtual keyboard.

Controls
--------

:guilabel:`Channel:`
   The MIDI channel the notes are sent on, 1 to 16.

:guilabel:`Program:`
   Sends a program change on the chosen channel when changed.

:guilabel:`Sustain`, :guilabel:`Hold`
   Pedal buttons, held while lit.

:guilabel:`Width:`
   The key width, from 14 to 24 pixels, with :guilabel:`-` and
   :guilabel:`+` buttons.

Playing from the computer keyboard
----------------------------------

Turn on :kbd:`Caps Lock` while the virtual keyboard is visible and the letter
keys play notes, wherever the focus is in the window: the row from
:kbd:`A` upwards plays the white keys of one octave and the row above plays
the black keys, in the usual two-row layout. Turn :kbd:`Caps Lock` off to get
the keys back for shortcuts and typing.

While the keyboard has focus these keys change its settings:

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Key
     - Action
   * - :kbd:`Left`, :kbd:`Right`
     - MIDI channel down or up
   * - :kbd:`Up`, :kbd:`Down`
     - Program up or down; with :kbd:`Alt`, in steps of ten
   * - :kbd:`0` to :kbd:`9`
     - Set the octave the letter keys play
   * - Numeric keypad :kbd:`-`, :kbd:`+`
     - Octave down or up
   * - :kbd:`-`, :kbd:`+`
     - Narrower or wider keys
   * - :kbd:`Cmd+Space`
     - Toggle :guilabel:`Sustain`
   * - :kbd:`Cmd+Alt+Space`
     - Toggle :guilabel:`Hold`

Note names in |El| use scientific pitch notation, with middle C as C4.

Keyboard splits and layers
--------------------------

Splits are a property of nodes rather than of the keyboard. In the
:guilabel:`Node` panel (:ref:`interface-node-panel`), give one instrument a
:guilabel:`Key Start` and :guilabel:`Key End` covering the low part of the
keyboard and another instrument the high part, feed both from the same MIDI
input, and each plays only its own range. Give two instruments the same
range to layer them, and use :guilabel:`Transpose` to shift one of them.
