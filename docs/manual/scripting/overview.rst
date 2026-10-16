.. include:: /shortcuts.rstext

.. _scripting-overview:

Overview
========

|El| embeds a Lua interpreter. Scripts extend it in three ways: a **DSP
script** runs inside a Script node as a processor in a graph, a **DSPUI
script** provides that node's editor, and the **console** runs Lua against
the live session. The scripting API is documented separately in the
`scripting API`_ reference; this part covers how scripts are written, found
and used.

Script types
------------

Every script declares its type in its header. The types are:

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Type
     - Purpose
   * - ``DSP``
     - Audio and MIDI processing inside a Script node.
   * - ``DSPUI``
     - The editor of a DSP script, built from |El|'s widget classes.
   * - ``Content``
     - A widget shown as a main view (experimental).
   * - ``View``
     - A widget shown in the user interface (experimental).
   * - ``Anonymous``
     - Anything else: a script run from the console or by another script,
       whose behaviour is up to its author.

:ref:`scripting-script-types` describes what each type must return.

Defining a script
-----------------

A script is a Lua file that starts with an `ldoc <https://stevedonovan.github.io/ldoc/>`_
style header comment and returns a descriptor table. The header gives the
script's title, a description and a set of tags:

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Tag
     - Meaning
   * - ``@script``
     - The script's identifier. Required.
   * - ``@type``
     - One of the types above. Required.
   * - ``@author``
     - Who wrote it.
   * - ``@license``
     - License information.

The title is the first line, ending with a period; the description is the
free text after it.

.. code-block:: lua

   --- Script Title.
   --
   -- The script's description.
   --
   -- @script      com.example.script
   -- @type        DSP
   -- @license     GPL v3
   -- @author      Example Author

   ...

   return {
       type = "DSP",
       -- callbacks
   }

Where scripts live
------------------

|El| looks for scripts in these folders, in order:

#. ``Scripts`` in your |El| library (the ``Element`` folder in your Music
   folder). This is where your own scripts go, and where the Script node's
   file chooser opens.
#. ``Scripts`` in the application data folder
   (:ref:`appendix-files-and-locations`).
#. The scripts shipped with |El|: :file:`Element.app/Contents/Resources/Scripts`
   on macOS, :file:`share/element/scripts` under the install prefix on
   Linux, :file:`Scripts` next to the executable on Windows. On Linux and
   macOS :file:`~/.local/share/element/scripts` is searched as well.

Lua modules loaded with ``require`` are searched in ``Modules`` in the
application data folder and in the modules shipped with |El|
(:file:`Contents/Resources/Modules`, :file:`share/element/modules`, or
:file:`lua` next to the executable on Windows). The ``el.*`` modules below are
built into |El| and need no files.

Three environment variables override the search: ``ELEMENT_SCRIPTS_PATH``
adds script folders, and ``LUA_PATH`` and ``LUA_CPATH`` extend the module
search path in the usual Lua way.

The ``el`` modules
------------------

Scripts reach |El| through modules named ``el.<name>``:

Application
   ``el.Context`` (the entry point: ``Context.instance()`` gives the
   running application), ``el.Session``, ``el.Node``, ``el.Graph``,
   ``el.Commands`` and ``el.command``, ``el.script``, ``el.object``,
   ``el.session``, ``el.strings``, ``el.color``.

Audio and MIDI
   ``el.audio``, ``el.midi``, ``el.bytes``, ``el.round``,
   ``el.AudioBuffer`` (``el.AudioBuffer32`` and ``el.AudioBuffer64``),
   ``el.MidiMessage``, ``el.MidiBuffer``, ``el.MidiPipe``.

User interface
   ``el.Widget``, ``el.View``, ``el.Content``, ``el.DocumentWindow``,
   ``el.TextButton``, ``el.Slider``, ``el.Graphics``, ``el.Point``,
   ``el.Range``, ``el.Rectangle``, ``el.Bounds``, ``el.Desktop``,
   ``el.MouseEvent``, ``el.GraphEditor``, ``el.File``.

Most of them are also available under the older ``kv.`` prefix. Each module
is documented in the `scripting API`_ reference, which is generated from the
sources.

Bundled scripts
---------------

The Script node's factory programs are scripts shipped inside |El|:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Script
     - What it does
   * - ``amp`` and ``ampui``
     - A stereo amplifier with a volume parameter, and its editor.
   * - ``channelize``
     - Forces all MIDI onto one channel; channel 0 passes it through.
   * - ``dial``
     - Outputs a normalised 0 to 1 control value from a parameter.
   * - ``midicc``
     - Turns a 0 to 1 value into a MIDI CC message.
   * - ``mtc_generator``
     - Generates MIDI Timecode from the transport.
   * - ``spontonchordchooser``
     - Sends Spoton scale settings over CC.
   * - ``testtone``
     - A sine wave generator.
   * - ``tremolo``
     - A sine LFO tremolo.

The :ref:`scripting-examples` chapter reproduces several of them in full.
