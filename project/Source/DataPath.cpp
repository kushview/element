
#include "DataPath.h"

namespace Element {
    DataPath::DataPath() { }
    DataPath::~DataPath() { }
    
    const File DataPath::applicationDataDir()
    {
#if JUCE_MAC
        return File::getSpecialLocation(File::userApplicationDataDirectory)
            .getChildFile ("Application Support/Element");
#else
        return File::getSpecialLocation(File::userApplicationDataDirectory)
            .getChildFile ("Element");
#endif
    }
    
    const File DataPath::defaultSettingsFile()
    {
       #if JUCE_DEBUG
        return applicationDataDir().getChildFile ("ElementDebug.conf");
       #else
        return applicationDataDir().getChildFile ("Element.conf");
       #endif
    }
    
    const File DataPath::defaultLocation()
    {
        return File::getSpecialLocation(File::userMusicDirectory)
            .getChildFile ("Element");
    }
    
    const File DataPath::defaultSessionDir()    { return defaultLocation().getChildFile ("Sessions"); }
    const File DataPath::defaultGraphDir()      { return defaultLocation().getChildFile ("Graphs"); }
}
