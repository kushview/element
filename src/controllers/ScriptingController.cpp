

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
