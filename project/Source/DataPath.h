
#ifndef EL_DATAPATH_H
#define EL_DATAPATH_H

#include "ElementApp.h"

namespace Element
{
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
        
    private:
        File root;
    };
    
    class DataSearchPath { };
}

#endif  // EL_DATAPATH_H
