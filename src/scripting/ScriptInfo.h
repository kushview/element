#pragma once

#include "JuceHeader.h"

namespace Element {

struct ScriptInfo
{
    String name;
    String author;
    String trigger;
    String description;

    bool isValid() const
    {
        return name.isNotEmpty() && trigger.isNotEmpty();
    }
};

}
