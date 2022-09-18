/// A file or directory on your system.
// @classmod el.File
// @pragma nostrip

#include "lua-kv.hpp"
#include LKV_JUCE_HEADER
#define LKV_TYPE_NAME_FILE "File"

using namespace juce;

EL_PLUGIN_EXPORT
int luaopen_el_File (lua_State* L)
{
    sol::state_view lua (L);
    auto t = lua.create_table();
    t.new_usertype<File> (LKV_TYPE_NAME_FILE, sol::no_constructor, sol::call_constructor, sol::factories (
                                                                                              /// Non-existent file
                                                                                              // @function File.__call
                                                                                              // @within Metamethods
                                                                                              []() { return File(); },

                                                                                              /// File from absolute path
                                                                                              // @string abspath Absolute file path.
                                                                                              // @function File.__call
                                                                                              // @within Metamethods
                                                                                              // @usage
                                                                                              // local f = kv.File ("/path/to/file/or/dir")
                                                                                              // -- do something with file
                                                                                              [] (const char* path) { return File (String::fromUTF8 (path)); }),

                          /// File name with extension (string)(readonly).
                          // @class field
                          // @name File.name
                          // @within Attributes
                          "name",
                          sol::readonly_property ([] (File& self) {
                              return self.getFileName().toStdString();
                          }),

                          /// Absolute file path (string)(readonly).
                          // @class field
                          // @name File.path
                          // @within Attributes
                          "path",
                          sol::readonly_property ([] (File& self) {
                              return self.getFullPathName().toStdString();
                          }));

    auto M = t.get<sol::table> (LKV_TYPE_NAME_FILE);
    t.clear();
    sol::stack::push (L, M);
    return 1;
}
