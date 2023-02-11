/*
    PortCount.h - This file is part of Element
    Copyright (C) 2021 Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <element/porttype.hpp>

namespace element {

// FIXME:
using namespace juce;

struct PortCount
{
    PortCount()
    {
        clear();
    }

    PortCount (const PortCount& o)
    {
        operator= (o);
    }

    PortCount& operator= (const PortCount& o)
    {
        memcpy (inputs, o.inputs, PortType::Unknown * sizeof (int));
        memcpy (outputs, o.outputs, PortType::Unknown * sizeof (int));
        return *this;
    }

    bool operator== (const PortCount& o)
    {
        for (int i = 0; i < PortType::Unknown; ++i)
            if (inputs[i] != o.inputs[i] || outputs[i] != o.outputs[i])
                return false;
        return true;
    }

    void clear()
    {
        memset (inputs, 0, PortType::Unknown * sizeof (int));
        memset (outputs, 0, PortType::Unknown * sizeof (int));
    }

    int get (PortType type, bool isInput) const
    {
        return isInput ? inputs[type.id()] : outputs[type.id()];
    }

    void set (PortType type, int count, bool input)
    {
        auto& counts = input ? inputs : outputs;
        counts[type.id()] = count;
    }

    void set (PortType type, int numIns, int numOuts)
    {
        set (type, numIns, true);
        set (type, numOuts, false);
    }

    PortCount with (PortType type, int count, bool isInput)
    {
        auto ret = *this;
        ret.set (type, count, isInput);
        return ret;
    }

    PortCount with (PortType type, int numIns, int numOuts)
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
        for (int i = 0; i < PortType::Unknown; ++i)
        {
            PortType pt (i);
            const auto symPrefix = pt.getSlug();
            const auto namePrefix = pt.getName();

            for (int j = 0; j < inputs[i]; ++j)
            {
                String symbol = symPrefix;
                symbol << "_in_" << String (j + 1);
                String name = namePrefix;
                name << " In " << int (j + 1);

                ports.add (i, index++, j, symbol, name, true);
            }

            for (int j = 0; j < outputs[i]; ++j)
            {
                String symbol = symPrefix;
                symbol << "_out_" << String (j + 1);
                String name = namePrefix;
                name << " Out " << int (j + 1);

                ports.add (i, index++, j, symbol, name, false);
            }
        }
    }

private:
    int inputs[PortType::Unknown];
    int outputs[PortType::Unknown];
};

} // namespace element
