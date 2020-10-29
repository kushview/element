#pragma once

#include "JuceHeader.h"

namespace Element {

struct ScriptDescription
{
    String name;
    String type;
    String author;
    String description;

    ScriptDescription() = default;
    ScriptDescription (const ScriptDescription& o) { operator= (o); }
    ~ScriptDescription() = default;
    
    ScriptDescription& operator= (const ScriptDescription& o)
    {
        this->name          = o.name;
        this->type          = o.type;
        this->author        = o.author;
        this->description   = o.description;
        return *this;
    }

    bool isValid() const
    {
        return name.isNotEmpty() && type.isNotEmpty();
    }

    static ScriptDescription parse (const String& buffer)
    {
        static const StringArray tags = { "@author", "@name", "@description" };

        ScriptDescription desc;
        const auto lines = StringArray::fromLines (buffer);
        int index = 0;
        bool inBlock = false;
        bool finished = false;
        for (int index = 0; index < lines.size(); ++index)
        {
            const auto line = lines[index].trim();
            
            if (! inBlock)
                inBlock = line.startsWith ("--[[");
            
            if (inBlock || line.startsWith ("--"))
            {
                for (const auto& tag : tags)
                {
                    if (line.contains (tag))
                    {
                        const auto value = line.fromFirstOccurrenceOf (tag, false, false).trimStart()
                                               .upToFirstOccurrenceOf ("--]]", false, false).trimEnd();
                        
                        // DBG (tag.replace("@","") << " = " << value);
                        
                        if (tag == "@name" && desc.name.isEmpty())
                        {
                            desc.name = value;
                        }
                        else if (tag == "@element" && desc.type.isEmpty())
                        {
                            desc.type = value;
                        }
                        else if (tag == "@author" && desc.author.isEmpty())
                        {
                            desc.author = value;
                        }
                        else if (tag == "@description" && desc.description.isEmpty())
                        {
                            desc.description = value;
                        }
                    }
                }

                if (inBlock)
                {
                    inBlock  = ! line.contains ("--]]");
                    finished = ! inBlock;
                }
            }
            else if (! inBlock && ! line.startsWith ("--"))
            {
                finished = true;
            }

            if (finished) {
                // DBG("finihed at line index: " << index);
                // DBG("LINE: " << lines[index]);
                break;
            }
        }

        return desc;
    }
     
    static ScriptDescription parse (File file)
    {
        return parse (file.loadFileAsString());
    }
};

}
