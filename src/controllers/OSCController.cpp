/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

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
#include "session/CommandManager.h"
#include "Commands.h"
#include "Globals.h"

#define EL_OSC_ADDRESS_COMMAND "/element/command"

namespace Element {

struct CommandOSCListener final : OSCReceiver::ListenerWithOSCAddress<>
{       
    CommandOSCListener (Globals& w)
        : world (w) 
    { }

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
    Globals& world;
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

        listenersReady = true;
    }

    void shutdown()
    {
        if (! listenersReady)
            return;
        listenersReady = false;

        receiver.removeListener (application.get());
        application.reset();
    }

private:
    OSCController& owner;
    OSCSender sender;
    OSCReceiver receiver { "elosc" };

    bool listenersReady = false;
    bool serving { false };
    int serverPort { 9000 };

    std::unique_ptr<CommandOSCListener> application;
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

void OSCController::activate()
{
    impl->initialize();
    impl->startServer();
}

void OSCController::deactivate()
{
    impl->stopServer();
    impl->shutdown();
}

}
