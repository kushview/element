
#ifndef EL_DATAPATH_H
#define EL_DATAPATH_H

#include "ElementApp.h"

#define EL_PRESET_FILE_EXTENSIONS "*.elp;*.elpreset"

namespace Element
{
    class Node;
    class NodeArray;
    
    class DataPath
    {
    public:
        DataPath();
        ~DataPath();
        
        static const File applicationDataDir();

        static const File defaultSettingsFile();
        static const File defaultLocation();
        static const File defaultSessionDir();
        static const File defaultGraphDir();
        
        const File& getRootDir() const { return root; }
        File createNewPresetFile (const Node& node, const String& name = String()) const;
        void findPresetsFor (const String& format, const String& identifier, NodeArray& nodes) const;
        
    private:
        File root;
    };
    
    class DataSearchPath { };
}

#endif  // EL_DATAPATH_H
