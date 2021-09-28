/*
    This file is part of Element
    Copyright (C) 2021  Kushview, LLC.  All rights reserved.

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

#pragma once

#include "Utils.h"

namespace Element {

class ScriptSource
{
public:
    ScriptSource() = default;
    virtual ~ScriptSource() = default;

    /** Returns the script text */
    virtual String getCode() const =0;

    /** Updates the script text */
    virtual void setCode (const String& code) =0;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptSource);
};

/** A script source for a value tree */
class ValueTreeScriptSource : public ScriptSource
{
public:
    ValueTreeScriptSource (const ValueTree& tree) : data(tree) {}
    ValueTreeScriptSource (const ValueTree& tree, const Identifier& dataKey)
        : data (tree), 
          key (dataKey)
    { }

    String getCode() const override
    { 
        return data.isValid() && data.hasProperty (key)
            ? gzip::decode (data.getProperty(key).toString())
            : String();
    }

    void setCode (const String& code) override
    {
        data.setProperty (key, gzip::encode (code), nullptr);
    }

private:
    ValueTree data;
    Identifier key { "data" };
};

}
