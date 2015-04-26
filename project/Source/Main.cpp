/*
    Main.cpp - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "JuceHeader.h"
#include "engine/AudioEngine.h"
#include "engine/InternalFormat.h"
#include "session/Session.h"
#include "gui/Alerts.h"
#include "gui/GuiApp.h"
#include "Globals.h"


namespace juce {
    extern void initEGL();
}

namespace Element {

class Application  : public JUCEApplication, public Timer
{

    Scoped<Globals>     world;
    Shared<AudioEngine> engine;
    Scoped<Gui::GuiApp> gui;

public:

   Application() { }

   const String getApplicationName()       { return "Element"; }
   const String getApplicationVersion()    { return ProjectInfo::versionString; }
   bool moreThanOneInstanceAllowed()       { return true; }


   void initialise (const String& /* commandLine */)
   {
       world = new Globals();

       Settings& settings (world->settings());
       if (ScopedXml dxml = settings.getUserSettings()->getXmlValue ("devices"))
            world->devices().initialise (16, 16, dxml.get(), true, "default", nullptr);

       engine.reset (new AudioEngine (*world));
       world->setEngine (engine); // this will also instantiate the session

       // global data is ready, so now we can start using it;
       PluginManager& plugins (world->plugins());
       plugins.addDefaultFormats();
       plugins.addFormat (new InternalFormat (*engine));
       plugins.restoreUserPlugins (settings);

       gui = Gui::GuiApp::create (*world);

       engine->activate();
       gui->run();

       Logger::writeToLog ("Gui is running...");
   }


   void timerCallback()
   {
        static bool hasQuit = false;
        if (! hasQuit)
        {
            hasQuit = true;
            quit();
        }
   }

   void shutdown()
   {
       if (gui != nullptr)
           gui = nullptr;

       PluginManager& plugins (world->plugins());
       Settings& settings (world->settings());
       plugins.saveUserPlugins (settings);

       if (ScopedXml el = world->devices().createStateXml())
           settings.getUserSettings()->setValue ("devices", el);

       engine->deactivate();
       world->setEngine (Shared<Engine>());
       engine.reset();

       std::clog << "Going away with " << engine.use_count() << " AudioEngine refs out there\n";
       world = nullptr;
   }

   //==============================================================================
   void
   systemRequestedQuit()
   {
       if (gui->shutdownApp())
       {
           gui = nullptr;
           this->quit();
       }
   }

   void
   anotherInstanceStarted (const String& commandLine)
   {

   }

};
}

START_JUCE_APPLICATION (Element::Application)
