// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/element.h>
#include <element/model.hpp>
#include <element/lua.hpp>

namespace element {

struct ScriptInfo final {
    juce::String name;
    juce::String type;
    juce::String author;
    juce::String description;
    juce::String code;

    ScriptInfo() = default;

    static ScriptInfo read (lua_State*, const juce::String& buffer);
    static ScriptInfo read (const juce::String& buffer);
    static ScriptInfo read (juce::File file);

    static ScriptInfo parse (const juce::String& buffer);
    static ScriptInfo parse (juce::File file);

    bool valid() const noexcept { return type.isNotEmpty(); }
};

#define EL_SCRIPT_VERSION 0

class EL_API Script : public Model {
public:
    Script();
    Script (const juce::ValueTree& data);
    Script (const juce::String& src);

    inline bool operator== (const Script& o) const noexcept { return objectData == o.objectData; }

    juce::String name() const noexcept;
    void setName (const juce::String& newName);

    juce::String code() const noexcept;
    void setCode (const juce::String&);
    bool valid() const noexcept;

    static Script make (const juce::String& name, const juce::Identifier& type);
    static Script anonymous();
    static Script view();

private:
    void setMissing();
};

} // namespace element
