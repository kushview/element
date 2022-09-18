/*
    This file is part of Element
    Copyright (C) 2018-2020  Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "LuaUnitTest.h"
using namespace element;

class LuaStream : public LuaUnitTest {
public:
    LuaStream() : LuaUnitTest ("LuaStream", "Lua", "stream") {}
    ~LuaStream() override {}

    void runTest() override
    {
        sol::state& L = lua;
        beginTest ("C to Lua");
        const char* testData = "This is some test data";

        try {
            sol::userdata f = L["io"]["tmpfile"]();
            auto* stream = (luaL_Stream*) f.pointer();
            fwrite (testData, 1, strlen (testData), stream->f);
            expect (ftell (stream->f) == strlen (testData));
            rewind (stream->f);
            lua["stream"] = f;

            runSnippet ("stream_from_c.lua");
            expect (lua["currentpos"].get<lua_Integer>() == 0);
            expect (lua["result"].get<std::string>() == std::string (testData));
        } catch (const std::exception& e) {
            expect (false, e.what());
        }

        lua.collect_garbage();
    }
};

static LuaStream sLuaTableTest;

template <typename T>
static T* proxy_userdata (const sol::table& proxy)
{
    if (! proxy.valid())
        return nullptr;
    auto mt = proxy[sol::metatable_key];
    return mt["__impl"].get_or<T*> (nullptr);
}

void my_panic (sol::optional<std::string> maybe_msg)
{
    std::cerr << "Lua is in a panic state and will now abort() the application" << std::endl;
    if (maybe_msg) {
        const std::string& msg = maybe_msg.value();
        std::cerr << "\terror message: " << msg << std::endl;
    }
    // When this function exits, Lua will exhibit default behavior and abort()
}

class LuaInherritUserdata : public LuaUnitTest {
public:
    LuaInherritUserdata() : LuaUnitTest ("Inherrit CXX", "Lua", "userdata") {}
    ~LuaInherritUserdata() override {}

    class Parent {
    public:
        Parent()
        {
            // DBG("Parent::Parent()");
        }

        ~Parent()
        {
            // DBG("Parent::~Parent()");
        }

        void printValue()
        {
            std::clog << getValue() << std::endl;
        }

        static void init (sol::variadic_args args)
        {
            sol::object obj = args.get<sol::object>();
            if (obj.is<sol::table>()) {
                auto self = obj.as<sol::table>();
                if (auto* impl = proxy_userdata<Parent> (self)) {
                    impl->proxy = self;
                    impl->_value = self["value"];
                }
            }
        }

        double getValue() const
        {
            if (_value) {
                sol::object ret = _value (proxy);
                return ret.as<double>();
            }
            return value;
        }

        void setValue (const double val)
        {
            if (val == value)
                return;
            value = val;
            if (changed)
                changed();
        }

        std::function<void()> changed;

        void set_name (const std::string& n) { name = n; }
        std::string get_name() const { return name; }

    private:
        std::string name = "Parent Object";
        double value = 100;
        sol::table proxy;
        sol::function _value;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Parent);
    };

    void runTest() override
    {
        sol::automagic_enrollments enroll;
        enroll.call_operator = false;
        enroll.default_constructor = false;
        enroll.pairs_operator = false;

        beginTest ("Sol3 Parent");
        auto T = lua.new_usertype<Parent> ("Parent",
                                           sol::no_constructor,
                                           "new",
                                           sol::constructors<Parent()>(),
                                           "name",
                                           sol::property (&Parent::get_name, &Parent::set_name),

                                           "init",
                                           Parent::init,

                                           "set",
                                           &Parent::setValue,
                                           "get",
                                           &Parent::getValue,
                                           "print",
                                           &Parent::printValue,
                                           "changed",
                                           &Parent::changed);

        sol::table T_mt = T[sol::metatable_key];
        T_mt.set_function ("__newuserdata", [this]() {
            return std::make_shared<Parent>();
        });

        T_mt["__pairs"] = sol::lua_nil;

        T_mt["__props"] = lua.create_table().add (
            "name");

        T_mt["__methods"] = lua.create_table().add (
            "print", "get", "set");

        auto status = runSnippet ("sol3_parent.lua");
        expect (status == sol::call_status::ok);
    }
};

static LuaInherritUserdata sLuaInherritUserdata;
#if 0

//=============================================================================
class StaticMethodTest : public UnitTestBase
{
public:
    StaticMethodTest() : UnitTestBase ("Lua Static Method", "Lua", "static") {}
    virtual ~StaticMethodTest() { }
    void initialise() override
    {
        lua.open_libraries();
    }

    void shutdown() override
    {
        lua.collect_garbage();
    }

    void runTest() override
    {
        beginTest ("static method");
        lua.new_usertype<Object> ("Object", sol::no_constructor,
            "one_hundred", Object::oneHundred
        );

        try {
            lua.script (R"(
                Object.two_hundred = function() return 200 end
                value100 = Object:one_hundred()
                value200 = Object:two_hundred()
            )");
            expect (100 == (int) lua["value100"]);
            expect (200 == (int) lua["value200"]);
        } catch (const std::exception& e) {
            expect (false, e.what());
        }
    }

private:
    struct Object { inline static int oneHundred() { return 100; } };
    sol::state lua;
};

static StaticMethodTest sStaticMethodTest;


class CArrayTest : public UnitTestBase
{
public:
    CArrayTest() : UnitTestBase ("Lua C-Array", "Lua", "carray") {}
    virtual ~CArrayTest() { }
    void initialise() override
    {
        lua.open_libraries();
    }

    void shutdown() override
    {
        lua.collect_garbage();
    }

    void runTest() override
    {
        beginTest ("raw");
        int size = 100;
       
        values.reset (new float [size]);
        // auto rawValues = new float [size];
        float rawValues [100];
        std::vector<float> vvalues (100, 0);
        vvalues.reserve (size);
        for (int i = 0; i < 100; ++i)
        {
            rawValues[i] = (float) i + 100.f;
            vvalues[i] = rawValues[i];
        }

        lua["params"] = std::ref(rawValues); //[this]() { return &rawValues; };
        lua["size"] = [&size]() -> int { return size; };
        size = 10;
        
        try {
            lua.script (R"(
                for k = 1, size() do
                    v = params[k]
                    print(" ", k, v)
                end
            )");
        } catch (const std::exception& e) {
            expect (false, e.what());
        }
    }

private:
    std::unique_ptr<float []> values;
    float* rawValues = nullptr;
    sol::state lua;
};

static CArrayTest sCArrayTest;

class LuaGlobalsTest : public UnitTestBase
{
public:
    LuaGlobalsTest() : UnitTestBase ("Lua Globals", "Lua", "globals") {}
    virtual ~LuaGlobalsTest() { }
    void initialise() override
    {
        initializeWorld();
        lua.open_libraries();
        
        lua["expect"] = [this](bool result, const char* msg) {
            this->expect (result, msg);
        };
    }

    void shutdown() override
    {
        lua.collect_garbage();
        shutdownWorld();
    }

    void runTest() override
    {
        testSession();
    }

private:
    sol::state lua;

    void testSession()
    {
        beginTest("session");
        auto s = getWorld().getSession();
        s->setName ("test session");
        
        try
        {
            auto result = lua.safe_script (R"(
                local s = element.session()
                expect (s.name == "test session", "incorrect session name")
                s.name = "lua session"
                expect (s.name == "lua session", "wrong session name")
                local g = element.newgraph()
                expect (#g == 0, string.format ("%d != 0", #g))
                --[[
                s:add_graph (g, false)
                g = element.newgraph (true, "New Graph")
                print(g)
                expect (#g == 4, string.format ("%d != 5", #g))
                s:add_graph (g, false)
                expect (#s == 2, "incorrect number of graphs")
                --]]
            )");

            expect (result.valid());
            expect (s->getNumGraphs() == 0);
            expect (s->getName() == "lua session");
        }
        catch (const std::exception& e)
        {
            expect (false, e.what());
        }
    }
};

static LuaGlobalsTest sLuaGlobalsTest;

//=============================================================================

class LuaAudioBufferTest : public UnitTestBase
{
public:
    LuaAudioBufferTest() : UnitTestBase ("Lua Globals", "Lua", "audiobuffer") {}
    virtual ~LuaAudioBufferTest() { }
    
    void initialise() override
    {
        lua.open_libraries();
        lua.collect_garbage();
    }

    void shutdown() override
    {
        lua.collect_garbage();
    }

    void runTest() override
    {
        beginTest ("buffer");
        lua.script (R"(
            local audio = require ('kv.audio')
            local b = audio.Buffer()
        )");
    }

private:
    sol::state lua;
};

static LuaAudioBufferTest sLuaAudioBufferTest;

//=============================================================================
class LuaScriptsTest : public LuaUnitTest
{
public:
    LuaScriptsTest() : LuaUnitTest ("Lua", "Lua", "kv") {}
    virtual ~LuaScriptsTest() {}

    void initialise() override
    {
        LuaUnitTest::initialise();
        DirectoryIterator iter (getTestsDir().getChildFile("lua"), true, "*.lua", File::findFiles);
        while (iter.next())
        {
            auto* const t = cases.add (new TestCase (*this, lua, iter.getFile()));
            t->init();
        }
    }

    void shutdown() override
    {
        for (auto* t : cases)
            t->shutdown();
        cases.clearQuick (true);
        LuaUnitTest::shutdown();
    }

    void runTest() override
    {
        for (auto* t : cases)
            t->run();
    }

private:
    class TestCase
    {
    public:
        TestCase (LuaScriptsTest& t, sol::state& l, const File& f)
            : script (f), test(t), lua (l), env (l, sol::create, l.globals())
        {
            env.set_function ("begintest", [this](const char* m) { test.beginTest (String::fromUTF8 (m)); });
            env.set_function ("expect", sol::overload (
                [this](bool r) { test.expect (r, String()); },
                [this](bool r, const char* m) { test.expect (r, m != nullptr ? String::fromUTF8 (m) : String()); }
            ));
        }

        void init()
        {
            lua.script_file (script.getFullPathName().toRawUTF8(), env);
            if (sol::function f = env ["init"])
                f();
        }

        void run()
        {
            if (sol::function f = env ["run"])
                f();
            lua.collect_garbage();
        }

        void shutdown()
        {
            if (sol::function f = env ["shutdown"])
                f();
        }

    private:
        File script;
        LuaScriptsTest& test;
        sol::state& lua;
        sol::environment env;
    };

    OwnedArray<TestCase> cases;

    void testScript (const String& name, const String& script)
    {
        beginTest (name);
        try {
            lua.script (script.toRawUTF8());
        } catch (const std::exception& e) {
            expect (false, e.what());
        }
    }

    void testScript (const String& name, const File& script)
    {
        if (script.existsAsFile())
        {
            try {
                sol::environment env (lua, sol::create, lua.globals());
                env.set_function ("begintest", [this](const char* m) { this->beginTest (String::fromUTF8 (m)); });
                env.set_function ("expect", sol::overload (
                    [this](bool r) { this->expect (r, String()); },
                    [this](bool r, const char* m) { this->expect (r, m != nullptr ? String::fromUTF8 (m) : String()); }
                ));

                lua.script_file (script.getFullPathName().toRawUTF8(), env);

                if (sol::function f = env ["init"])
                    f();
                if (sol::function f = env ["run"])
                    f();
                if (sol::function f = env ["shutdown"])
                    f();

            } catch (const std::exception& e) {
                ignoreUnused (e);
            }
        }
    }

    File resolveScript (const String& leaf) {
        return getTestsDir().getChildFile("lua").getChildFile (leaf);
    }
};

static LuaScriptsTest sLuaScriptsTest;

#endif
