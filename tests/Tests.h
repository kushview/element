#pragma once

#include "JuceHeader.h"
#include "engine/AudioEngine.h"
#include "engine/GraphProcessor.h"
#include "engine/InternalFormat.h"
#include "session/PluginManager.h"
#include "session/PluginManager.h"
#include "Globals.h"

namespace Element 
{
    class UnitTestBase : public UnitTest
    {
    public:
        UnitTestBase (const String& name, const String& category = String(), 
                      const String& _slug = String())
            : UnitTest (name, category), slug (_slug) { }
        const String getId() const { return getCategory().toLowerCase() + "." + getSlug().toLowerCase(); }
        const String& getSlug() const { return slug; }
    private:
        const String slug;
    };
}
