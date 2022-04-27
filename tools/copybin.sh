#!/bin/bash
# run this when adding binary resources or changing jucer projects
set -x
cp -fv tools/jucer/Standalone/JuceLibraryCode/BinaryData.h libs/compat/BinaryData.h
cp -fv tools/jucer/Standalone/JuceLibraryCode/BinaryData.cpp libs/compat/BinaryData.cpp
cp -fv tools/jucer/Standalone/JuceLibraryCode/AppConfig.h libs/compat/AppConfig.h
cp -fv tools/jucer/Standalone/JuceLibraryCode/JuceHeader.h libs/compat/JuceHeader.h
cp -fv tools/jucer/Element/JuceLibraryCode/JucePluginDefines.h libs/compat/JucePluginDefines.h
cp -fv tools/jucer/Standalone/Builds/VisualStudio2019/resources.rc tools/windeploy/resources.rc
cp -fv tools/jucer/Standalone/Builds/VisualStudio2019/icon.ico tools/windeploy/icon.ico
