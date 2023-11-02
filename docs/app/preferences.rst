Preferences
===========

Preferences are the main options Element, and can be accessed
through the main menu ``File -> Preferences`` (macOS ``Element -> Preferences``).
Alternatively, some items are available directly in the ``Options`` main menu
as well.

General
-------
=========================================== ==============================================
Option                                      Description
=========================================== ==============================================
**Clock Source**                            Sets the synchronization mode of the audio engine.

                                            - *Internal*: Use the system time base
                                            - *MIDI Clock*: Sync Tempo with incoming from MIDI input.
**Check for updates on startup**            Runs an update check when Element is launched.
**Scan plugins at startup**                 Runs a plugin scan when Element is launched in the background.
**Automatically show plugin windows**       If enabled, will cause plugin windows to show immediately 
                                            after a plugin is added to a graph.
**Plugin windows on top by default**        If enabled, keeps plugin windows on top of others
                                            by default when first shown.
**Hide plugin windows when app inactive**   Hides plugin windows when application focus is lost.
**Open last used Session**                  Setting this to yes causes Element to open the 
                                            last opened Session from disk. Disabling it causes 
                                            a new session to be created on start up.
**Ask to save sessions on exit**            If enabled, ask to save modifed sessions. When
                                            disabled the the current session will automatically save.
**UI Type**                                 The type of main UI to use. (*not in use*)
**Show system tray**                        If enabled, show the application system tray icon.
**Desktop scale**                           Increase or decrease to change the overall 'zoom' of
                                            the application. 1.0 equates to no scaling.
**Default new Session**                     If set, use this session as a template when creating
                                            new sessions.
=========================================== ==============================================    

Audio
-------
=========================================== ==============================================
Option                                      Description
=========================================== ==============================================
**Audio device type**                       The type of device or driver. e.g. JACK, ASIO 
                                            or CoreAudio
**Output**                                  Device to use for audio input.
**Input**                                   Device to use for audio output.
**Active output channels**                  Enable/disable specific device channels
**Active input channels**                   Enable/disable specific device channels
**Sample rate**                             The audio sampling rate to use in th engine.
**Audio buffer size**                       Number of samples to use per processing cycle
**Control Panel**                           If using ASIO, this button will open the 
                                            driver's control panel.
=========================================== ==============================================
MIDI
-------

:MIDI Output Device:
    The device chosen here is used as the Global MIDI output

:MIDI Input Devices:
    Devices enabled here aggregate MIDI to the Global MIDI input

OSC
---
:OSC Host Enabled?:
    Whether to use OSC should be used or not

:OSC Host:
    The host address to serve on

:OSC Host Port:
    The port to run on
