.. include:: /shortcuts.rstext

.. _using-sessions:

Sessions
========

A session is the document |El| works on: its root graphs, tempo, MIDI
mappings and window layout, saved as an ``.els`` file. The
:guilabel:`File` menu (:ref:`interface-menus-and-commands`) holds the usual
commands; this chapter covers what they do and the safety nets around them.

Creating and opening
--------------------

:menuselection:`File --> New Session` (:kbd:`Cmd+N`) starts an empty session
with one graph. If :guilabel:`Default new Session` in the preferences points
at an ``.els`` file, new sessions are copies of that file instead, which is
the way to start every session from your own template of graphs, I/O and
mappings.

:menuselection:`File --> Open Session...` (:kbd:`Cmd+O`) opens an ``.els``
file, and :guilabel:`Open Recent` lists the last few. Double-clicking a
session in the :guilabel:`Data Path` panel, dropping it on the main window,
or passing its path on the command line opens it too. Only one session is
open at a time; opening another closes the current one.

With :guilabel:`Open last used session` enabled (the default), |El| reopens
the previous session at launch.

Saving
------

:menuselection:`File --> Save Session` (:kbd:`Cmd+S`) writes the file,
asking for a name the first time; :guilabel:`Save Session As...`
(:kbd:`Shift+Cmd+S`) writes a copy under a new name. The default folder is
``Sessions`` in your |El| library.

Closing a session or quitting with unsaved changes asks
:guilabel:`Save Session?` with :guilabel:`Save Session`,
:guilabel:`Don't Save` and :guilabel:`Cancel`. Turn :guilabel:`Ask to save
sessions on exit` off in the preferences to skip the question, in which case
|El| saves automatically on exit.

Every change to the session can be undone with :menuselection:`Edit --> Undo`
(:kbd:`Cmd+Z`) and redone with :guilabel:`Redo` (:kbd:`Shift+Cmd+Z`),
including adding and removing nodes, connections and graphs.

Autosave and recovery
---------------------

While a session has unsaved changes, the standalone application writes a
recovery copy every two minutes. For a saved session it sits next to the file
as ``<name>.els.recover``; for a session that has never been saved it is
kept in the application data folder. Saving or closing the session removes
the recovery file.

If |El| did not exit cleanly and a recovery file is newer than the saved
session, the next launch asks :guilabel:`Recover Session?` with
:guilabel:`Recover` and :guilabel:`Discard`. A recovered session is treated
as unsaved, so save it to keep it.

The plugin editions never write recovery files; the host is responsible for
saving.

Safe start
----------

If |El| crashed while opening the last session, the next launch detects it
and asks whether to open that session anyway or to start with an empty one
(:guilabel:`Open Anyway` / :guilabel:`Start Empty`). Choose
:guilabel:`Start Empty` to get past a plugin that fails to load, then
remove or replace the offending node before reopening the session.

Importing and exporting graphs
------------------------------

Graphs travel between sessions as ``.elg`` files.

- :menuselection:`File --> Export graph...` saves the active graph, with all
  its nodes, connections and settings, to an ``.elg`` file. The default
  folder is ``Graphs`` in your |El| library.
- :menuselection:`File --> Import...` adds an ``.elg`` file to the session
  as a new root graph. Dropping the file on the main window or double-clicking
  it in the :guilabel:`Data Path` panel does the same. An invalid file is
  refused with :guilabel:`Invalid graph`.

Exported graphs reference plugins by identifier, so a graph imported on
another computer shows placeholders for plugins that are not installed there.

Session settings
----------------

:menuselection:`View --> Session Properties`, or :guilabel:`Session
Settings...` from the :guilabel:`Session` panel's menu, shows:

:guilabel:`Name`
   The session's name, shown in the window title.

:guilabel:`Tempo`
   The session tempo in beats per minute, from 20 to 999. The tempo box in
   the toolbar edits the same value.

:guilabel:`Notes`
   Free text saved with the session.

Press :kbd:`Esc` to return to the previous view.
