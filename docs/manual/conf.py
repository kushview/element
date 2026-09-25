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
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', 'README.md']


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = "sphinx_rtd_theme"

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

# The master doc to use
master_doc = 'index'

latex_toplevel_sectioning = 'chapter'

# latex_documents = [
#     ('manual', 'manual.tex', 'Element', 'Michael R. Fisher', 'manual', False)
# ]

latex_elements = {
    'geometry': r'''
\usepackage[paperwidth=6in,paperheight=9in,margin=0.74in]{geometry}
'''
}
