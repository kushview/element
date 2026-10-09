.. include:: /shortcuts.rstext

.. _using-nodes-and-plugins:

Nodes and Plugins
=================

A node is one processor in a graph: a plugin loaded from disk or one of
|El|'s built-in nodes. This chapter covers the life of a node, from adding it
to saving its state.

Adding nodes
------------

There are several ways to add a node to the graph in the editor:

- Right-click the graph editor or patch bay background and pick from the
  :guilabel:`Plugins` section: :guilabel:`Favorites`, the scanned plugins by
  manufacturer, |El|'s own nodes under :guilabel:`Element`, and
  :guilabel:`Unverified` plugins found on disk but not yet scanned.
- Drag a plugin from the :guilabel:`Plugins` sidebar panel onto the editor.
  Hold :kbd:`Alt` while dropping to connect it to the graph's inputs,
  :kbd:`Cmd` to connect it to the outputs, or both.
- Drop a plugin file, an ``.eln`` preset or an ``.elg`` graph on the window.
- Double-click a preset in the :guilabel:`Data Path` panel.

A newly added node keeps the plugin's default state, unless a default preset
has been saved for that plugin (below). With :guilabel:`Automatically show
plugin windows` enabled its window opens at once.

Replacing a plugin
------------------

:guilabel:`Replace` on a block's right-click menu lists every plugin; choose
one and the node's plugin is swapped while the block stays in place and
keeps every connection whose port still exists. Use it to try alternatives
without rewiring, or to fix a placeholder.

Placeholders
------------

When a session references a plugin that is not installed, the node is loaded
as a placeholder. It is drawn in bold red in the graph editor and keeps its
name, connections and settings, so the session opens and the rest of the
graph plays. Double-click the block to see why the plugin failed. To resolve
it, install the plugin and reopen the session, use :guilabel:`Replace`, or
drop another plugin onto the block.

Enable, bypass and mute
-----------------------

Every node has three switches, available on the block, in the plugin window
and in the mixer:

Enabled
   :guilabel:`Disable` on the right-click menu unloads the node from the
   engine. It passes nothing and uses no CPU.

Bypass
   The power button. The node stays loaded but its input is copied to its
   output.

Mute
   The :guilabel:`M` button silences the node's output. :guilabel:`Options
   --> Mute input ports` silences what the node receives instead, which lets
   an effect ring out without new input.

All three can be controlled from MIDI (:ref:`using-midi-mapping`).

Processing options
------------------

On the block's right-click menu:

:guilabel:`Options --> Oversample`
   Runs the node at 2, 4 or 8 times the session sample rate, for plugins
   that alias. It costs CPU in proportion.

:guilabel:`Ports...`
   Hides ports you do not use so the block stays tidy. Hidden ports keep
   their connections.

In the :guilabel:`Node` panel (:ref:`interface-node-panel`):

:guilabel:`Delay comp.`
   Adds a fixed delay to the node's output, from -1000 to 1000 ms, on top
   of the plugin's reported latency.

:guilabel:`MIDI Channel`, :guilabel:`Key Start`, :guilabel:`Key End`, :guilabel:`Transpose`
   Filter and shift the MIDI the node receives. Two instruments with
   complementary key ranges make a split; the same range makes a layer.

Bus configuration
-----------------

Plugins that support several input or output buses show a cog button on
their block. It opens :guilabel:`Input Configuration` and :guilabel:`Output
Configuration`, where each bus has a :guilabel:`Bus Name` and a
:guilabel:`Channel Layout`, and buses can be added or removed. Enabling a
side-chain input bus, for example, adds its ports to the block.

Node presets
------------

A node preset is the complete state of a node, plugin parameters included,
saved as an ``.eln`` file in the ``Nodes`` folder of your |El| library.
Presets are listed on the :guilabel:`Presets` submenu of the block's
right-click menu, of the :guilabel:`n` menu in the plugin window, and in the
:guilabel:`Data Path` panel.

:guilabel:`Save node...`
   Saves the node's current state as a preset. The preset is matched to the
   plugin by format and identifier, so it appears only for the same plugin.

:guilabel:`Save as default...`
   Saves the current state as the default for this plugin: every node of
   that plugin added afterwards starts from it. :guilabel:`Reset default...`
   removes the default.

Choosing a preset from the list applies it to the node. Presets can also be
added to a graph as new nodes from the :guilabel:`Data Path` panel or by
dropping the file on the editor.

Factory and native presets
--------------------------

:guilabel:`Presets --> Factory Presets` lists the programs a plugin provides
itself; the current one is ticked. For VST2 plugins, :guilabel:`Presets -->
Native Presets` offers :guilabel:`Save FXB/FXP` and :guilabel:`Load FXB/FXP`
for the plugin's own bank and program file formats.

MIDI programs
-------------

Separately from presets, a node can store up to 128 snapshots that are
recalled by MIDI program change messages, either per session or globally for
the plugin. They are managed in the :guilabel:`Programs` section of the
:guilabel:`Node` panel (:ref:`interface-node-panel`).

Duplicating and removing
------------------------

:guilabel:`Duplicate` on the right-click menu adds a copy of the node with
the same state but no connections. :guilabel:`Remove`, or :kbd:`Delete`
with the block selected, removes the node and its connections. Both can be
undone.
