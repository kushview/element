.. include:: /shortcuts.rstext

.. _plugin-editions:

Plugin Editions
===============

The plugin editions put the |El| engine inside a DAW or other host. They
open the same sessions and graphs as the standalone application, and the
interface is the same main window with a few differences described here.

The three editions
------------------

.. list-table::
   :header-rows: 1
   :widths: 22 38 40

   * - Edition
     - Buses
     - Formats
   * - Element
     - Presents itself as an instrument: a stereo :guilabel:`Main` output,
       a stereo :guilabel:`Main` input that is off by default, sixteen
       stereo :guilabel:`Aux` inputs and outputs, MIDI in and out.
     - AU, VST3, CLAP, LV2, and VST where built
   * - Element FX
     - Presents itself as an effect: stereo :guilabel:`Main` in and out,
       sixteen stereo :guilabel:`Aux` inputs and outputs, MIDI in and out.
     - AU, VST3, CLAP, LV2, and VST where built
   * - Element MFX
     - Presents itself as a MIDI effect: MIDI in and out, no audio.
     - AU, macOS only

Enable the aux buses in the host to route extra channels in and out of the
graph; they appear as ports on the root graph's input and output nodes.

Differences from the standalone application
-------------------------------------------

- There is no menu bar. The toolbar has a menu button with the session, edit
  and view commands, :guilabel:`About Element` and :guilabel:`Close all
  plugin windows...`.
- The :guilabel:`EXT` button is always present and, when on, follows the
  host's transport and tempo. Turn it off to run |El|'s transport on its
  own.
- The status bar reads :guilabel:`Host` and shows the latency reported to
  the host in samples.
- Plugin windows of nodes inside the graph stay on top of the host.
- There is no system tray, no autosave recovery and no MIDI clock
  generation; the host owns saving and timing. The audio and MIDI device
  preferences do not apply, since the host provides the streams.
- Keyboard shortcuts are the defaults listed in :ref:`appendix-shortcuts`.

Performance parameters
----------------------

Each edition exposes eight automatable parameters to the host, shown as
sliders along the bottom of the editor. Right-click a slider to bind it: the
menu lists every node in the session with its parameters, and choosing one
links the slider to it. Choose :guilabel:`Unlink`, or the same parameter
again, to remove the binding. Bindings are saved with the plugin state, so
host automation drives the same node parameter every time the project
loads.

.. SCREENSHOT plugin/editions-01: the Element FX editor inside a host with
   the eight performance sliders visible and one right-click menu open
.. .. figure:: /images/plugin/editions-01.png
..    :alt: The plugin editor with performance parameters
..
..    The plugin editor with its performance parameters.

Script node parameters can be bound in the same way, which lets a Lua
control script react to host automation.

Sharing sessions
----------------

Save a session in the standalone application and open it from the plugin's
menu, or the other way round; graphs exported as ``.elg`` files import into
either. Plugins that are missing on one side load as placeholders.
