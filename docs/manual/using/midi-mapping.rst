.. include:: /shortcuts.rstext

.. _using-midi-mapping:

MIDI Mapping
============

A MIDI mapping binds a note or continuous controller from a MIDI device to
something in |El|: a plugin parameter, a node's enable, bypass, mute or gain,
tap tempo, or a transport button. Mappings are learned in a couple of
gestures and saved with the session.

Learning a parameter
--------------------

#. Click :guilabel:`learn` in the toolbar, or press :kbd:`Cmd+M`. The button
   lights while learn mode is on.
#. Move the parameter you want to control: turn a knob in a plugin window or
   the :guilabel:`Editor` panel, or click a node's power or mute button.
#. Move the control on your MIDI device, or press a key. |El| captures the
   note or CC together with the device it came from.

The mapping is created and live immediately, learn mode ends, and the
:guilabel:`MIDI Mappings` view (below) shows the new entry. A mapped
parameter blinks in the plugin window when MIDI moves it.

Learning tap tempo and the transport
------------------------------------

Tap tempo and the four transport buttons are learned without the parameter
step:

- Right-click :guilabel:`TAP` and choose :guilabel:`MIDI Learn Tap Tempo`,
  or turn learn mode on and click :guilabel:`TAP`.
- Right-click Play, Stop, Record or Seek-to-start and choose
  :guilabel:`MIDI Learn` followed by the action, or turn learn mode on and
  click the button.

Then send the note or CC. The same right-click menus offer
:guilabel:`Re-learn MIDI Mapping` and :guilabel:`Clear MIDI Mapping`
afterwards. These are trigger mappings: they fire once when the control
crosses its threshold, so holding a knob past it does not retrigger.

The MIDI Mappings view
----------------------

:menuselection:`View --> MIDI Mappings` (:kbd:`Shift+Cmd+M`) lists every
mapping in the session in a table with :guilabel:`Device`,
:guilabel:`Event`, channel, :guilabel:`Node` and :guilabel:`Parameter`
columns. The :guilabel:`Filter` section narrows it by :guilabel:`Device`,
:guilabel:`Graph`, :guilabel:`Node` and :guilabel:`Target`
(:guilabel:`All Targets`, :guilabel:`Nodes` or :guilabel:`Session`).

.. SCREENSHOT using/midi-mapping-01: the MIDI Mappings view with several
   mappings listed, one selected, its settings shown
.. .. figure:: /images/using/midi-mapping-01.png
..    :alt: The MIDI Mappings view
..
..    The MIDI Mappings view.

Select a row to edit it:

:guilabel:`Name`
   A label of your own.

:guilabel:`Device`
   :guilabel:`Any Device`, or one input. A mapping learned from a device
   remembers it; choose :guilabel:`Any Device` to let any controller drive
   it.

:guilabel:`Event`
   :guilabel:`MIDI CC` or :guilabel:`Note`, with the :guilabel:`CC Number`
   or :guilabel:`Note` below it.

:guilabel:`Channel`
   :guilabel:`Omni` or 1 to 16.

:guilabel:`Latch`
   For notes: :guilabel:`Toggle on each note-on` switches the target between
   its two states on every press instead of following the key.

:guilabel:`Trigger`, :guilabel:`Threshold`
   For tap tempo and transport mappings: fire :guilabel:`At or Above` the
   threshold, or when the control is :guilabel:`Touched 0` or
   :guilabel:`Touched 127`.

:guilabel:`Node`, :guilabel:`Parameter`
   The target. Besides the plugin's own parameters the list offers
   :guilabel:`Enable/Disable`, :guilabel:`Bypass`, :guilabel:`Mute`,
   :guilabel:`Input Gain` and :guilabel:`Output Gain`.

Delete a mapping with the delete button, the :kbd:`Delete` key, or
:guilabel:`Delete Mapping` on its right-click menu. A mapping whose node has
been removed from the session is dropped the next time the session loads.

Notes
-----

- A CC drives a parameter across its full range; a note sets it to maximum
  on note-on and minimum on note-off, or toggles with :guilabel:`Latch`.
- Mappings target a node, not a graph, so they keep working when the node's
  graph is inactive only if that graph still renders (see
  :guilabel:`Rendering Mode` in :ref:`interface-graph-panel`).
- To switch graphs from a controller, use MIDI program changes rather than
  mappings (:ref:`using-graphs`).
