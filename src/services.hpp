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

#pragma once

#include "session/commandmanager.hpp"
#include <element/runmode.hpp>

namespace element {

class Context;
class Settings;
struct AppMessage;
class ServiceManager;

class Service
{
public:
    Service() {}
    virtual ~Service()
    {
        owner = nullptr;
    }

    template <class T>
    inline T* findSibling() const;

    virtual void initialize() {}
    virtual void activate() {}
    virtual void deactivate() {}
    virtual void shutdown() {}
    virtual void saveSettings() {}

    ServiceManager& getServices() const;
    Settings& getSettings();
    Context& getWorld();
    RunMode getRunMode() const;

protected:
    virtual bool handleMessage (const AppMessage&) { return false; }

private:
    friend class ServiceManager;
    ServiceManager* owner = nullptr;
};

//=============================================================================
class ServiceManager : public MessageListener,
                       protected ApplicationCommandTarget
{
public:
    ServiceManager (Context&, RunMode mode = RunMode::Standalone);
    ~ServiceManager();

    /** Returns the running mode of this instance */
    RunMode getRunMode() const { return runMode; }

    /** Access to global data */
    inline Context& getWorld() { return getGlobals(); }

    /** Alias of getWorld() */
    inline Context& getGlobals() { return world; }

    /** Returns the undo manager */
    inline UndoManager& getUndoManager() { return undo; }

    /** Add a service */
    void addChild (Service* service)
    {
        service->owner = this;
        services.add (service);
    }

    template <class T>
    inline T* findChild() const
    {
        for (auto const* c : services)
            if (T* t = const_cast<T*> (dynamic_cast<const T*> (c)))
                return t;
        return nullptr;
    }

    inline void saveSettings()
    {
        for (auto* s : services)
            s->saveSettings();
    }

    /** Child controllers should use this when files are opened and need
        to be saved in recent files.
    */
    inline void addRecentFile (const File& file) { recentFiles.addFile (file); }

    /** Activate this and children */
    void activate();

    /** Deactivate this and children */
    void deactivate();

    RecentlyOpenedFilesList& getRecentlyOpenedFilesList() { return recentFiles; }

protected:
    friend class ApplicationCommandTarget;
    ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands (Array<CommandID>& commands) override;
    void getCommandInfo (CommandID commandID, ApplicationCommandInfo& result) override;
    bool perform (const InvocationInfo& info) override;

    void handleMessage (const Message&) override;

private:
    friend class Application;
    OwnedArray<Service> services;
    File lastSavedFile;
    File lastExportedGraph;
    Context& world;
    RecentlyOpenedFilesList recentFiles;
    UndoManager undo;
    RunMode runMode;

    void run();
};

template <class T>
inline T* Service::findSibling() const
{
    return (owner != nullptr) ? owner->findChild<T>() : nullptr;
}

} // namespace element
