Setting A Graph's MIDI Channel
==============================
To change the MIDI channel of a Graph, follow these steps:

#. Right click the Graph in the Navigation pane
#. Choose "Settings" from the menu
#. Choose the MIDI channel from the properties list.

Graph settings can also be accessed from the main menu (View -> Graph Settings)

Switching Graphs
================

The active graph can be changed by clicking on it, or by sending a MIDI program 
change.

**Switch Graphs with a MIDI Program Change**

    It is possible to change graphs with MIDI program changes by setting the MIDI 
    Program property of a root graph. To change the MIDI program, follow these steps:

    #. Right click the Graph in the Navigation pane
    #. Choose "Settings" from the menu
    #. Change the MIDI Program from the properties list.

    MIDI Program of 0 disables listening for program changes, while program 1-128 
    enables it for the chosen program* Graph settings can also be accessed from the 
    main menu (View -> Graph Settings)

Velocity Curves
===============
Velocity curves can be added to Graphs in order to filter MIDI input velocities.  
Use it to adjust the sensitivity of your input device.

**Changing Velocity Curves**

    Go to the graph's settings and select the desired mode from the dropdown menu.

**Velocity Curve Modes**
    
    =========== =====================================
    Mode        Description
    =========== =====================================
    Linear      Pass thru, no change to velocity
    Soft        For heavy hands. Curve velocity down
    Softer      More intense variation of Soft
    Softest     More intense variation of Softer
    Hard        For light hands. Curve velocity up
    Harder      More intense variation of Hard
    Hardest     More intense variation of Harder
    Max         Output is always max velocity (127)
    =========== =====================================