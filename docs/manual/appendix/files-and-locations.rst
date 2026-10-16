.. include:: /shortcuts.rstext

.. _appendix-files-and-locations:

File Types and Locations
========================

File types
----------

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Extension
     - Contents
   * - ``.els``
     - A session: graphs, tempo, mappings and layout. XML.
   * - ``.els.recover``
     - The autosave copy of a session, next to the ``.els`` file.
   * - ``.elg``
     - A single graph exported from, or imported into, a session.
   * - ``.eln``
     - A node preset. ``.elpreset`` and ``.elp`` are older names that are
       still read.
   * - ``.fxb``, ``.fxp``
     - VST2 bank and program files, loaded and saved from a VST2 node's
       :guilabel:`Native Presets` menu.
   * - ``.lua``
     - A script.

Dropping any of the first four on the main window opens or adds it; ``.els``
and ``.elg`` files can also be passed on the command line.

The user library
----------------

Your own files go in the ``Element`` folder inside your Music folder
(:file:`~/Music/Element` on macOS and Linux, :file:`Music\\Element` in your
user profile on Windows). The :guilabel:`Data Path` panel browses it. Its
folders:

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Folder
     - Contents
   * - ``Sessions``
     - The default location for ``.els`` files.
   * - ``Graphs``
     - The default location for exported ``.elg`` files.
   * - ``Nodes``
     - Node presets (``.eln``).
   * - ``Scripts``
     - Your Lua scripts; the first place |El| looks for them.
   * - ``Controllers``
     - Reserved for controller definitions.

Settings and application data
-----------------------------

The settings file and the application data folder around it live in the
per-user configuration area:

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Platform
     - Location
   * - macOS
     - :file:`~/Library/Application Support/Kushview/Element/`
   * - Windows
     - :file:`%APPDATA%\\Kushview\\Element\\`
   * - Linux
     - :file:`~/.config/Kushview/Element/`

In that folder:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - File or folder
     - Contents
   * - ``Element.conf``
     - Every setting from the preferences, the audio and MIDI device setup,
       key mappings, plugin search paths, the last session, and the
       updater's state. Debug builds use ``Element_Debug.conf``.
   * - ``plugins.xml``, ``plugin-metadata.xml``
     - The scanned plugin list, and favourites and hidden plugins.
   * - ``recents.txt``
     - The :guilabel:`Open Recent` list.
   * - ``log/``
     - Log files, opened by :menuselection:`Help --> Log files...`:
       ``main.log`` for the application and ``scanner.log`` for the plugin
       scanner.
   * - ``scanner/crashed.txt``
     - Plugins that crashed the scanner and are blacklisted.
   * - ``nodes/<plugin>/default.eln``
     - Default presets saved with :guilabel:`Save as default...`.
   * - ``cache/midi/programs/``
     - Global MIDI programs of nodes.
   * - ``recovery/untitled.els.recover``
     - The autosave of a session that has never been saved.
   * - ``Scripts/``, ``Modules/``
     - Additional scripts and Lua modules.

Plugin locations
----------------

|El|'s own plugin editions, and the default search paths for hosted plugins,
are the platform's standard folders:

.. list-table::
   :header-rows: 1
   :widths: 15 85

   * - Platform
     - Folders
   * - macOS
     - :file:`/Library/Audio/Plug-Ins/{Components,VST,VST3,CLAP,LV2}` and the
       same under :file:`~/Library`.
   * - Windows
     - :file:`C:\\Program Files\\Common Files\\{VST3,CLAP,LV2}`, the
       :file:`Steinberg\\VstPlugins` folders, and
       :file:`%LOCALAPPDATA%\\Programs\\Common\\CLAP`.
   * - Linux
     - :file:`~/.vst3`, :file:`~/.clap`, :file:`~/.lv2`,
       :file:`/usr/local/lib/{vst3,clap,lv2}` and :file:`/usr/lib/{vst3,clap,lv2}`.

Search paths are edited from the plugin manager (:ref:`using-plugin-manager`).

Command line
------------

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Argument
     - Effect
   * - ``<file>.els``
     - Open the session. A relative path is resolved from the current
       directory.
   * - ``<file>.elg``
     - Import the graph into the current session.
   * - ``--hidden``
     - Start with the window hidden in the system tray.

If |El| is already running, the arguments are passed to the running instance.
