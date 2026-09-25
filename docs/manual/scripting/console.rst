.. include:: /shortcuts.rstext

.. _scripting-console:

The Lua Console
===============

The console is an interactive Lua prompt inside |El| with the running
session in reach. Open it with :menuselection:`View --> Console`
(:kbd:`F3`); it appears in the accessory view under the main view.

.. SCREENSHOT scripting/console-01: the console in the accessory view with a
   few commands and their results
.. .. figure:: /images/scripting/console-01.png
..    :alt: The Lua console
..
..    The console.

Type a Lua expression or statement and press :kbd:`Enter`. The result of an
expression is printed; ``print`` and ``console.log`` output goes to the
console too, as do script errors and |El|'s own log messages.

Editing keys
   :kbd:`Up` and :kbd:`Down` step through the command history;
   :kbd:`Ctrl+A` and :kbd:`Ctrl+E` jump to the start and end of the line.

The environment
---------------

The console keeps its variables and history while it is closed and
reopened. When it first opens, a start-up script loads these globals:

``session()``
   Returns the active session. It is a function rather than a value because
   the session object is replaced when another session is loaded; call it
   each time rather than storing the result.

``console.log (...)``
   Prints its arguments, converted with ``tostring``.

``clear()``
   Clears the console.

``object``, ``command``, ``script``, ``Context``
   The ``el.object``, ``el.command``, ``el.script`` and ``el.Context``
   modules, already required.

For example, to read and set the tempo::

   > session().tempo
   120
   > session().tempo = 96

and to run a bundled or user script by name::

   > script.exec ('helloworld')

Anything else is a ``require`` away; see :ref:`scripting-overview` for the
module list and the `scripting API`_ for their methods.
