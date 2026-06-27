// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/processor.hpp>
#include "engine/mappingengine.hpp"
#include "engine/mappingtarget.hpp"
#include "engine/midiengine.hpp"
#include <element/midimapping.hpp>
#include <element/node.hpp>
#include <element/session.hpp>

namespace element {

//=============================================================================
struct MappingEngine::Binding
{
    MidiMapping mapping;
    std::unique_ptr<MappingTarget> target;
};

/** Receives MIDI from all inputs on the MIDI thread and marshals each message
    to the message thread before routing through MappingEngine::process(). */
class MappingEngine::Router : public juce::MidiInputCallback,
                              public juce::AsyncUpdater
{
public:
    explicit Router (MappingEngine& e) : engine (e) {}
    ~Router() override { cancelPendingUpdate(); }

    void handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message) override
    {
        if (! message.isController() && ! message.isNoteOnOrOff())
            return;
        const juce::String id = source != nullptr ? source->getIdentifier() : juce::String();
        {
            juce::ScopedLock sl (lock);
            queue.add ({ id, message });
        }
        triggerAsyncUpdate();
    }

    void handleAsyncUpdate() override
    {
        juce::Array<Item> local;
        {
            juce::ScopedLock sl (lock);
            local.swapWith (queue);
        }
        for (auto& item : local)
            engine.process (item.device, item.message);
    }

private:
    struct Item
    {
        juce::String device;
        juce::MidiMessage message;
    };

    MappingEngine& engine;
    juce::CriticalSection lock;
    juce::Array<Item> queue;
};

//=============================================================================
MappingEngine::MappingEngine() {}
MappingEngine::~MappingEngine()
{
    bindings.clear();
    router = nullptr;
}

void MappingEngine::rebuildBindings (SessionPtr session)
{
    bindings.clear();
    if (session == nullptr)
        return;

    for (int i = 0; i < session->getNumMidiMappings(); ++i)
    {
        auto mapping = session->getMidiMapping (i);
        auto target = createTarget (mapping, *session);
        if (target == nullptr)
            continue;
        auto binding = std::make_unique<Binding>();
        binding->mapping = mapping;
        binding->target = std::move (target);
        bindings.push_back (std::move (binding));
    }
}

void MappingEngine::process (const juce::String& device, const juce::MidiMessage& message)
{
    if (mapCapture.get())
    {
        mapCapture.set (false);
        capturedDevice = device;
        capturedMessage = message;
        mapCapturedCallback();
        return;
    }

    for (auto& binding : bindings)
    {
        const auto& mapping = binding->mapping;
        if (! mapping.getDevice().isEmpty() && mapping.getDevice() != device)
            continue;
        if (mapping.matches (message) && binding->target != nullptr)
            binding->target->apply (message, mapping.isToggle());
    }
}

void MappingEngine::startListening (MidiEngine& midi)
{
    if (router == nullptr)
        router.reset (new Router (*this));
    midi.addMidiInputCallback (router.get(), true);
}

void MappingEngine::stopListening (MidiEngine& midi)
{
    if (router != nullptr)
    {
        midi.removeMidiInputCallback (router.get());
        router->cancelPendingUpdate();
    }
}

} // namespace element
