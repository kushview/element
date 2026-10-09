.. include:: /shortcuts.rstext

.. _getting-started-concepts:

Core Concepts
=============

Everything in |El| is arranged in a small hierarchy: a session holds graphs,
a graph holds nodes, and nodes are wired together through their ports. This
chapter defines those terms as the rest of the manual uses them.

.. SCREENSHOT getting-started/concepts-01: the Session panel expanded to show
   a session with two root graphs, one expanded to show its nodes and a
   subgraph
.. .. figure:: /images/getting-started/concepts-01.png
..    :alt: A session with two graphs in the session tree
..
..    A session with two root graphs in the session tree.

Session
-------

A :term:`session` is the document you open and save. It is stored as an
``.els`` file and contains:

- one or more root graphs, in the order shown in the session tree;
- which graph is active;
- the tempo and time signature, and whether the transport follows an external
  clock;
- the MIDI mappings (:ref:`using-midi-mapping`);
- a free-form :guilabel:`Notes` field;
- the layout state of the window panels and each graph's editor.

Only one session is open at a time. :ref:`using-sessions` covers creating,
saving, autosave and recovery.

Graph
-----

A :term:`graph` is a set of nodes and the connections between them. The graphs
listed directly under the session are **root graphs**. Each root graph has its
own audio and MIDI input and output nodes, which stand for the audio device
and the enabled MIDI devices. A root graph also has settings that decide how
it responds to MIDI: the channels it listens on, an optional program number
that activates it, and a velocity curve.

The **active graph** is the one that renders sound and receives MIDI. Switch it
by double-clicking a graph in the session tree, by pressing the graph's
hotkey, or by sending the MIDI program change assigned to it. A root graph's
:guilabel:`Rendering Mode` decides what happens to the others: in
:guilabel:`Single` mode only the active graph renders, and a graph that goes
to the background is allowed to finish its tail before it stops; in
:guilabel:`Parallel` mode all root graphs render all the time.

A graph placed inside another graph is a **subgraph**. It appears as a single
block with its own inputs and outputs, and double-clicking it opens its
contents in the editor. Subgraphs can be nested to any depth.
:ref:`using-graphs` covers graph settings, switching and subgraphs.

Node
----

A :term:`node` is one processor in a graph. It is either a plugin loaded from
disk (AU, VST, VST3, LV2 or CLAP) or one of |El|'s built-in nodes
(:ref:`part-nodes`). Every node has:

- a name you can change;
- :guilabel:`Enabled` and :guilabel:`Bypass` states and a :guilabel:`Mute`;
- MIDI settings: the channels it accepts, a key range and a transpose amount,
  which together make keyboard splits and layers possible;
- an optional delay compensation and oversampling factor;
- parameters, which can be automated from MIDI mappings, and in the plugin
  editions from the host.

A node whose plugin cannot be found on the current computer is loaded as a
**placeholder**: it keeps its connections and settings so the session still
loads, and you can replace it with the real plugin later.

Ports and connections
---------------------

Nodes exchange data through **ports**. There are three kinds, drawn in
different colours in the graph editor: audio (green), MIDI (orange) and
control (blue). A **connection** links an output port to an input port of the
same kind. One output may feed many inputs, and one input may receive many
outputs; audio inputs sum their sources.

You make connections by dragging between ports in the graph editor or by
clicking cells in the patch bay. The two views show the same graph.

Presets and programs
--------------------

A **node preset** is the complete state of a node saved to a file in the
``Nodes`` folder of your |El| library, ready to be added to any graph or
dropped onto the editor. A node can also be given a **default preset** that is
applied whenever that plugin is added.

Separately, a node can hold up to 128 **MIDI programs**: snapshots of its
state that are recalled by MIDI program change messages. Plugins that expose
their own factory programs list them as well, and VST2 plugins can load and
save FXB and FXP files. :ref:`using-nodes-and-plugins` explains each of these.

Devices and the engine
----------------------

The audio device and the set of enabled MIDI inputs are global settings
(:ref:`using-preferences`). The active root graph's :guilabel:`Audio In`,
:guilabel:`Audio Out`, :guilabel:`MIDI In` and :guilabel:`MIDI Out` nodes
connect to them. To address one specific MIDI port instead, add a
:guilabel:`MIDI Input Device` or :guilabel:`MIDI Output Device` node.

The engine runs at the sample rate and buffer size of the audio device. Plugin
latency and per-node delay compensation are taken into account so that
parallel paths stay aligned.

Scripts
-------

|El| embeds Lua. A **Script node** runs a DSP script as a processor in a
graph, optionally with its own editor written in Lua; the console lets you
inspect and change the running session. :ref:`part-scripting` covers this.

Files
-----

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Extension
     - Contents
   * - ``.els``
     - A session
   * - ``.elg``
     - A single graph, exported from or imported into a session
   * - ``.eln``
     - A node preset (``.elpreset`` and ``.elp`` are older names still read)
   * - ``.lua``
     - A script

:ref:`appendix-files-and-locations` lists where each kind lives on every
platform.
