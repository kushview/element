/*
    WorldBase.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#ifndef ELEMENT_WORLD_BASE_H
#define ELEMENT_WORLD_BASE_H

#include "ElementApp.h"

namespace Element {

/** A global collection of an appilcation/plugin's data */
class WorldBase {
public:
    explicit WorldBase (void* host = nullptr);
    virtual ~WorldBase();

    virtual bool loadModule (const char* moduleName);
    virtual int executeModule (const char* entryModule);

    void setAppName (const String& name) { appName = name; }
    virtual const String& getAppName() const { return appName; }

protected:
    void unloadModules();

private:
    class Private;
    Scoped<Private> priv;
    String appName;
};

}

#endif // ELEMENT_WORLD_H
