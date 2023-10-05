// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <lv2/atom/atom.h>
#include <lv2/event/event.h>

#include <element/juce/core.hpp>
#include <element/porttype.hpp>

namespace element {

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wdeprecated-declarations")

class PortBuffer final
{
public:
    using uint32 = juce::uint32;
    using int64 = juce::int64;
    PortBuffer (bool inputPort, uint32 portType, uint32 dataType, uint32 bufferSize);
    ~PortBuffer();

    void clear();
    void reset();

    bool addEvent (int64 frames, uint32 size, uint32 type, const uint8* data);

    inline uint32 getCapacity() const { return capacity; }
    void* getPortData() const;

    inline uint32 getType() const { return type; }

    inline bool isAtom() const noexcept { return type == PortType::Atom; }
    inline bool isAudio() const noexcept { return type == PortType::Audio; }
    inline bool isCV() const noexcept { return type == PortType::CV; }
    inline bool isControl() const noexcept { return type == PortType::Control; }
    inline bool isEvent() const noexcept { return type == PortType::Event; }
    inline bool isSequence() const noexcept { return isAtom(); }

    void referTo (void* location)
    {
        buffer.referred = location;
        referenced = true;
    }

    float getValue() const;
    void setValue (float value);

    inline constexpr bool isInput() const noexcept { return input; }

private:
    uint32_t type = 0;
    uint32_t capacity = 0;
    uint32_t bufferType = 0;
    bool input = true;

    std::unique_ptr<uint8[]> data;
    bool referenced = false;

    juce::Atomic<float> value;

    union
    {
        void* referred;
        float* control;
        float* audio;
        float* cv;
        LV2_Atom* atom;
        LV2_Event_Buffer* event;
    } buffer;
};

JUCE_END_IGNORE_WARNINGS_GCC_LIKE

} // namespace element
