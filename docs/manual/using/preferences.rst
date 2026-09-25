.. include:: /shortcuts.rstext

.. _using-preferences:

Preferences
===========

The preferences window holds the application-wide settings. Open it with
:menuselection:`File --> Preferences..` (:menuselection:`Element -->
Preferences...` on macOS) or :kbd:`Cmd+,`. The pages are listed on the left:
:guilabel:`General`, :guilabel:`Audio`, :guilabel:`MIDI`, :guilabel:`OSC`
and, in builds with the updater, :guilabel:`Updates`. The most common
settings are also on the :guilabel:`Options` menu
(:ref:`interface-menus-and-commands`).

Settings take effect immediately and are stored in the settings file
(:ref:`appendix-files-and-locations`).

General
-------

.. figure:: /images/using/preferences-00.png
   :alt: The General page

   The General page.

:guilabel:`Clock Source`
   :guilabel:`Internal` runs the transport on its own clock;
   :guilabel:`MIDI Clock` follows incoming MIDI clock and shows the
   :guilabel:`EXT` toolbar button. See :ref:`using-transport-and-clock`.

:guilabel:`Check for updates on startup`
   Checks for a new version shortly after launch (updater builds only).

:guilabel:`Scan plugins on startup`
   Runs a plugin scan in the background each time |El| starts.

:guilabel:`Automatically show plugin windows`
   Opens a plugin's window as soon as it is added to a graph.

:guilabel:`Plugin windows on top by default`
   New plugin windows start with :guilabel:`^` (keep on top) enabled.

:guilabel:`Hide plugin windows when app inactive`
   Hides plugin windows while another application is in front. On by
   default except on Linux.

:guilabel:`Open last used session`
   Reopens the previous session at launch. When off, |El| starts with a new
   session.

:guilabel:`Ask to save sessions on exit`
   Asks before discarding unsaved changes. When off, the session is saved
   automatically on exit.

:guilabel:`Show system tray`
   Adds |El| to the system tray or menu bar. On by default except on Linux.

:guilabel:`Start hidden in system tray`
   Starts with the main window hidden. Available when the tray is enabled.

:guilabel:`Desktop scale`
   Scales the whole interface, from 0.1 to 8.0; 1.0 is unscaled.

:guilabel:`Default new Session`
   An ``.els`` file used as the template for :guilabel:`New Session`.
   The :guilabel:`X` button clears it.

Audio
-----

.. figure:: /images/using/preferences-01.png
   :alt: The Audio page

   The Audio page.

The device selector chooses the :guilabel:`Audio device type` (CoreAudio on
macOS; WASAPI, DirectSound or ASIO on Windows; ALSA or JACK on Linux), the
:guilabel:`Output` and :guilabel:`Input` devices, the active channels of
each, the :guilabel:`Sample rate` and the :guilabel:`Audio buffer size`. A
smaller buffer lowers latency and raises CPU load. With ASIO a
:guilabel:`Control Panel` button opens the driver's own settings.

If the device disappears, |El| keeps it selected, shows
:guilabel:`Disconnected` in the status bar, and reopens it when it returns.

:guilabel:`Experimental --> Multithreaded Rendering`
   Renders root graphs across several CPU cores. Off by default; the
   number field sets the worker threads, from 2 to 16. Try it for sessions
   with several heavy graphs in :guilabel:`Parallel` rendering mode.

MIDI
----

.. figure:: /images/using/preferences-02.png
   :alt: The MIDI page

   The MIDI page.

:guilabel:`MIDI Output Device`
   The device that receives the root graph's :guilabel:`MIDI Out` node and
   generated MIDI clock, or :guilabel:`<< none >>`.

:guilabel:`Output latency (ms)`
   Delays MIDI output by -1000 to 1000 ms. Not available on Windows.

:guilabel:`Generate MIDI Clock`
   Sends MIDI clock and Start/Stop/Continue to the output device.

:guilabel:`Send Clock to MIDI Input?`
   Feeds the generated clock into the root graphs instead.

:guilabel:`Transport: MIDI Start/Stop`
   Makes the transport follow incoming MIDI Start, Stop and Continue.

:guilabel:`MIDI Panic CC`
   An enable switch, a channel (0 for any) and a CC number. Receiving that
   CC sends all-notes-off to every node.

:guilabel:`Active MIDI Inputs`
   An :guilabel:`On` / :guilabel:`Off` switch for every MIDI input. Enabled
   inputs feed the root graph's :guilabel:`MIDI In` node and the MIDI
   mappings. A remembered input that is unplugged stays enabled and resumes
   when it returns.

OSC
---

.. figure:: /images/using/preferences-03.png
   :alt: The OSC page

   The OSC page.

:guilabel:`OSC Host Enabled?`
   Runs the OSC server.

:guilabel:`OSC Host`
   The address clients send to (read-only).

:guilabel:`OSC Host Port`
   The UDP port, 9000 by default.

See :ref:`using-osc` for what the host accepts.

Updates
-------

.. figure:: /images/using/preferences-04.png
   :alt: The Updates page

   The Updates page.

:guilabel:`Release Channel`
   :guilabel:`Stable` for tested releases, or :guilabel:`Preview` for
   builds ahead of the next release.

:guilabel:`Preview Access`
   Shows :guilabel:`Not authorized` or :guilabel:`Authorized as:` followed
   by your account. :guilabel:`Sign in with Kushview.net` opens the browser
   to sign in; :guilabel:`Sign Out` forgets the account. The preview channel
   requires an account with preview access.

Updates are checked at startup when :guilabel:`Check for updates on startup`
is on, and on demand from :guilabel:`Check For Updates`. See
:ref:`getting-started-installation`.
