/*
    Settings.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
*/

#pragma once

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
