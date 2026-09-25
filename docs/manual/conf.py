# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
# import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))

import re
from pathlib import Path

import sphinx_rtd_theme

# -- Project information -----------------------------------------------------

project = 'Element'
copyright = '2026, Kushview'
author = 'Kushview'


def _element_version():
    """Reads the version from the root CMakeLists.txt project() call."""
    cmake = Path(__file__).resolve().parents[2] / 'CMakeLists.txt'
    try:
        match = re.search(r'project\s*\(\s*element\s+VERSION\s+([\d.]+)', cmake.read_text())
        if match:
            return match.group(1)
    except OSError:
        pass
    return '0.0.0'


# The full version, including alpha/beta/rc tags
release = _element_version()
version = release


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'myst_parser',
    'sphinx_rtd_theme',
    'sphinxcontrib.luadomain',
]

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', 'README*']

# Number figures and tables in every output format.
numfig = True


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = "sphinx_rtd_theme"

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

html_title = 'Element User Manual'
html_logo = '_static/logo.png'
html_theme_options = {
    'navigation_depth': 3,
    'collapse_navigation': False,
}

# The master doc to use
master_doc = 'index'

# -- Options for LaTeX / PDF output -------------------------------------------
#
# The manual is typeset as a 6x9 inch book: parts, chapters, sections.

latex_engine = 'xelatex'
latex_theme = 'manual'
latex_toplevel_sectioning = 'part'
latex_show_urls = 'footnote'
latex_logo = '_static/logo.png'

latex_documents = [
    ('index', 'element-manual.tex', 'Element User Manual', 'Kushview, LLC', 'manual', False),
]

latex_elements = {
    'pointsize': '10pt',
    # Latin Modern by file name: present in every TeX Live, including BasicTeX,
    # unlike the FreeFont family Sphinx assumes for xelatex.
    'fontpkg': r'''
\setmainfont{lmroman10-regular.otf}[
  BoldFont       = lmroman10-bold.otf,
  ItalicFont     = lmroman10-italic.otf,
  BoldItalicFont = lmroman10-bolditalic.otf]
\setsansfont{lmsans10-regular.otf}[
  BoldFont       = lmsans10-bold.otf,
  ItalicFont     = lmsans10-oblique.otf,
  BoldItalicFont = lmsans10-boldoblique.otf]
\setmonofont{lmmonolt10-regular.otf}[
  BoldFont       = lmmonolt10-bold.otf,
  ItalicFont     = lmmonolt10-oblique.otf,
  BoldItalicFont = lmmonolt10-boldoblique.otf]
''',
    'geometry': r'''
\usepackage[paperwidth=6in,paperheight=9in,margin=0.74in]{geometry}
''',
    'preamble': r'''
\setcounter{tocdepth}{1}
% :menuselection: emits U+2023 (triangular bullet), which Latin Modern lacks.
\IfFileExists{newunicodechar.sty}{%
  \usepackage{newunicodechar}%
  \newunicodechar{‣}{\ensuremath{\triangleright}}%
}{}
''',
}
