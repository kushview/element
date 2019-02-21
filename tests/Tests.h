#pragma once

#include "JuceHeader.h"
#include "ElementApp.h"
#include "controllers/AppController.h"
#include "controllers/SessionController.h"
#include "engine/AudioEngine.h"
#include "engine/GraphProcessor.h"
#include "engine/MappingEngine.h"
#include "engine/InternalFormat.h"
#include "engine/PlaceholderProcessor.h"
#include "engine/VolumeProcessor.h"
#include "engine/VelocityCurve.h"
#include "engine/SubGraphProcessor.h"
#include "session/PluginManager.h"
#include "session/PluginManager.h"
#include "session/Session.h"
#include "session/UnlockStatus.h"
#include "Globals.h"
#include "Settings.h"

namespace Element 
{
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
            world.reset (new Globals ());
            world->setEngine (new AudioEngine (*world));
            world->getPluginManager().addDefaultFormats();
            world->getPluginManager().addFormat (new ElementAudioPluginFormat (*world));
            world->getPluginManager().addFormat (new InternalFormat (*world->getAudioEngine()));
            app.reset (new AppController (*world));
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

        const File getDataDir() const
        {
            const auto thedir = File::getSpecialLocation (File::invokedExecutableFile)
                .getParentDirectory().getParentDirectory().getParentDirectory()
                .getChildFile("data");
            jassert (thedir.exists());
            return thedir;
        }

        void runDispatchLoop (const int millisecondsToRunFor = 40) { MessageManager::getInstance()->runDispatchLoopUntil (millisecondsToRunFor); }
        
        Globals& getWorld() { initializeWorld(); return *world; }
        AppController& getAppController() { initializeWorld(); return *app; }

    private:
        const String slug;
        std::unique_ptr<Globals> world;
        std::unique_ptr<AppController> app;
    };
}
