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
    juce::String source;
    bool compressed = false;

    ScriptInfo() = default;
    ScriptInfo (const ScriptInfo& o) { operator= (o); }
    ~ScriptInfo() = default;

    inline ScriptInfo& operator= (const ScriptInfo& o)
    {
        this->name = o.name;
        this->type = o.type;
        this->author = o.author;
        this->description = o.description;
        this->source = o.source;
        return *this;
    }

    static ScriptInfo read (lua_State*, const juce::String& buffer);
    static ScriptInfo read (const juce::String& buffer);
    static ScriptInfo read (juce::File file);

    static ScriptInfo parse (const juce::String& buffer);
    static ScriptInfo parse (juce::File file);

    bool valid() const noexcept { return name.isNotEmpty(); }
};

class EL_API Script : public Model {
public:
    Script();
    Script (const Script& o);
    Script (const juce::ValueTree& data);
    Script (const juce::String& src);
    ~Script() = default;

    Script& operator= (const Script& o);

    juce::String name() const noexcept;
    void setName (const juce::String& newName);

    juce::String source() const noexcept;
    void setSource (const juce::String&);
    bool valid() const noexcept;

private:
    void setMissing();
};

} // namespace element
