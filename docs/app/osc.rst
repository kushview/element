OSC
#####
Element can be controlled by OSC.

Engine Control
--------------
=========================================== ==============================================
Command                                     Description
=========================================== ==============================================
``/element/engine/samplerate/:rate``        Change the sample rate
=========================================== ==============================================

Application Commands
--------------------
=========================================== ==============================================
Command                                     Description
=========================================== ==============================================
``/element/command/quit``                     Quit the application 
``/element/command/undo``                     Undo the last action 
``/element/command/redo``                     Redo the last undone action 
``/element/command/showAbout``                Show the about window 
``/element/command/showLegacyView``           Show legacy view when workspaces are enabled 
``/element/command/showPluginManager``        Show the plugin manager 
``/element/command/showPreferences``          Show the preferences dialog 
``/element/command/showSessionConfig``        Show the current session's config view 
``/element/command/showGraphConfig``          Show the current graph configuration 
``/element/command/showPatchBay``             Show the patch bay 
``/element/command/showGraphEditor``          Show the graph editor 
``/element/command/showLastContentView``      Show the previous main view 
``/element/command/showAllPluginWindows``     Show all plugin windows 
``/element/command/hideAllPluginWindows``     Hides all plugin windows 
``/element/command/showKeymapEditor``         Show the keymaps editor 
``/element/command/toggleVirtualKeyboard``    Toggle the virtual keyboard 
``/element/command/rotateContentView``        Switch to the next content view 
``/element/command/showControllerDevices``    Show the controllers editor 
``/element/command/toggleUserInterface``      Show or hide the whole user interface 
``/element/command/toggleChannelStrip``       Toggles the Node channel strip 
``/element/command/showGraphMixer``           Show the graph mixer 
``/element/command/panic``                    Triggers a Panic message via MIDI 
``/element/command/graphNew``                 Create a new graph 
``/element/command/graphOpen``                Open a graph 
``/element/command/graphSave``                Save the current graph 
``/element/command/graphSaveAs``              Save the current graph as 
=========================================== ==============================================

OSC Receiver/Sender Node
------------------------
============================= =========================================================== =================
Command                       Values                                                      Description   
============================= =========================================================== =================
``/midi/noteOn``              `int` channel, `int` note number, `float` velocity          Note on 
``/midi/noteOff``             `int` channel, `int` note number, `float` velocity          Note off 
``/midi/programChange``       `int` channel, `int` program number                         Program change 
``/midi/pitchBend``           `int` channel, `int` position  Pitch bend 
``/midi/afterTouch``          `int` channel, `int` note number, `int` aftertouch amount   Aftertouch 
``/midi/channelPressure``     `int` channel, `int` pressure                               Channel pressure 
``/midi/controlChange``       `int` channel, `int` controller type, `int`  value          Control change 
``/midi/allNotesOff``         `int` channel                                               All notes off 
``/midi/allSoundOff``         `int` channel                                               All sound off 
``/midi/allControllersOff``   `int` channel                                               All controllers off 
``/midi/start``               N/A                                                         Transport start 
``/midi/continue``            N/A                                                         Transport continue 
``/midi/stop``                N/A                                                         Transport stop 
``/midi/clock``               N/A                                                         MIDI clock 
``/midi/songPositionPointer`` `int` position in MIDI beats                                Song position pointer 
``/midi/activeSense``         N/A                                                         Active sense 
============================= =========================================================== =================
