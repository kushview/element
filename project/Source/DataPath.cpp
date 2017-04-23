
#include "DataPath.h"

namespace Element {
    DataPath::DataPath() { }
    DataPath::~DataPath() { }
    
    const File DataPath::defaultSettingsFile()
    {
       #if JUCE_MAC
        return File::getSpecialLocation(File::userApplicationDataDirectory)
            .getChildFile("Application Support/Element/Element.conf");
       #else
        return File::getSpecialLocation(File::userApplicationDataDirectory)
            .getChildFile("Element/Element.conf");
       #endif
    }
}
