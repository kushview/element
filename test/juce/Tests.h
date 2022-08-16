/*
    This file is part of Element
    Copyright (C) 2018-2019  Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "JuceHeader.h"
#include "ElementApp.h"
#include "services.hpp"
#include "services/sessionservice.hpp"

#include "engine/audioengine.hpp"
#include "engine/mappingengine.hpp"
#include "engine/internalformat.hpp"
#include "engine/LinearFade.h"
#include "engine/velocitycurve.hpp"
#include "engine/ToggleGrid.h"
#include "engine/nodes/PlaceholderProcessor.h"
#include "engine/nodes/VolumeProcessor.h"

#include "engine/graphnode.hpp"
#include "engine/ionode.hpp"

#include "session/PluginManager.h"
#include "session/PluginManager.h"
#include "session/Session.h"
#include "Globals.h"
#include "Settings.h"
#include "common.hpp"

namespace element  {

class UnitTestBase : public UnitTest
{
public:
    UnitTestBase (const String& name, const String& category = String(), 
                    const String& _slug = String())
        : UnitTest (name, category), slug (_slug) { }
    
    virtual ~UnitTestBase()
    {
        if (world)
        {
            jassertfalse;
            shutdownWorld();
        }
    }

    const String getId() const { return getCategory().toLowerCase() + "." + getSlug().toLowerCase(); }
    const String& getSlug() const { return slug; }

protected:
    void initializeWorld()
    {
        if (world) return;
        world.reset (new Context ());
        world->setEngine (new AudioEngine (*world));
        world->getPluginManager().addDefaultFormats();
        world->getPluginManager().addFormat (new ElementAudioPluginFormat (*world));
        world->getPluginManager().addFormat (new InternalFormat (*world->getAudioEngine(), world->getMidiEngine()));
        app.reset (new ServiceManager (*world));
        app->activate();
        auto& settings = getWorld().getSettings();
        PropertiesFile::Options opts = settings.getStorageParameters();
        opts.applicationName = "ElementTests";
        settings.setStorageParameters (opts);
        settings.setCheckForUpdates (false);
        settings.saveIfNeeded();
    }

    void shutdownWorld()
    {
        if (! world) return;
        app->deactivate();
        app.reset (nullptr);
        world->setEngine (nullptr);
        world.reset (nullptr);
    }

    const File getTestsDir() const
    {
        const auto thedir = File::getCurrentWorkingDirectory().getChildFile ("tests");
        jassert (thedir.exists());
        return thedir;
    }

    const File getDataDir() const
    {
        const auto thedir = File::getCurrentWorkingDirectory().getChildFile ("build/data");
        jassert (thedir.exists());
        return thedir;
    }

    void runDispatchLoop (const int millisecondsToRunFor = 40)
    { 
        MessageManager::getInstance()->runDispatchLoopUntil (millisecondsToRunFor); 
    }
    
    Context& getWorld() { initializeWorld(); return *world; }
    ServiceManager& getServices() { initializeWorld(); return *app; }

private:
    const String slug;
    std::unique_ptr<Context> world;
    std::unique_ptr<ServiceManager> app;
};

}
