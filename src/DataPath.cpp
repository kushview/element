
#include "DataPath.h"
#include "session/Node.h"

namespace Element
{
    namespace DataPathHelpers
    {
        StringArray getSubDirs()
        {
            return StringArray ({ "Controllers", "Graphs", "Presets", "Templates", "Sessions" });
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
    
    const File DataPath::defaultSessionDir()        { return defaultLocation().getChildFile ("Sessions"); }
    const File DataPath::defaultGraphDir()          { return defaultLocation().getChildFile ("Graphs"); }
    const File DataPath::defaultControllersDir()    { return defaultLocation().getChildFile ("Controllers"); }

    File DataPath::createNewPresetFile (const Node& node, const String& name) const
    {
        String path = "Presets/";
        if (name.isNotEmpty())
            path << name;
        else
            path << String (node.getName().isNotEmpty() ? node.getName() : "New Preset");
        path << ".elpreset";
        return getRootDir().getChildFile(path).getNonexistentSibling();
    }
    
    void DataPath::findPresetsFor (const String& format, const String& identifier, NodeArray& nodes) const
    {
        const auto presetsDir = getRootDir().getChildFile ("Presets");
        if (! presetsDir.exists() || ! presetsDir.isDirectory())
            return;

        DirectoryIterator iter (presetsDir, true, EL_PRESET_FILE_EXTENSIONS);
        while (iter.next())
        {
            Node node (Node::parse (iter.getFile()));
            if (node.isValid() && 
                node.getFileOrIdentifier() == identifier && 
                node.getFormat() == format)
            {
                nodes.add (node);
            }
        }
    }

    void DataPath::findPresetFiles (StringArray& results) const
    {
        const auto presetsDir = getRootDir().getChildFile ("Presets");
        if (! presetsDir.exists() || ! presetsDir.isDirectory())
            return;
        DirectoryIterator iter (presetsDir, true, EL_PRESET_FILE_EXTENSIONS);
        while (iter.next())
            results.add (iter.getFile().getFullPathName());
    }
}
