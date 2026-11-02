// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>

#include <element/porttype.hpp>

namespace element {

struct PortCount {
    PortCount() {}

    bool operator== (const PortCount& o) const noexcept
    {
        for (int i = 0; i < PortType::Unknown; ++i)
            if (inputs[i] != o.inputs[i] || outputs[i] != o.outputs[i])
                return false;
        return true;
    }

    void clear() noexcept
    {
        inputs.fill (0);
        outputs.fill (0);
    }

    int get (PortType type, bool isInput) const noexcept
    {
        return isInput ? inputs[type.id()] : outputs[type.id()];
    }

    void set (PortType type, int count, bool input) noexcept
    {
        auto& counts = input ? inputs : outputs;
        counts[type.id()] = count;
    }

    void set (PortType type, int numIns, int numOuts) noexcept
    {
        set (type, numIns, true);
        set (type, numOuts, false);
    }

    PortCount with (PortType type, int count, bool isInput) const noexcept
    {
        auto ret = *this;
        ret.set (type, count, isInput);
        return ret;
    }

    PortCount with (PortType type, int numIns, int numOuts) noexcept
    {
        auto ret = *this;
        ret.set (type, numIns, numOuts);
        return ret;
    }

    PortList toPortList() const
    {
        PortList ports;
        getPorts (ports);
        return ports;
    }

    void getPorts (PortList& ports) const
    {
        uint32_t index = 0;
        for (int i = 0; i < PortType::Unknown; ++i) {
            PortType pt (i);
            const auto symPrefix = pt.getSlug();
            const auto namePrefix = pt.getName();

            for (int j = 0; j < inputs[i]; ++j) {
                juce::String symbol = symPrefix;
                symbol << "_in_" << juce::String (j + 1);
                juce::String name = namePrefix;
                name << " In " << int (j + 1);

                ports.add (i, index++, j, symbol, name, true);
            }

            for (int j = 0; j < outputs[i]; ++j) {
                juce::String symbol = symPrefix;
                symbol << "_out_" << juce::String (j + 1);
                juce::String name = namePrefix;
                name << " Out " << int (j + 1);

                ports.add (i, index++, j, symbol, name, false);
            }
        }
    }

    std::array<int, PortType::Unknown> inputs {};
    std::array<int, PortType::Unknown> outputs {};
};

} // namespace element
