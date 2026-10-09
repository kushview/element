.. include:: /shortcuts.rstext

.. _scripting-examples:

Example Scripts
===============

The scripts below ship with the manual in :file:`docs/manual/examples`. The
first three are also the :guilabel:`Amp` and :guilabel:`Channelizer`
programs of the Script node. :ref:`scripting-script-types` explains the
callbacks they implement.

Amplifier
---------

A DSP script with a stereo input and output and one volume parameter.

.. literalinclude:: /examples/amp.lua
    :caption: amp.lua
    :name: amp-lua
    :language: lua

Amplifier UI
------------

The DSPUI script for the amplifier: a slider bound to the volume parameter.

.. literalinclude:: /examples/ampui.lua
    :caption: ampui.lua
    :name: ampui-lua
    :language: lua

MIDI Filter
-----------

A DSP script that moves every MIDI message to one channel.

.. literalinclude:: /examples/channelize.lua
    :caption: channelize.lua
    :name: midi-filter
    :language: lua

SysEx Sender
------------

A DSP script that sends a System Exclusive message, with a DSPUI script that
provides a button to trigger it.

.. literalinclude:: /examples/sysex.lua
    :caption: sysex.lua
    :name: sysex-lua
    :language: lua

.. literalinclude:: /examples/sysexui.lua
    :caption: sysexui.lua
    :name: sysexui-lua
    :language: lua

Hello World
-----------

An Anonymous script that opens a window containing a custom widget and a
button. Run it from the console with ``script.exec ('helloworld')`` after
copying it to your scripts folder.

.. literalinclude:: /examples/helloworld.lua
    :caption: helloworld.lua
    :name: helloworld-lua
    :language: lua
