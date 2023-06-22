// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/data_structures.hpp>
#include <element/juce/gui_basics.hpp>

#include <juce_gui_extra/juce_gui_extra.h>

#include <element/runmode.hpp>

namespace element {

class Context;
class Settings;
struct AppMessage;
class Services;

/** Provides some kind of high level support for something in Element. */
class Service {
public:
    Service() {}
    virtual ~Service()
    {
        owner = nullptr;
    }

    /** Initialize the service. */
    virtual void initialize() {}
    /** Activate the service. */
    virtual void activate() {}
    /** Deactivate the service. */
    virtual void deactivate() {}
    /** Shutdown the service. */
    virtual void shutdown() {}
    /** Save service settings. */
    virtual void saveSettings() {}

    /** Locate a sibling service by type. */
    template <class T>
    inline T* sibling() const;

    Services& services() const;
    Settings& settings();
    Context& context();
    RunMode getRunMode() const;

protected:
    virtual bool handleMessage (const AppMessage&) { return false; }

private:
    friend class Services;
    Services* owner = nullptr;
};

//=============================================================================
class Services : public juce::MessageListener {
public:
    Services (Context&, RunMode mode = RunMode::Standalone);
    ~Services();

    /** Returns the running mode of this instance */
    RunMode getRunMode() const;

    /** Alias of context() */
    Context& context();

    /** Add a service */
    void add (Service* service);

    /** Find a service by type */
    template <class T>
    inline T* find() const
    {
        for (auto* c : *this)
            if (T* t = const_cast<T*> (dynamic_cast<const T*> (c)))
                return t;
        return nullptr;
    }

    void saveSettings();

    /** Activate this and children */
    void initialize();

    /** Activate this and children */
    void activate();

    /** Deactivate this and children */
    void deactivate();

    void launch();

    void shutdown();

    Service** begin() noexcept;
    Service* const* begin() const noexcept;
    Service** end() noexcept;
    Service* const* end() const noexcept;

    // protected:
    //     friend class juce::ApplicationCommandTarget;
    //     juce::ApplicationCommandTarget* getNextCommandTarget() override;
    //     void getAllCommands (juce::Array<juce::CommandID>& commands) override;
    //     void getCommandInfo (juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    //     bool perform (const InvocationInfo& info) override;

    void handleMessage (const juce::Message&) override;

private:
    friend class Application;
    friend class Context;
    class Impl;
    std::unique_ptr<Impl> impl;

    /** Run/Launch the core application. */
    void run();
};

template <class T>
inline T* Service::sibling() const
{
    return (owner != nullptr) ? owner->find<T>() : nullptr;
}

} // namespace element
