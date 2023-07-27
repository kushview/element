// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/element.h>
#include <element/gzip.hpp>

namespace element {

class EL_API ScriptSource
{
public:
    ScriptSource() = default;
    virtual ~ScriptSource() = default;

    /** Returns the script text */
    virtual String getCode() const = 0;

    /** Updates the script text */
    virtual void setCode (const String& code) = 0;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScriptSource)
};

/** A script source for a value tree */
class EL_API ValueTreeScriptSource : public ScriptSource
{
public:
    ValueTreeScriptSource (const ValueTree& tree) : data (tree) {}
    ValueTreeScriptSource (const ValueTree& tree, const Identifier& dataKey)
        : data (tree),
          key (dataKey)
    {
    }

    String getCode() const override
    {
        return data.isValid() && data.hasProperty (key)
                   ? gzip::decode (data.getProperty (key).toString())
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

} // namespace element
