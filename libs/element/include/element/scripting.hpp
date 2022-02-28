
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "element/lua.hpp"

namespace element {

class EL_API Scripting final {
public:
    Scripting();
    ~Scripting();

    std::vector<std::string> available_packages() const noexcept;
    void add_package (const std::string& name, lua::CFunction loader);
    lua_State* root_state() const;

private:
    class State;
    std::unique_ptr<State> state;
    Scripting (const Scripting&) = delete;
};

} // namespace element
