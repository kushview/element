
#ifndef EL_DATAPATH_H
#define EL_DATAPATH_H

#include "ElementApp.h"

namespace Element {
    class DataPath
    {
    public:
        DataPath();
        ~DataPath();
        
        static const File defaultSettingsFile();
    };
}


#endif  // EL_DATAPATH_H
