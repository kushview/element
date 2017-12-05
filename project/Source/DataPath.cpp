
#include "DataPath.h"

namespace Element
{
    namespace DataPathHelpers
    {
        StringArray getSubDirs()
        {
            return StringArray ({ "Controllers", "Graphs", "Presets", "Templates" });
        }
        
        void initializeUserLibrary (const File& path)
        {
            for (const auto& d : getSubDirs())
            {
                const auto subdir = path.getChildFile (d);
                if (subdir.existsAsFile())
                    subdir.deleteFile();
                subdir.createDirectory();
            }
        }
    }
    
    DataPath::DataPath()
    {
        root = defaultLocation();
        DataPathHelpers::initializeUserLibrary (root);
    }
    
    DataPath::~DataPath() { }
    
    const File DataPath::applicationDataDir()
    {
       #if JUCE_MAC
        return File::getSpecialLocation (File::userApplicationDataDirectory)
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
        return File::getSpecialLocation (File::userMusicDirectory)
            .getChildFile ("Element");
    }
    
    const File DataPath::defaultSessionDir()    { return defaultLocation().getChildFile ("Sessions"); }
    const File DataPath::defaultGraphDir()      { return defaultLocation().getChildFile ("Graphs"); }
}
