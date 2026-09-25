.. include:: /shortcuts.rstext

.. _part-nodes:

Node Reference
==============

Alongside the plugins you install, |El| ships with a set of built-in nodes.
They are listed under :guilabel:`Element` in the add-node menus and are
available in every build. This part describes each of them: what it does,
its ports and parameters, and its custom editor where it has one.

The chapters group the nodes by what they process. Adding, connecting and
configuring nodes in general, which works the same for plugins and built-in
nodes, is covered in :doc:`/using/nodes-and-plugins`, and MIDI mapping of
their parameters in :doc:`/using/midi-mapping`.

:doc:`audio`
   Nodes that process, route or play audio: gain, filters, a reverb, a
   compressor, a mixer and router, and the audio file and media players.

:doc:`midi`
   Nodes that filter, route and generate MIDI: channel tools, a router,
   program maps, the set list, the monitor, and the MIDI device nodes.

:doc:`utility`
   The graph's input and output nodes, the Graph node that holds a subgraph,
   the Placeholder, the OSC nodes and the Script node.

.. toctree::
   :maxdepth: 2

   audio
   midi
   utility
