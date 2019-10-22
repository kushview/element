/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "controllers/ScriptingController.h"

#if HAVE_PYTHON
#include <pybind11/embed.h>
namespace py = pybind11;

PYBIND11_EMBEDDED_MODULE(element, module) {
    module.def("add", [](int i, int j) -> int {
        return i + j;
    });
}
#endif

namespace Element {

struct PythonTest
{
    void run()
    {
       #if HAVE_PYTHON
        py::scoped_interpreter guard{};
        try {
            py::exec(R"(
                import element
                answer = element.add(4, 5)
                print("[EL] Hello from Python! The answer is %s" % answer)
            )");
        }  catch (std::exception& e) {
            std::cout << e.what();
        }
       #endif
    }
};

ScriptingController::ScriptingController() {}
ScriptingController::~ScriptingController() {}

void ScriptingController::activate()
{
}

void ScriptingController::deactivate()
{
}

}
