.. include:: /shortcuts.rstext

.. _getting-started-installation:

Installation
============

Pre-built packages for macOS, Windows and Linux are published on the
`Element downloads`_ page. Public builds are free; supporters and members get
the newest stable and preview builds through the built-in updater. You can
also build |El| yourself, see :ref:`part-developers`.

Each package contains the standalone application and the plugin editions
(:ref:`plugin-editions`).

macOS
-----

Copy :file:`Element.app` to your :file:`Applications` folder. The plugin
editions install into the standard locations under
:file:`/Library/Audio/Plug-Ins`:

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Format
     - Folder
   * - AU
     - :file:`/Library/Audio/Plug-Ins/Components`
   * - VST3
     - :file:`/Library/Audio/Plug-Ins/VST3`
   * - CLAP
     - :file:`/Library/Audio/Plug-Ins/CLAP`
   * - LV2
     - :file:`/Library/Audio/Plug-Ins/LV2`

Hosts that scan the user folders under :file:`~/Library/Audio/Plug-Ins` find
the plugins there too. |El| requires macOS 10.13 or later.

To uninstall, run :file:`Uninstall.app` from the download. It removes
:file:`Element.app` from :file:`/Applications` and :file:`~/Applications` and
every ``KV-Element`` plugin bundle from both plug-in folders, after asking for
an administrator password. Your sessions, presets and settings are left in
place.

Windows
-------

Run the installer, or unpack the archive, and start :file:`Element.exe`. The
plugin editions go into the common plugin folders:

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Format
     - Folder
   * - VST3
     - :file:`C:\\Program Files\\Common Files\\VST3`
   * - CLAP
     - :file:`C:\\Program Files\\Common Files\\CLAP`
   * - LV2
     - :file:`C:\\Program Files\\Common Files\\LV2`

|El| requires Windows 10 or later. ASIO drivers are supported for low-latency
audio; see :ref:`using-preferences`.

Linux
-----

Install the ``.deb`` package with your package manager, for example:

.. code-block:: console

   $ sudo apt install ./element-1.2.0-linux-x86_64.deb

The package installs the ``element`` command, a desktop entry in the
Audio/MIDI category that opens ``.els`` and ``.elg`` files, the plugin
editions under :file:`lib/clap`, :file:`lib/lv2` and :file:`lib/vst3` of the
install prefix, and this manual under :file:`share/doc/element/manual`.
Ubuntu is the most tested distribution.

On other distributions build from source: :ref:`part-developers` lists the
packages needed on Debian, Ubuntu and Arch Linux.

The first launch
----------------

On its first run |El| records the default plugin search path for every
format it supports and looks for plugins on disk without scanning them; these
appear under :guilabel:`Unverified` in the plugin menus until you run a scan.
It does not check for updates on the first run. Start with the
:ref:`getting-started-quick-start` to set up audio, scan plugins and play
something.

Settings are kept in a per-user file, and sessions, presets and scripts in an
|El| folder inside your Music folder. :ref:`appendix-files-and-locations`
lists the exact paths for each platform.

Updates
-------

Builds that include the updater check for a new version shortly after every
launch, and on demand with :menuselection:`Element --> Check For Updates...`
(macOS) or :menuselection:`File --> Check For Updates..` (Windows and Linux).
Turn the automatic check off with :guilabel:`Check for updates on startup` on
the :guilabel:`General` page of the preferences.

The :guilabel:`Updates` page of the preferences chooses the
:guilabel:`Release Channel`:

Stable
   Tested releases.

Preview
   Builds ahead of the next release. Preview requires signing in with a
   Kushview.net account that has preview access: click
   :guilabel:`Sign in with Kushview.net`, finish the sign-in in your browser,
   and |El| shows :guilabel:`Authorized as:` followed by your account.
   :guilabel:`Sign Out` returns to the stable channel.

An update is downloaded and installed by the platform's update framework
(Sparkle on macOS, WinSparkle on Windows). On Linux install the new package
the same way as the first one.

Running as a plugin
-------------------

Rescan plugins in your DAW after installing. The three plugin editions appear
under the manufacturer name Kushview as ``Element``, ``Element FX`` and, on
macOS, ``Element MFX``. :ref:`plugin-editions` describes how they differ from
the standalone application.
