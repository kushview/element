MIDI Clock
==========
Element respond to and generate MIDI Clock messages.

**Receiving Clock**

    To slave to a clock source, first you need to enable it in Preferences -> General.  
    Under the Clock Source option, choose "MIDI Clock."  After doing this, Element will 
    sync tempo to the clock master.  Element will also respond to MIDI Start, stop and 
    continue messages sent from the master.

    EXT button, to disable clock sync, you can do so by turning off the "EXT" button 
    found in the toolbar.

**Sending Clock**

    To be the MIDI clock master, open the ``Preferences -> MIDI page``, and turn on the 
    "Generate MIDI Clock" option.

    The default behavior sends clocks to the MIDI output device.  Turning on Send Clock 
    to MIDI input, will instead send clocks to the MIDI input.  The latter allows routing 
    clocks to plugins on your graphs.

    MIDI start, stop, and continue will also be generated when the transport is started 
    and stopped.
