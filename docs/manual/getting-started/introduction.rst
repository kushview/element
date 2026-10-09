.. include:: /shortcuts.rstext

.. _getting-started-introduction:

Introduction
============

|El| is a modular audio plugin host. You build instruments and effects racks
by placing plugins and built-in processors on a graph and connecting them to
each other, to your audio and MIDI hardware, and to nested graphs. It is made
for live performance as much as for the studio: switch whole graphs with a
MIDI program change, map any parameter to a controller, and script your own
processors in Lua.

|El| runs as a standalone application and as a plugin inside a DAW. Sessions
and graphs created in one can be opened in the other.

What you can do with it
-----------------------

- Route audio and MIDI from anywhere to anywhere, including between plugins
  of different formats.
- Play virtual instruments and effects live, with a built-in virtual keyboard
  when no hardware is attached.
- Build reusable instruments and effect graphs, and nest graphs inside each
  other.
- Switch between root graphs with MIDI program changes; play several at once.
- Map MIDI controllers to plugin parameters, node state, tap tempo and the
  transport.
- Follow an external MIDI clock, or generate one.
- Embed plugin editors directly in the graph.
- Save any node's state as a preset, or as a set of MIDI programs.
- Extend |El| with Lua: DSP scripts, script editors and a live console.
- Control the engine over OSC and translate between OSC and MIDI.

Editions
--------

The standalone application is the main way to use |El|. Three plugin editions
wrap the same engine for use inside a host:

.. list-table::
   :header-rows: 1
   :widths: 25 45 30

   * - Edition
     - Presents itself as
     - Formats
   * - Element
     - An instrument with audio outputs and MIDI in and out
     - AU, VST3, CLAP, LV2 (VST2 where built)
   * - Element FX
     - An effect with audio in and out
     - AU, VST3, CLAP, LV2 (VST2 where built)
   * - Element MFX
     - A MIDI effect
     - AU (macOS only)

:ref:`plugin-editions` covers the differences.

Supported platforms and plugin formats
--------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 25 25 50

   * - Platform
     - Minimum version
     - Plugin formats hosted
   * - macOS
     - 10.13
     - AU, VST, VST3, LV2, CLAP
   * - Windows
     - 10
     - VST, VST3, LV2, CLAP
   * - Linux
     - any current distribution
     - LV2, VST3, CLAP (VST where built)

VST here means VST 2.4. It is only hosted by builds made with VST2 support.
The :ref:`using-plugin-manager` chapter explains scanning and search paths for
each format.

Free software
-------------

|El| is free software released under the GNU General Public License, version 3
or later. The source code is on the `source repository`_. Bugs and feature
requests go to the `issue tracker`_, and the `Discord server`_ is where
development is discussed. Pre-built binaries and the update service are
available from the `Element website`_.

About this manual
-----------------

- Menu paths are written as :menuselection:`View --> Plugin Manager`.
- Buttons, labels and options are written as :guilabel:`Scan`.
- Keys are written as :kbd:`Cmd+S`. :kbd:`Cmd` is the Command key on macOS;
  on Windows and Linux use :kbd:`Ctrl` instead. :kbd:`Alt` is Option on
  macOS.
- Terms defined in the :ref:`appendix-glossary` are linked on first use, for
  example :term:`MIDI`.

The manual is also available as the `online manual`_.
