/*
    PluginManager.cpp - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
*/

#include "session/PluginManager.h"

namespace Element {

class PluginManager::Private
{
public:
    Private()
        : sampleRate (44100.0f),
          blockSize (512)
    {  }

    ~Private() {  }

    KnownPluginList allPlugins;
    AudioPluginFormatManager formats;

   #if ELEMENT_LV2_PLUGIN_HOST
    OptionalPtr<LV2World> lv2;
    OptionalPtr<SymbolMap> symbols;
   #endif

    double sampleRate;
    int    blockSize;
};

PluginManager::PluginManager()
{
    priv = new Private();
   #if ELEMENT_LV2_PLUGIN_HOST
    priv->symbols.setOwned (new SymbolMap ());
    priv->lv2.setOwned (new LV2World());
   #endif
}

PluginManager::~PluginManager()
{
    priv = nullptr;
}

void PluginManager::addDefaultFormats()
{
    formats().addDefaultFormats();
   #if ELEMENT_LV2_PLUGIN_HOST
    addFormat (new LV2PluginFormat (*priv->lv2));
   #endif
}

void PluginManager::addFormat (AudioPluginFormat* fmt)
{
    formats().addFormat (fmt);
}

AudioPluginInstance* PluginManager::createAudioPlugin (const PluginDescription& desc, String& errorMsg)
{
    return formats().createPluginInstance (desc, priv->sampleRate, priv->blockSize, errorMsg);
}

Processor* PluginManager::createPlugin (const PluginDescription &desc, String &errorMsg)
{
    if (AudioPluginInstance* instance = createAudioPlugin (desc, errorMsg))
        return dynamic_cast<Processor*> (instance);
    return nullptr;
}

AudioPluginFormat* PluginManager::format (const String& name)
{
    for (int i = 0; i < formats().getNumFormats(); ++i)
    {
        AudioPluginFormat* fmt = priv->formats.getFormat (i);
        if (fmt && fmt->getName() == name)
            return fmt;
    }

    return nullptr;
}

AudioPluginFormatManager& PluginManager::formats()
{
    return priv->formats;
}


KnownPluginList& PluginManager::availablePlugins()
{
    return priv->allPlugins;
}

void PluginManager::saveUserPlugins (ApplicationProperties& settings)
{
    ScopedXml elm (priv->allPlugins.createXml());
    settings.getUserSettings()->setValue ("plugin-list", elm.get());
}

void PluginManager::setPlayConfig (double sampleRate, int blockSize)
{
    priv->sampleRate = sampleRate;
    priv->blockSize  = blockSize;
}

void PluginManager::restoreUserPlugins (ApplicationProperties& settings)
{
    if (ScopedXml xml = settings.getUserSettings()->getXmlValue ("plugin-list"))
        restoreUserPlugins (*xml);
}

void PluginManager::restoreUserPlugins (const XmlElement& xml)
{
    priv->allPlugins.recreateFromXml (xml);
}

}
