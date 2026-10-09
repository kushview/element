.. include:: /shortcuts.rstext

.. _using-plugin-manager:

Plugin Manager
==============

The plugin manager lists every plugin |El| knows about, runs scans, and
manages search paths, favourites and hidden plugins. Open it with
:menuselection:`View --> Plugin Manager`; :kbd:`Esc` or :guilabel:`Close`
returns to the previous view.

.. figure:: /images/using/plugin-manager-00.png
   :alt: The plugin manager

   The plugin manager.

Formats
-------

Which formats are available depends on the platform and the build:

- macOS: Audio Unit, VST3, CLAP, LV2, and VST (2.4) where built.
- Windows: VST3, CLAP, LV2, and VST where built.
- Linux: VST3, CLAP, LV2, and VST where built.

|El|'s own nodes are listed under the ``Element`` format and never need
scanning.

The list
--------

The table has a favourite column, then :guilabel:`Name`, :guilabel:`Format`,
:guilabel:`Category`, :guilabel:`Manufacturer`, :guilabel:`Description` and
:guilabel:`IO` (the channel counts). Click a header to sort. Click the heart
to make a plugin a favourite; favourites appear first in the
:guilabel:`Favorites` submenu when adding nodes.

Right-click a row for :guilabel:`Hide plugin` or :guilabel:`Show plugin`
(hidden plugins stay in the list but leave the add-node menus),
:guilabel:`Clear list` and :guilabel:`Remove selected`. :kbd:`Delete` also
removes the selected rows. Plugin files dropped on the list are added
directly.

Scanning
--------

:guilabel:`Scan` scans every supported format. :guilabel:`Options...` offers
:guilabel:`Scan for new or updated <format> plugins` for one format at a
time. Both open :guilabel:`Select folders to scan...` with the format's
search paths, which you can adjust before pressing :guilabel:`Scan`. A folder
that does not look like a plugin folder is queried first, because scanning
arbitrary files can be slow.

Scanning runs in a separate process. A plugin that crashes while being
scanned cannot crash |El|; it is recorded in a blacklist and skipped from
then on. Plugins that fail to load are reported at the end of the scan,
and the status bar shows :guilabel:`Scanning:` with the current plugin. The
scanner's log is ``scanner.log`` in the log folder
(:ref:`appendix-troubleshooting`).

:guilabel:`Scan plugins on startup` in the preferences repeats the scan each
time |El| starts, in the background.

Search paths
------------

:guilabel:`Options... --> Search Paths` has an entry per format:
:guilabel:`CLAP Path`, :guilabel:`VST Path`, :guilabel:`VST3 Path` and
:guilabel:`LV2 Path`. Each opens a list of folders to add to or remove from,
saved with :guilabel:`Save`. On the first run the standard folders for each
format are filled in.

Housekeeping
------------

The rest of the :guilabel:`Options...` menu:

- :guilabel:`Clear list` forgets every scanned plugin.
- :guilabel:`Remove selected plug-in from list`.
- :guilabel:`Show folder containing selected plug-in` opens it in your file
  manager.
- :guilabel:`Remove any plug-ins whose files no longer exist`.
- :guilabel:`Clear blacklisted plug-ins` gives plugins that crashed the
  scanner another chance on the next scan.

Unverified plugins
------------------

At startup |El| looks through the search paths for plugin files it has not
scanned and offers them under :guilabel:`Unverified` in the add-node menus,
grouped by format. Adding one loads it directly; if it loads, it joins the
list. This is the quickest way to use a freshly installed plugin without a
full scan.
