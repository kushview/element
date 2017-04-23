
#ifndef DATAPATH_H_INCLUDED
#define DATAPATH_H_INCLUDED

#include "ElementApp.h"

namespace Element {
    class DataPath {
    public:
        DataPath();
        ~DataPath();
        
        static const File defaultSettingsFile();
    };
}


#endif  // DATAPATH_H_INCLUDED
