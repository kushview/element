# SPDX-FileCopyrightText: 2026 Kushview, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

# Documentation targets. A missing tool only disables its target with a
# status message.
#
#   docs         builds everything that is available
#   docs-manual  Sphinx user manual  -> <build>/docs/manual/html
#   docs-manual-pdf  6x9 book via LaTeX -> <build>/docs/manual-latex/latex/element-manual.pdf
#   docs-lua     ldoc Lua API docs   -> <build>/docs/lua
#   docs-clean   removes the output of every target above
#
# On Linux the manual is part of the default build and is installed to
# ${CMAKE_INSTALL_DOCDIR}/manual when Sphinx is available.

# Sphinx is run through the interpreter (python -m sphinx) so the module
# check below and the build agree on which installation is used. When a
# sphinx-build is on the PATH, prefer the interpreter it belongs to.
find_program(SPHINX_BUILD_EXECUTABLE NAMES sphinx-build sphinx-build3)
if(SPHINX_BUILD_EXECUTABLE AND NOT Python3_EXECUTABLE)
    get_filename_component(_sphinx_bindir "${SPHINX_BUILD_EXECUTABLE}" REALPATH)
    get_filename_component(_sphinx_bindir "${_sphinx_bindir}" DIRECTORY)
    get_filename_component(Python3_ROOT_DIR "${_sphinx_bindir}" DIRECTORY)
endif()
find_package(Python3 COMPONENTS Interpreter)
find_program(LDOC_EXECUTABLE NAMES ldoc)
find_program(LATEXMK_EXECUTABLE NAMES latexmk)
find_program(XELATEX_EXECUTABLE NAMES xelatex)

set(ELEMENT_HAVE_SPHINX OFF)
if(Python3_Interpreter_FOUND)
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -c "import sphinx, sphinx_rtd_theme, myst_parser, sphinxcontrib.luadomain"
        RESULT_VARIABLE _sphinx_check
        OUTPUT_QUIET ERROR_QUIET)
    if(_sphinx_check EQUAL 0)
        set(ELEMENT_HAVE_SPHINX ON)
    endif()
endif()

add_custom_target(docs)

# Removes only the documentation output; the global 'clean' target would
# also discard the compiled application.
add_custom_target(docs-clean
    COMMAND ${CMAKE_COMMAND} -E rm -rf
        "${CMAKE_CURRENT_BINARY_DIR}/manual"
        "${CMAKE_CURRENT_BINARY_DIR}/manual-latex"
        "${CMAKE_CURRENT_BINARY_DIR}/lua"
    COMMENT "Removing generated documentation"
    VERBATIM)

if(ELEMENT_HAVE_SPHINX)
    set(ELEMENT_MANUAL_SRCDIR "${CMAKE_CURRENT_SOURCE_DIR}/manual")
    set(ELEMENT_MANUAL_HTMLDIR "${CMAKE_CURRENT_BINARY_DIR}/manual/html")
    set(ELEMENT_MANUAL_STAMP "${CMAKE_CURRENT_BINARY_DIR}/manual/html.stamp")

    file(GLOB_RECURSE ELEMENT_MANUAL_SOURCES CONFIGURE_DEPENDS
        "${ELEMENT_MANUAL_SRCDIR}/*.rst"
        "${ELEMENT_MANUAL_SRCDIR}/*.rstext"
        "${ELEMENT_MANUAL_SRCDIR}/*.md"
        "${ELEMENT_MANUAL_SRCDIR}/*.py"
        "${ELEMENT_MANUAL_SRCDIR}/*.lua"
        "${ELEMENT_MANUAL_SRCDIR}/images/*"
        "${ELEMENT_MANUAL_SRCDIR}/_static/*")
    list(FILTER ELEMENT_MANUAL_SOURCES EXCLUDE REGEX "/_build/")

    add_custom_command(
        OUTPUT "${ELEMENT_MANUAL_STAMP}"
        COMMAND "${Python3_EXECUTABLE}" -m sphinx -b html
            -d "${CMAKE_CURRENT_BINARY_DIR}/manual/doctrees"
            "${ELEMENT_MANUAL_SRCDIR}" "${ELEMENT_MANUAL_HTMLDIR}"
        COMMAND ${CMAKE_COMMAND} -E touch "${ELEMENT_MANUAL_STAMP}"
        DEPENDS ${ELEMENT_MANUAL_SOURCES}
        WORKING_DIRECTORY "${ELEMENT_MANUAL_SRCDIR}"
        COMMENT "Building the user manual with Sphinx"
        USES_TERMINAL VERBATIM)

    if(UNIX AND NOT APPLE)
        add_custom_target(docs-manual ALL DEPENDS "${ELEMENT_MANUAL_STAMP}")
        install(DIRECTORY "${ELEMENT_MANUAL_HTMLDIR}/"
            DESTINATION "${CMAKE_INSTALL_DOCDIR}/manual"
            PATTERN ".buildinfo" EXCLUDE)
    else()
        add_custom_target(docs-manual DEPENDS "${ELEMENT_MANUAL_STAMP}")
    endif()
    add_dependencies(docs docs-manual)

    # The PDF is opt-in: it is not part of 'docs' and never part of ALL.
    # latexmk is preferred; without it, three xelatex passes are run directly.
    set(ELEMENT_MANUAL_LATEXDIR "${CMAKE_CURRENT_BINARY_DIR}/manual-latex")
    if(LATEXMK_EXECUTABLE)
        add_custom_target(docs-manual-pdf
            COMMAND "${Python3_EXECUTABLE}" -m sphinx -M latexpdf
                "${ELEMENT_MANUAL_SRCDIR}" "${ELEMENT_MANUAL_LATEXDIR}"
            WORKING_DIRECTORY "${ELEMENT_MANUAL_SRCDIR}"
            COMMENT "Building the user manual PDF with Sphinx and latexmk"
            USES_TERMINAL VERBATIM)
    elseif(XELATEX_EXECUTABLE)
        set(_latex_out "${ELEMENT_MANUAL_LATEXDIR}/latex")
        set(_xelatex "${XELATEX_EXECUTABLE}" -interaction=nonstopmode -halt-on-error element-manual.tex)
        add_custom_target(docs-manual-pdf
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_latex_out}"
            COMMAND "${Python3_EXECUTABLE}" -m sphinx -q -b latex
                "${ELEMENT_MANUAL_SRCDIR}" "${_latex_out}"
            COMMAND ${CMAKE_COMMAND} -E chdir "${_latex_out}" ${_xelatex}
            COMMAND ${CMAKE_COMMAND} -E chdir "${_latex_out}" ${_xelatex}
            COMMAND ${CMAKE_COMMAND} -E chdir "${_latex_out}" ${_xelatex}
            WORKING_DIRECTORY "${ELEMENT_MANUAL_SRCDIR}"
            COMMENT "Building the user manual PDF with Sphinx and xelatex"
            USES_TERMINAL VERBATIM)
    else()
        message(STATUS "Neither latexmk nor xelatex found; 'docs-manual-pdf' target disabled")
    endif()
else()
    message(STATUS "Sphinx with the packages in docs/manual/requirements.txt not found; 'docs-manual' target disabled")
    if(UNIX AND NOT APPLE)
        message(STATUS "The user manual will not be installed")
    endif()
endif()

if(LDOC_EXECUTABLE)
    add_custom_target(docs-lua
        COMMAND ${LDOC_EXECUTABLE} -f markdown -q -X
            -c "${CMAKE_CURRENT_SOURCE_DIR}/config.ld"
            -s "${CMAKE_CURRENT_SOURCE_DIR}"
            --multimodule
            -d "${CMAKE_CURRENT_BINARY_DIR}/lua" .
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Building the Lua API docs with ldoc"
        USES_TERMINAL VERBATIM)
    add_dependencies(docs docs-lua)
else()
    message(STATUS "ldoc not found; 'docs-lua' target disabled")
endif()
