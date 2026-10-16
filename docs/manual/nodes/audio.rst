.. include:: /shortcuts.rstext

.. _nodes-audio:

Audio Nodes
===========

The nodes in this chapter process, route or play audio. All of them are
listed under :guilabel:`Element` in the add-node menus and are available in
every build. Unless a custom editor is mentioned, a node's parameters are
shown in the generic editor of its plugin window and the :guilabel:`Editor`
panel, and every parameter can be MIDI mapped (:ref:`using-midi-mapping`).
Saving a node's settings as a preset is covered in
:ref:`using-nodes-and-plugins`.

Volume
------

A gain stage, in mono and stereo versions.

Parameters
   :guilabel:`Volume` in dB.

Wet/Dry
-------

Combines a stereo wet signal and a stereo dry signal into one stereo output,
for building parallel effect chains.

Ports
   Two stereo inputs (wet and dry) and one stereo output.

Parameters
   :guilabel:`Wet Level` and :guilabel:`Dry Level`, 0 to 1.

eVerb
-----

A stereo reverb.

Parameters
   :guilabel:`Room Size`, :guilabel:`Damping`, :guilabel:`Wet Level`,
   :guilabel:`Dry Level` and :guilabel:`Width`, each 0 to 1.

Factory presets
   :guilabel:`Default`, :guilabel:`Small Room`, :guilabel:`Bright Room`,
   :guilabel:`Dark Room`, :guilabel:`Studio`, :guilabel:`Live Stage`,
   :guilabel:`Club`, :guilabel:`Small Hall`, :guilabel:`Concert Hall`,
   :guilabel:`Large Hall`, :guilabel:`Cathedral`, :guilabel:`Ambient`,
   :guilabel:`Ethereal`, :guilabel:`Plate`, :guilabel:`Spring`,
   :guilabel:`Tight`, :guilabel:`Spacious`, :guilabel:`Warm`,
   :guilabel:`Shimmer` and :guilabel:`Gated`, under :menuselection:`Presets
   --> Factory Presets`.

EQ Filter
---------

A single-band parametric filter with its own editor.

Parameters
   :guilabel:`Cutoff Frequency [Hz]`, :guilabel:`Filter Q` (default
   0.707), :guilabel:`Filter Gain [dB]` and :guilabel:`EQ Shape`:
   :guilabel:`Bell`, :guilabel:`Notch`, :guilabel:`Hi Shelf`,
   :guilabel:`Low Shelf`, :guilabel:`HPF` or :guilabel:`LPF`. Gain applies
   to the bell and shelf shapes.

Chain several EQ Filter nodes for a multi-band equaliser.

.. SCREENSHOT nodes/audio-01: the EQ Filter editor in a plugin window
.. .. figure:: /images/nodes/audio-01.png
..    :alt: The EQ Filter editor
..
..    The EQ Filter editor.

Frequency Band Splitter
-----------------------

Splits its input into three frequency bands on separate outputs, for
multi-band processing.

Ports
   One input; :guilabel:`Low`, :guilabel:`Mid` and :guilabel:`High`
   outputs.

Parameters
   :guilabel:`Low Frequency [Hz]` (default 500) and :guilabel:`High
   Frequency [Hz]` (default 2000), 20 Hz to 22 kHz.

Compressor
----------

A dynamics compressor with a side-chain input and a custom editor showing the
transfer curve.

Ports
   :guilabel:`Main` and :guilabel:`Sidechain` inputs, one output. Enable
   the side-chain bus from the block's cog button if it is not shown.

Parameters
   :guilabel:`Threshold [dB]` (-30 to 0), :guilabel:`Ratio` (0.5 to 10),
   :guilabel:`Knee [dB]` (0 to 12), :guilabel:`Attack [ms]` (0.1 to 1000),
   :guilabel:`Release [ms]` (10 to 3000), :guilabel:`Makeup [dB]` (-18 to
   18) and :guilabel:`Side Chain` (0 to 1), which blends the detector between
   the main input and the side-chain input.

Comb Filter
-----------

A feedback comb filter, in mono and stereo versions. Short buffer lengths
give resonant, pitched effects; long ones give echoes.

Parameters
   :guilabel:`Buffer Length` (1 to 500, default 90), :guilabel:`Damping`
   (0 to 1) and :guilabel:`Feedback Level` (0 to 1, default 0.5).

AllPass Filter
--------------

An all-pass filter, in mono and stereo versions. It changes phase without
changing the frequency balance; use it for diffusion and phaser-style
effects.

Parameters
   :guilabel:`Buffer Length` (1 to 500, default 90).

Audio Mixer
-----------

A four-track stereo mixer with a master section, with an editor of channel
strips.

Ports
   Four stereo inputs (:guilabel:`Track 1` to :guilabel:`Track 4`) and a
   stereo :guilabel:`Master` output.

Parameters
   Per track, volume and mute; :guilabel:`Master Volume` (-120 to 12 dB)
   and :guilabel:`Master Mute`.

Audio Router
------------

An audio patch grid: any input to any output, with a click-free crossfade
when routes change.

Ports
   Equal numbers of audio inputs and outputs, plus a :guilabel:`MIDI In`
   port through which program changes select stored routings.

Editor
   A grid of inputs against outputs; click a cell to connect. The
   :guilabel:`Size` button chooses :guilabel:`2x2`, :guilabel:`4x4`,
   :guilabel:`8x8`, :guilabel:`10x10`, :guilabel:`12x12` or
   :guilabel:`16x16`. The knob sets the fade time from 1 ms to 2 s.

Factory presets
   :guilabel:`Linear Stereo` and :guilabel:`Inverse Stereo`.

.. SCREENSHOT nodes/audio-02: the Audio Router editor at 4x4 with two routes
   set
.. .. figure:: /images/nodes/audio-02.png
..    :alt: The Audio Router editor
..
..    The Audio Router editor.

Audio File Player
-----------------

Plays an audio file from disk, optionally in sync with the transport.

Ports
   A stereo output.

Editor
   A file chooser, :guilabel:`Play`, :guilabel:`Stop`, :guilabel:`Seek to
   Zero`, :guilabel:`Loop`, :guilabel:`MIDI S/S/C` (follow MIDI Start, Stop
   and Continue) and :guilabel:`Host` (follow the transport).

Parameters
   :guilabel:`Playing`, :guilabel:`Slave`, :guilabel:`Volume` (-60 to 12
   dB) and :guilabel:`Loop`, so playback can be MIDI mapped.

Media Player
------------

A simpler player for audio files.

Ports
   A stereo output.

Editor
   A file chooser and a :guilabel:`Play` / :guilabel:`Pause` button.

Parameters
   :guilabel:`Playing`, :guilabel:`Slave` and :guilabel:`Volume` (-60 to
   12 dB).
