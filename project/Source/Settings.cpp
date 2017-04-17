/*
    Settings.cpp - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
*/

#include "Settings.h"

namespace Element {
Settings::Settings()
{
    PropertiesFile::Options opts;
    opts.applicationName     = "Element";
    opts.filenameSuffix      = "conf";
    opts.osxLibrarySubFolder = "Application Support";
    opts.storageFormat       = PropertiesFile::storeAsXML;

   #if JUCE_LINUX
    opts.folderName          = ".config/element";
   #else
    opts.folderName          = opts.applicationName;
   #endif

    setStorageParameters (opts);
}

Settings::~Settings() { }
}
