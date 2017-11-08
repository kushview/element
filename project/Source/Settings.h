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
    
    static const char* checkForUpdatesKey;
	static const char* pluginListKey;
	static const char* pluginListKey64;
    
    XmlElement* getLastGraph() const;
    void setLastGraph (const ValueTree& data);
    bool checkForUpdates() const;
    
private:
    PropertiesFile* getProps() const;
};

}
