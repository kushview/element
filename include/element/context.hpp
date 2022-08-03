
#pragma once

#include <iostream>
#include <memory>
#include <string>

#include "element.hpp"

namespace element {

class Modules;
class Scripting;

class EL_API Context {
public:
    Context();
    virtual ~Context();

    //=========================================================================
    void open_module (const std::string& path);
    void load_modules();
    void add_module_path (const std::string&);
    void discover_modules();

    //=========================================================================
    Scripting& scripting() noexcept { return *p_scripting; }

private:
    EL_DISABLE_COPY (Context);
    EL_DISABLE_MOVE (Context);
    friend class Module; // FIXME
    std::unique_ptr<Modules> modules;
    std::unique_ptr<Scripting> p_scripting;
};

} // namespace element
