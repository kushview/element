.. include:: /shortcuts.rstext

.. _part-scripting:

Scripting
=========

|El| embeds a Lua interpreter. Scripts extend it in three ways: a DSP script
runs inside a Script node as a processor in a graph, a DSPUI script provides
that node's editor, and the console runs Lua against the live session. This
part covers how scripts are written, where they live, and how they are used.
The functions and classes available to a script are documented separately in
the `scripting API`_ reference.

Some familiarity with Lua is assumed, but the bundled examples are short and
are a good place to start. A script can be edited in |El| itself, from the
:guilabel:`DSP` and :guilabel:`UI` children of a Script node in the
:guilabel:`Session` panel.

:doc:`overview`
   The script types, the descriptor table every script returns, where
   scripts live on disk, the ``el`` modules, and the bundled scripts.

:doc:`script-types`
   The callbacks of the DSP, DSPUI, Content, View and anonymous script
   types.

:doc:`script-node`
   The node that runs a DSP script as a processor, and how its ports and
   parameters come from the script's layout.

:doc:`console`
   The interactive Lua prompt with the running session in reach.

:doc:`examples`
   The scripts that ship with the manual: an amplifier and its UI, a MIDI
   filter, a SysEx sender and a hello world.

.. toctree::
   :maxdepth: 2

   overview
   script-types
   script-node
   console
   examples
