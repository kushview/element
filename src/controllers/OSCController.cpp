/*
    This file is part of Element
    Copyright (C) 2019-2021  Kushview, LLC.  All rights reserved.

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

#include "controllers/OSCController.h"
#include "session/commandmanager.hpp"
#include "session/devicemanager.hpp"
#include "commands.hpp"
#include "context.hpp"
#include "settings.hpp"

#define EL_OSC_ADDRESS_COMMAND "/element/command"
#define EL_OSC_ADDRESS_ENGINE "/element/engine"

namespace element {

struct CommandOSCListener final : OSCReceiver::ListenerWithOSCAddress<>
{
    CommandOSCListener (Context& w)
        : world (w)
    {
    }

    void oscMessageReceived (const OSCMessage& message) override
    {
        const auto msg = message[0];
        if (! msg.isString())
            return;

        const auto command = Commands::fromString (msg.getString());
        if (command != Commands::invalid)
        {
            world.getCommandManager().invokeDirectly (
                Commands::quit, true);
        }
    }

private:
    Context& world;
};

//=============================================================================
struct EngineOSCListener final : OSCReceiver::ListenerWithOSCAddress<>
{
    EngineOSCListener (Context& g)
        : globals (g)
    {
    }

    void oscMessageReceived (const OSCMessage& message) override
    {
        const auto slug = message[0];
        if (! slug.isString())
            return;

        if (message.size() >= 2 && slug.getString().toLowerCase().trim() == "samplerate")
            handleSampleRate (message[1]);
    }

private:
    Context& globals;

    void handleSampleRate (const OSCArgument& arg)
    {
        double sampleRate = 0.0;
        switch (arg.getType())
        {
            // see juce_OSCTypes.cpp for other types
            case 'i':
                sampleRate = (double) arg.getInt32();
                break;
            case 'f':
                sampleRate = (double) roundToInt (arg.getFloat32());
                break;
            default:
                break;
        }

        if (sampleRate <= 0.0)
            return;

        auto& devs = globals.getDeviceManager();
        auto setup = devs.getAudioDeviceSetup();
        if (sampleRate != setup.sampleRate)
        {
            setup.sampleRate = sampleRate;
            devs.setAudioDeviceSetup (setup, true);
        }
    }
};

//=============================================================================
class OSCController::Impl
{
public:
    Impl (OSCController& o)
        : owner (o) {}
    ~Impl() {}

    bool startServer()
    {
        if (isServing())
            return true;
        serving = receiver.connect (serverPort);
        return serving;
    }

    bool stopServer()
    {
        if (! isServing())
            return true;
        const auto result = receiver.disconnect();
        if (result)
            serving = false;
        return result;
    }

    bool isServing() const { return serving; }

    void setServerPort (int newPort)
    {
        if (newPort == serverPort)
            return;
        const auto wasServing = isServing();
        stopServer();
        serverPort = newPort;
        if (wasServing)
            startServer();
    }

    void initialize()
    {
        if (listenersReady == true)
            return;

        application.reset (new CommandOSCListener (owner.getWorld()));
        receiver.addListener (application.get(), EL_OSC_ADDRESS_COMMAND);

        engine.reset (new EngineOSCListener (owner.getWorld()));
        receiver.addListener (engine.get(), EL_OSC_ADDRESS_ENGINE);

        listenersReady = true;
    }

    void shutdown()
    {
        if (! listenersReady)
            return;
        listenersReady = false;

        receiver.removeListener (application.get());
        receiver.removeListener (engine.get());

        application.reset();
        engine.reset();
    }

    int getHostPort() const { return serverPort; }

private:
    OSCController& owner;
    OSCSender sender;
    OSCReceiver receiver { "elosc" };

    bool listenersReady = false;
    bool serving { false };
    int serverPort { 9000 };

    std::unique_ptr<CommandOSCListener> application;
    std::unique_ptr<EngineOSCListener> engine;
};

//=============================================================================

OSCController::OSCController()
{
    impl.reset (new Impl (*this));
}

OSCController::~OSCController()
{
    impl.reset();
}

void OSCController::refreshWithSettings (bool alertOnFail)
{
    auto& settings = getWorld().getSettings();
    impl->stopServer();
    impl->setServerPort (settings.getOscHostPort());

    if (settings.isOscHostEnabled())
    {
        if (! impl->startServer() && alertOnFail)
        {
            String msg = "Could not start OSC host on port ";
            msg << impl->getHostPort();
            AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                              "OSC Host",
                                              msg);
        }
    }
}

void OSCController::activate()
{
    impl->initialize();
    refreshWithSettings (false);
}

void OSCController::deactivate()
{
    impl->stopServer();
    impl->shutdown();
}

} // namespace element
