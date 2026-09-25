.. include:: /shortcuts.rstext

.. _scripting-script-node:

The Script Node
===============

The Script node runs a DSP script as a processor in a graph. Its ports and
parameters are whatever the script's ``layout`` declares, so one node type
can be a MIDI filter, a control source, a tone generator or an effect.

Adding one
----------

Add :guilabel:`Script` from the :guilabel:`Element` group of the add-node
menu. A new node runs the :guilabel:`Amp` program. Pick another from
:menuselection:`Presets --> Factory Presets` on the block's right-click menu:
:guilabel:`Amp`, :guilabel:`Channelizer`, :guilabel:`Spoton Scale Chooser`,
:guilabel:`MIDI Timecode (MTC) Generator`, :guilabel:`Value`,
:guilabel:`MIDI CC`, :guilabel:`Tremolo` or :guilabel:`Test Tone`
(:ref:`scripting-overview` describes them).

The node's window shows the script's editor if it has a DSPUI script, or the
generic parameter editor. The :guilabel:`Params` button toggles a table of
the script's parameters below the editor, and the file button loads a
``.lua`` file from your scripts folder.

.. SCREENSHOT scripting/script-node-01: a Script node's plugin window
   running Amp with its DSPUI editor and the Params table shown
.. .. figure:: /images/scripting/script-node-01.png
..    :alt: A Script node window
..
..    A Script node running the Amp program.

Editing the scripts
-------------------

A Script node holds two scripts, the DSP script and its UI script. Open
either in the script editor from:

- :guilabel:`Edit DSP Script` and :guilabel:`Edit UI Script` on the block's
  right-click menu;
- the :guilabel:`Script` button of the :guilabel:`Node` panel;
- the :guilabel:`DSP` and :guilabel:`UI` children of the node in the
  session tree.

The editor opens in the main view with Lua syntax colouring. Click
:guilabel:`Apply` to load the edited script into the node; the previous
script keeps running until the new one is accepted. Errors appear in orange
under the editor and in the console.

Validation
----------

Before a DSP script replaces the running one, |El| runs it for a few cycles
in a scratch state. A script that raises an error or produces non-finite
audio is rejected and the error is shown in the editor. If a script that has
been accepted later raises an error inside ``process``, the node disables
that script and reports the error instead of stopping the application.

Parameters and mapping
----------------------

The control inputs declared in the script's ``layout`` become parameters
of the node. They appear in the generic editor and the :guilabel:`Params`
table, can be MIDI mapped (:ref:`using-midi-mapping`), and in the plugin
editions can be bound to the performance parameters
(:ref:`plugin-editions`). Control outputs appear as control ports on the
block and can drive parameters of other nodes.

The script's state is saved with the session: parameter values
automatically, and anything else through the script's ``save`` and
``restore`` callbacks (:ref:`scripting-script-types`).
