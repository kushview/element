/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "gui/widgets/LuaConsole.h"
#include "gui/LookAndFeel.h"
#include "gui/ViewHelpers.h"
#include "scripting/LuaBindings.h"
#include "Commands.h"

#include "sol/sol.hpp"

static int message_handler (lua_State* L)
{
    const char *msg = lua_tostring (L, 1);
    if (msg == NULL)
    {                                             /* is error object not a string? */
        if (luaL_callmeta (L, 1, "__tostring") && /* does it have a metamethod */
            lua_type(L, -1) == LUA_TSTRING)       /* that produces a string? */
            return 1;                             /* that is the message */
        else
            msg = lua_pushfstring (L, "(error object is a %s value)",
                                      luaL_typename(L, 1));
    }
    luaL_traceback (L, L, msg, 1); /* append a standard traceback */
    return 1;                      /* return the traceback */
}

namespace Element {

//=============================================================================

#if 0
class LuaConsole::Content : public Component,
                                     private Timer
{
public:
    Content (LuaConsole& o)
        : owner (o)
    {
        addAndMakeVisible (buffer);
        buffer.setLookAndFeel (&style);
        addAndMakeVisible (prefix);
        prefix.setText (">", dontSendNotification);
        prefix.setFont (prompt.getFont().withHeight (13.f));
        prefix.setJustificationType (Justification::centred);
        addAndMakeVisible (prompt);
        prompt.setLookAndFeel (&style);

        prompt.onReturnKey = [this]
        {
            if (! haveWorld)
                return;

            auto text = prompt.getText();
            prompt.setText ({}, dontSendNotification);
            buffer.moveCaretToEnd();
            buffer.insertTextAtCaret (String("> ") + text);
            buffer.insertTextAtCaret (newLine);

            buffer.moveCaretToEnd();

            lastError.clear();
            LuaResult result = lua.safe_script (text.toRawUTF8(),
                [this](lua_State* L, LuaResult pfr) { return errorHandler (L, pfr); });
            if (result.valid())
            {
                String str;

                switch (result.get_type())
                {
                    case sol::type::number: str << result.get<double>(); break;
                    case sol::type::string: str << result.get<std::string>(); break;
                    default: break;
                }

                if (str.isNotEmpty())
                {
                    buffer.insertTextAtCaret (str);
                    buffer.insertTextAtCaret (newLine);
                }
            }
            else if (lastError.isNotEmpty())
            {
                buffer.insertTextAtCaret (lastError);
                buffer.insertTextAtCaret (newLine);
            }
        };

       

        setSize (100, 100);
        startTimerHz (60);
    }

    ~Content()
    {
        buffer.setLookAndFeel (nullptr);
        prompt.setLookAndFeel (nullptr);
    }

    void resized() override
    {
        auto r1 = getLocalBounds();
        auto r2 = r1.removeFromBottom (23);
        prefix.setBounds (r2.removeFromLeft (30));
        prompt.setBounds (r2);
        buffer.setBounds (r1);
    }

private:
    using LuaResult = sol::protected_function_result;
    bool haveWorld { false };
    LuaConsole& owner;
    sol::state lua;
    LuaConsoleBuffer buffer;
    Label prefix;
    LuaConsolePrompt prompt;
    String lastError {} ;
    struct Style : public Element::LookAndFeel
    {
        void fillTextEditorBackground (Graphics& g, int w, int h, TextEditor& e) override
        {
            LookAndFeel::fillTextEditorBackground (g, w, h, e);
        }

        void drawTextEditorOutline (Graphics&, int, int, TextEditor&) override {}

        CaretComponent* createCaretComponent (Component* keyFocusOwner) override
        {
            return new CaretComponent (nullptr);
        }
    } style;

    LuaResult errorHandler (lua_State* L, LuaResult pfr) {
        lastError = pfr.get<std::string>();
        return pfr;
    }

    void timerCallback() override
    {
        if (haveWorld)
        {
            stopTimer();
            return;
        }

        if (auto* world = ViewHelpers::getGlobals (this))
        {
            lua.open_libraries();
            Lua::setWorld (lua, world);
            Lua::registerUI (lua);
            Lua::registerModel (lua);
            Lua::registerEngine (lua);
            Lua::registerElement (lua);
            lua["os"]["exit"] = sol::overload (
                [this]()
                {
                    ViewHelpers::invokeDirectly (this, Commands::quit, true);
                },
                [this](int code)
                {
                    JUCEApplication::getInstance()->setApplicationReturnValue (code);
                    ViewHelpers::invokeDirectly (this, Commands::quit, true);
                }
            );           
            haveWorld = true;
        }
    }
};
#endif

LuaConsole::LuaConsole()
    : Console ("Lua Console")
{
    setSize (100, 100);
}

LuaConsole::~LuaConsole() {}

static int exceptionHandler (lua_State* L, sol::optional<const std::exception&>, sol::string_view)
{
    return message_handler (L);
}

void LuaConsole::textEntered (const String& text)
{
    if (text.isEmpty() || env == nullptr)
        return;
    Console::textEntered (text);
    auto& lua = env->getState();
    auto e = env->get();
    
   #if 1
    lua.set_exception_handler (exceptionHandler);
   
    try
    {
        // auto buffer = lua.load_buffer (text.toRawUTF8() ,text.length());
        // buffer.call();
        lua.safe_script (text.toRawUTF8(), env->get(),
            [this](lua_State* L, LuaResult pfr) { return errorHandler (L, pfr); });
        
        if (lastError.isNotEmpty())
            addText (lastError);
    }
    catch (const sol::error& e)
    {
        addText (e.what());
    }
   #else
    auto* L = e.lua_state();
    int status = luaL_loadbuffer (L, text.toRawUTF8(), text.length(), "console");

    if (status == LUA_OK)
    {
        int narg = 0;
        int nres = LUA_MULTRET;
        
        int base = lua_gettop (L) - narg;       /* function index */
        lua_pushcfunction (L, message_handler);  /* push message handler */
        lua_insert (L, base);                   /* put it under function and args */
        // globalL = L;                            /* to be available to 'laction' */
        // signal (SIGINT, laction);               /* set C-signal handler */
        status = lua_pcall (L, narg, nres, base);
        // signal (SIGINT, SIG_DFL); /* reset C-signal handler */
        lua_remove (L, base);     /* remove message handler from the stack */

        if (status == LUA_OK)
        {
            l_print (L);
        }
    }
   #endif
    lua.set_exception_handler (sol::detail::default_exception_handler);
    lastError.clear();
}

void LuaConsole::setEnvironment (LuaEngine::Environment* newEnv)
{
    env.reset (newEnv);
    if (env != nullptr)
    {
        auto e = env->get();
        jassert (e.valid());

        e["os"]["exit"] = sol::overload (
            [this]()
            {
                ViewHelpers::invokeDirectly (this, Commands::quit, true);
            },
            [this](int code)
            {
                JUCEApplication::getInstance()->setApplicationReturnValue (code);
                ViewHelpers::invokeDirectly (this, Commands::quit, true);
            }
        );

        e["print"] = [this](sol::variadic_args va)
        {
            auto e = env->get();
            String msg;
            for (auto v : va)
            {
                sol::function ts = e["tostring"];
                if (ts.valid())
                {
                    sol::object str = ts ((sol::object) v);
                    if (str.valid())
                        if (const char* sstr = str.as<const char*>())
                            msg << sstr << "\t";
                }
            }

            addText (msg.trimEnd());
        };
    }
}

LuaConsole::LuaResult LuaConsole::errorHandler (lua_State* L, LuaResult pfr)
{
    sol::error err = pfr;
    lastError = err.what();
    throw err;
    return pfr;
}

}
