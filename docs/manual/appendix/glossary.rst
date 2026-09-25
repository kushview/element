.. include:: /shortcuts.rstext

.. _appendix-glossary:

Glossary
========

.. glossary::
   :sorted:

   session
     The document |El| opens and saves (an ``.els`` file). It holds one or
     more root :term:`graphs <graph>`, the tempo and time signature, the MIDI
     mappings and the window layout.

   graph
     A set of :term:`nodes <node>` and the connections between them. Graphs
     listed directly under the session are :term:`root graphs <root graph>`;
     a graph placed inside another graph is a :term:`subgraph`.

   root graph
     A graph at the top level of the session. It has its own audio and MIDI
     input and output nodes, MIDI channel and program settings, and a
     rendering mode.

   active graph
     The root graph that is rendering and receiving MIDI. Only one is active
     at a time; in :guilabel:`Parallel` rendering mode the others render too.

   subgraph
     A graph nested inside another graph. It appears as one block with its
     own inputs and outputs.

   node
     One processor in a graph: a plugin loaded from disk or one of |El|'s
     built-in nodes. Nodes have :term:`ports <port>`, parameters and settings
     such as enable, bypass, mute, MIDI channel and key range.

   port
     An input or output of a node. Ports carry audio, MIDI or control data,
     and a connection joins an output port to an input port of the same kind.

   placeholder
     A node standing in for a plugin that could not be loaded on this
     computer. It keeps the node's connections and settings so the session
     still opens.

   patch bay
     The grid view of a graph's connections, an alternative to the cables of
     the graph editor.

   preset
     The saved state of a node (an ``.eln`` file), or a program provided by a
     plugin itself.

   MIDI program
     One of up to 128 snapshots of a node's state recalled by a MIDI program
     change message.

   MIDI learn
     Creating a MIDI mapping by moving a parameter and then a control on a
     MIDI device.

   MIDI clock
     Timing messages sent 24 times per quarter note, with Start, Stop and
     Continue, that let devices follow one tempo. |El| can follow or generate
     it.

   program change
     A :term:`MIDI` message selecting a program number from 1 to 128. |El|
     uses it to switch root graphs and node programs.

   CC
   control change
     A :term:`MIDI` message carrying a controller number and a value from 0
     to 127, sent by knobs, faders and pedals.

   SysEx
   System Exclusive
     A :term:`MIDI` message of arbitrary length, defined by a manufacturer,
     used for device-specific settings and data.

   velocity curve
     A curve applied to note velocities entering a graph to suit a keyboard
     or playing style.

   oversampling
     Running a node at a multiple of the session sample rate to reduce
     aliasing, at a cost in CPU.

   delay compensation
     Delay added to some paths in a graph so that parallel paths with
     different latencies arrive at the same time.

   unverified plugin
     A plugin file found in the search paths that has not been scanned yet.
     It can be added directly from the :guilabel:`Unverified` menu.

   DSP script
     A Lua script that processes audio and MIDI inside a Script node.

   DSPUI script
     A Lua script that provides the editor of a DSP script.

   Lua
     The scripting language embedded in |El|.

   OSC
   Open Sound Control
     A network protocol for control messages, carried over UDP. |El| accepts
     a few engine commands over OSC and translates between OSC and MIDI with
     the OSC Receiver and OSC Sender nodes.

   ASIO
   Audio Stream Input/Output
     A low-:term:`latency` audio driver :term:`API` on Windows, used with
     professional audio interfaces.

   CoreAudio
     The audio system of macOS, used by |El| for every audio device there.

   JACK
     A low-latency audio server for Linux that routes audio between
     applications. |El| can use it as its audio device type.

   audio interface
     A device that brings audio in and out of the computer, usually over
     USB. Every computer has a basic one built in.

   latency
     The delay between a signal entering a system and leaving it, usually
     measured in milliseconds. Smaller audio buffers give lower latency at
     the cost of CPU.

   BPM
   Beats per minute
     The unit of tempo.

   tempo
     The speed of the transport in :term:`BPM`.

   MIDI
   Musical Instrument Digital Interface
     The protocol instruments, controllers and software use to exchange
     notes and control data. |El| receives MIDI from devices and routes it
     between nodes.

   controller
     A hardware device with knobs, faders, pads or keys that sends
     :term:`MIDI`. Its controls can be mapped to |El| parameters.

   pitch bend
     A :term:`MIDI` message that smoothly raises or lowers the pitch of the
     sounding notes, usually from the pitch wheel of a keyboard.

   dB
   decibel
     A logarithmic unit of level. In |El|, 0 dB on a fader is unity gain and
     a change of 6 dB roughly doubles or halves the amplitude.

   EQ
   Equalizer
     A filter that boosts or cuts a range of frequencies. The EQ Filter node
     is a single-band parametric equalizer.

   DAW
   Digital Audio Workstation
     Software for recording, editing and producing audio. The plugin
     editions run |El| inside one.

   AU
   Audio Units
     Apple's plugin format for macOS. |El| hosts Audio Units and is
     available as one.

   VST
     Steinberg's Virtual Studio Technology plugin format, version 2.4.
     Hosted only by builds made with VST2 support.

   VST3
     The current version of Steinberg's plugin format. |El| hosts VST3
     plugins on every platform and is available as one.

   LV2
     An open plugin format from the Linux audio community, hosted by |El| on
     every platform.

   CLAP
     The CLever Audio Plug-in format, an open standard for instruments and
     effects. |El| hosts CLAP plugins and is available as one.

   GUI
   Graphical User Interface
     The windows, panels and controls of an application.

   API
   Application Program Interface
     The set of functions through which programs talk to each other, for
     example the audio driver API or |El|'s scripting API.

   open source
     Software whose source code is public and may be used and modified under
     its license. |El| is released under the GNU General Public License.

   garbage collector
     The part of the Lua runtime that reclaims memory no longer referenced by
     a script.
