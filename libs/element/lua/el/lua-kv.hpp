
#pragma once
#include "lua-kv.h"
#include <sstream>
#include <sol/sol.hpp>

#ifndef LKV_JUCE_HEADER
 #define LKV_JUCE_HEADER "JuceHeader.h"
#endif

namespace kv {
namespace lua {
    /** Removes a field from the table then clears it.
        @param tbl Input table
        @param field The field to remove. Lua type MUST be a table
    */
    inline static sol::table remove_and_clear (sol::table tbl, const char* field) {
        // take the klass ref
        auto F = tbl.get<sol::table> (field);
        // clear the table.
        tbl.clear();
        return F;
    }

    template<class T>
    inline static std::string to_string (T& self, const char* name) {
        std::stringstream stream;
        stream << "kv." << name << ": 0x" << std::hex << (intptr_t) &self;
        return stream.str();
    }
}}
