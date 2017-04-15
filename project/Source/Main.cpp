/*
    Main.cpp - This file is part of Element
    Copyright (C) 2016-2017 Kushview, LLC.  All rights reserved.
*/

#include "ElementApp.h"

#include "controllers/AppController.h"
#include "Commands.h"
#include "Globals.h"

namespace Element {

class StartupThread : public Thread
{
public:
    StartupThread (Globals& w)
        : Thread ("element_startup"),
          world (w)
    { }

    ~StartupThread() { }

    void run() override
    {
#if 0
        Settings& settings (world.settings());
        if (ScopedXml dxml = settings.getUserSettings()->getXmlValue ("devices"))
             world.devices().initialise (16, 16, dxml.get(), true, "default", nullptr);

        AudioEnginePtr engine = new AudioEngine (world);
        world.setEngine (engine); // this will also instantiate the session

        // global data is ready, so now we can start using it;
        PluginManager& plugins (world.plugins());
        plugins.addDefaultFormats();
        plugins.addFormat (new InternalFormat (*engine));
        plugins.restoreUserPlugins (settings);

        engine->activate();

        world.loadModule ("test");
        controller = new AppController (world);
#endif
    }

    void launchApplication (const bool useThread = false)
    {
        if (world.cli.fullScreen)
        {
            Desktop::getInstance().setKioskModeComponent (&screen, false);
            screen.setVisible (true);
        }
        
        if (useThread)
        {
            startThread();
            while (isThreadRunning())
                MessageManager::getInstance()->runDispatchLoopUntil (30);
        }
        else
        {
            this->run();
        }

        if (screen.isOnDesktop())
            screen.removeFromDesktop();
    }

    ScopedPointer<AppController> controller;

private:
    class StartupScreen :  public TopLevelWindow
    {
    public:
        StartupScreen()
            : TopLevelWindow ("startup", true)
        {
            text.setText ("Loading Application", dontSendNotification);
            text.setSize (100, 100);
            text.setFont (Font (24.0f));
            text.setJustificationType (Justification::centred);
            text.setColour (Label::textColourId, Colours::white);
            addAndMakeVisible (text);
            centreWithSize (text.getWidth(), text.getHeight());
        }

        void resized() override {
            text.setBounds (getLocalBounds());
        }

        void paint (Graphics& g) override {
            g.fillAll (Colours::transparentBlack);
        }

    private:
        Label text;
    } screen;

    Globals& world;
};

class Application  : public JUCEApplication
{
public:
    Application() { }
    virtual ~Application() { }

    const String getApplicationName()    override      { return ProjectInfo::projectName; }
    const String getApplicationVersion() override      { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed()    override      { return true; }

    void initialise (const String&  commandLine ) override
    {
        initializeModulePath();
        world = new Globals (commandLine);
        launchApplication();
    }

    
    void shutdown() override
    {
#if 0
        if (gui != nullptr) {
            PluginWindow::closeAllCurrentlyOpenWindows();
            gui = nullptr;
        }

        PluginManager& plugins (world->plugins());
        Settings& settings (world->settings());

        if (ScopedXml el = world->devices().createStateXml())
            settings.getUserSettings()->setValue ("devices", el);

        engine->deactivate();
        world->setEngine (nullptr);
        engine = nullptr;
#endif
        controller = nullptr;
        world->unloadModules();
        world = nullptr;
    }

    void systemRequestedQuit() override
    {
        Application::quit();
    }

    void anotherInstanceStarted (const String& /*commandLine*/) override
    {
    
    }

    bool perform (const InvocationInfo& info) override
    {
        switch (info.commandID) {
            case Commands::quit: {
                this->systemRequestedQuit();
            } break;
        }

        return true;
    }

private:
    ScopedPointer<Globals>       world;
    ScopedPointer<AppController> controller;
    
    void launchApplication()
    {
        if (nullptr != controller)
            return;
        
        controller = new AppController (*world);
        controller->run();
    }
    
    void initializeModulePath()
    {
        const File path (File::getSpecialLocation (File::invokedExecutableFile));
        File modDir = path.getParentDirectory().getParentDirectory()
                          .getChildFile("lib/element").getFullPathName();
       #if JUCE_DEBUG
        if (! modDir.exists()) {
            modDir = path.getParentDirectory().getParentDirectory()
            .getChildFile ("modules");
        }
       #endif
        
       #if JUCE_WINDOWS
        String putEnv = "ELEMENT_MODULE_PATH="; putEnv << modDir.getFullPathName();
        putenv (putEnv.toRawUTF8());
       #else
        setenv ("ELEMENT_MODULE_PATH", modDir.getFullPathName().toRawUTF8(), 1);
        Logger::writeToLog (String("[element] module path: ") + String(getenv ("ELEMENT_MODULE_PATH")));
       #endif
    }
};

}

START_JUCE_APPLICATION (Element::Application)
