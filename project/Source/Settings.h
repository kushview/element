/*
    Settings.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
*/

#ifndef EL_SETTINGS_H
#define EL_SETTINGS_H

#include "ElementApp.h"

namespace Element {

class Settings :  public ApplicationProperties
{
public:
    Settings();
    ~Settings();
    
    XmlElement* getLastGraph() const;
    void setLastGraph (const ValueTree& data);

private:
    PropertiesFile* getProps() const;
};

}

#endif // EL_SETTINGS_H
