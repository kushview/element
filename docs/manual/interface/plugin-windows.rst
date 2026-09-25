.. include:: /shortcuts.rstext

.. _interface-plugin-windows:

Plugin Windows
==============

Every node except the graph's input and output nodes can open a window of its
own showing the plugin's editor, or a generic parameter editor for plugins
without one. Open it by double-clicking the block in the graph editor, the
node's icon in the session tree, or a row in the patch bay.

.. SCREENSHOT interface/plugin-windows-01: a plugin window with the toolbar
   buttons visible, over the main window
.. .. figure:: /images/interface/plugin-windows-01.png
..    :alt: A plugin window
..
..    A plugin window.

The window uses the native title bar, can be minimised, and can be resized
when the plugin allows it. Its title is the node's name. Above the editor is
a small toolbar:

- a power button that bypasses the node;
- :guilabel:`^`, which keeps the window on top of others;
- :guilabel:`n`, which opens the node menu (the same items as the block menu
  in :ref:`interface-graph-editor`, including :guilabel:`Options` and
  :guilabel:`Presets`).

Each node remembers where its window was last placed and whether it was on
top, and the window reopens there next time, even in a later session.

Managing many windows
---------------------

The :guilabel:`Window` menu closes or opens every plugin window of the active
graph at once (:kbd:`Cmd+Alt+W` and :kbd:`Shift+Cmd+Alt+W`), and
:guilabel:`Reset plugin windows` gathers windows that ended up outside the
screen, for example after changing monitors.

Three preferences shape the behaviour of plugin windows:

- :guilabel:`Automatically show plugin windows` opens a window as soon as a
  plugin is added to a graph.
- :guilabel:`Plugin windows on top by default` sets the initial state of the
  :guilabel:`^` button.
- :guilabel:`Hide plugin windows when app inactive` hides them when another
  application comes to the front and shows them again when |El| regains
  focus.

To keep a plugin's editor permanently in view without a window, use the
:guilabel:`Embed` display mode of its block, or the :guilabel:`Editor` panel
in the sidebar.
