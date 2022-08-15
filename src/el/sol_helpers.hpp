
#pragma once

#include <sstream>
#include "lua.hpp"
#include "sol/sol.hpp"

#include "JuceHeader.h"

namespace sol {
/** Support juce::ReferenceCountedObjectPtr */
template <typename T>
struct unique_usertype_traits<ReferenceCountedObjectPtr<T>>
{
    typedef T type;
    typedef ReferenceCountedObjectPtr<T> actual_type;
    static const bool value = true;
    static bool is_null (const actual_type& ptr) { return ptr == nullptr; }
    static type* get (const actual_type& ptr) { return ptr.get(); }
};
} // namespace sol

namespace Element {
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