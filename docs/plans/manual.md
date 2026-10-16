# Element User Manual: master plan and checklist

Living document for the manual under `docs/manual/`. It records the book
structure, the writing conventions, and what is still to do. Check items off
as they land. Screenshots are tracked in `manual-screenshots.md`; code defects
found while writing are tracked in `manual-code-issues.md`.

The manual is built with Sphinx (reStructuredText, MyST for the developer
pages). `index.rst` is the master document. It is published on Read the Docs,
installed with Element on Linux (`share/doc/element/manual`), and typeset as a
6x9 inch book with `docs-manual-pdf` (see `developers/building.md`).

---

## Structure

Parts are directories with an `index.rst` holding the part's toctree. The root
toctree is `:numbered:`. In LaTeX, parts become `\part`, chapters `\chapter`;
the Appendices part switches to `\appendix` so its chapters letter as A, B, C.

| Part | Directory | Chapters |
|------|-----------|----------|
| I Getting Started | `getting-started/` | introduction, installation, quick-start, concepts |
| II The Interface | `interface/` | main-window, menus-and-commands, graph-editor, patch-bay, mixer-strip-meters, session-tree, node-panel, plugin-windows |
| III Working with Element | `using/` | sessions, graphs, nodes-and-plugins, plugin-manager, transport-and-clock, midi-mapping, virtual-keyboard, osc, preferences |
| IV Node Reference | `nodes/` | audio, midi, utility |
| V Scripting | `scripting/` | overview, script-types, script-node, console, examples |
| VI Element as a Plugin | `plugin/` | editions |
| VII Developers | `developers/` | building, code-style, lua-style (Markdown) |
| Appendices | `appendix/` | shortcuts, osc-commands, files-and-locations, troubleshooting, glossary |

---

## Conventions

- **Headings**: `=` page title, `-` section, `~` subsection, `^` sub-subsection.
  One title per file.
- **Every page** starts with `.. include:: /shortcuts.rstext` followed by a
  label `.. _<part>-<page>:` (part index pages use `.. _part-<name>:`).
  Cross-reference with `:ref:` only; never use relative document links.
- **Substitutions and link targets** live in `shortcuts.rstext`: `|El|`,
  `|rarr|`, `|cmd|`, and the named URLs (`Element website`_, `scripting API`_,
  `source repository`_, `issue tracker`_). Do not hardcode those URLs in pages.
- **GUI text**: `:guilabel:` for labels and buttons, `:menuselection:` for menu
  paths (`View --> Plugin Manager`), `:kbd:` for keys, `:file:` for paths,
  `:term:` on the first use of a glossary word on a page.
- **Keys**: write `Cmd`; the shortcuts appendix and the introduction say it is
  `Ctrl` on Windows and Linux.
- **Voice**: second person, present tense, task-first ("To rename a node, ...").
  Marketing tone only in the introduction.
- **Figures**: `.. figure::` with `:alt:` and a caption; files under
  `images/<part>/<page>-<nn>.png`. Until an image exists, write the figure as a
  comment with a marker line above it:

  ```
  .. SCREENSHOT interface/graph-editor-01: block context menu open on a plugin
     node, Normal display mode, vertical orientation
  .. .. figure:: /images/interface/graph-editor-01.png
  ..    :alt: The block context menu
  ..
  ..    The block context menu.
  ```

  Add a matching row to `manual-screenshots.md`.
- **Admonitions**: `note`, `tip`, `warning` only. No bold pseudo-headings.
- **Tables**: `list-table` for anything wider than three columns so LaTeX
  wraps it.
- **Platform differences** are stated inline, not in separate pages.
- **Facts** come from the source code, re-read while writing. Anything that
  cannot be confirmed in code is left out and noted in `manual-code-issues.md`.
- **Build must stay warning-free**:
  `python3 -m sphinx -W --keep-going -b html docs/manual <out>`.

---

## Phases

- [x] **1. Scaffold**: layout above, `conf.py` book settings, shortcuts file,
      old pages merged into their new homes, side files, `docs-manual-pdf`.
- [x] **2. Part I Getting Started**
  - [x] introduction (formats incl. CLAP, editions, platforms)
  - [x] installation (macOS, Windows, Linux, plugin editions, first run, updates).
        Installer file types are not verifiable from the repo (the GitHub
        release has no assets); the chapter stays generic about them.
  - [x] quick-start
  - [x] concepts
- [x] **3. Part II The Interface** (all eight chapters written from
      `mainmenu.cpp`, `guiservice.cpp`, `standard.cpp`, `content.cpp`,
      `grapheditorcomponent.cpp`, `block.cpp`, `contextmenus.cpp`,
      `connectiongrid.cpp`, `graphmixerview.cpp`, `nodechannelstrip.hpp`,
      `meterbridge.cpp`, `sessiontreepanel.cpp`, `navigation.cpp`,
      `nodeproperties.cpp`, `pluginwindow.cpp`)
- [x] **4. Part III Working with Element** (nine chapters; preferences
      labels verbatim from `src/ui/preferences.cpp`; learn order verified in
      `mappingservice.cpp`: parameter first, then MIDI control)
- [x] **5. Part IV Node Reference** (from `src/nodes/*.hpp|cpp`,
      `src/engine/internalformat.cpp`, `include/element/node.h`; the MCU node
      is debug-only and omitted)
- [x] **6. Part V Scripting** (overview, script-types fixed and extended
      with Content/View/Anonymous, script-node, console, examples)
- [x] **7. Part VI Plugin, Part VII Developers** (editions from
      `src/pluginprocessor.cpp`, `src/plugins/*.cc`, root `CMakeLists.txt`;
      developers pages: typos, LV2 deps, Documentation section)
- [x] **8. Appendices** (shortcuts from the menus chapter; OSC verified in
      `src/utils.hpp`; locations from `src/settings.cpp`, `src/datapath.cpp`,
      `src/pluginmanager.cpp`; glossary rewritten)
- [x] **9. Editorial pass** (September 25, 2026): `:ref:` in every
      chapter, codespell clean, `-b linkcheck` clean, screenshot markers match
      `manual-screenshots.md` (22 needed, 6 existing), work-in-progress note
      removed, HTML builds with `-W`, PDF 213 pages with no missing glyphs.

## Next

- Take the screenshots listed in `manual-screenshots.md` and uncomment the
  figures.
- Fix the items in `manual-code-issues.md`, then document what they unlock.
- Read the PDF end to end for flow; the draft was written chapter by
  chapter from the source code, not from using the application.

---

## Notes

- Lua API reference: generated by ldoc from `docs/config.ld` (CMake target
  `docs-lua`, `util/ldoc.sh`, published by `.github/workflows/ldoc.yml`). The
  manual links to it and uses `sphinxcontrib.luadomain` for the script API
  tables; that extension is pip-only, so Linux builds need the packages in
  `docs/manual/requirements.txt`.
- `latexmk` is required for `docs-manual-pdf`; xelatex is the engine.
