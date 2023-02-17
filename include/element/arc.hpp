/*
    This file is part of Element
    Copyright (c) 2014-2023  Kushview, LLC.  All rights reserved.

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

#include <cstdint>

#include <element/juce/core.hpp>

namespace element {

struct JUCE_API Arc {
public:
    Arc (uint32_t sourceNode, uint32_t sourcePort, uint32_t destNode, uint32_t destPort) noexcept;
    virtual ~Arc() {}

    /** The source node ID */
    uint32_t sourceNode;

    /** The source port index */
    uint32_t sourcePort;

    /** The destination node ID */
    uint32_t destNode;

    /** The destination port index */
    uint32_t destPort;

    inline Arc& operator= (const Arc& o)
    {
        this->sourceNode = o.sourceNode;
        this->sourcePort = o.sourcePort;
        this->destNode = o.destNode;
        this->destPort = o.destPort;
        return *this;
    }

private:
    JUCE_LEAK_DETECTOR (Arc)
};

struct ArcSorter {
    static inline int compareElements (const Arc* const first, const Arc* const second) noexcept
    {
        if (first->sourceNode < second->sourceNode)
            return -1;
        if (first->sourceNode > second->sourceNode)
            return 1;
        if (first->destNode < second->destNode)
            return -1;
        if (first->destNode > second->destNode)
            return 1;
        if (first->sourcePort < second->sourcePort)
            return -1;
        if (first->sourcePort > second->sourcePort)
            return 1;
        if (first->destPort < second->destPort)
            return -1;
        if (first->destPort > second->destPort)
            return 1;

        return 0;
    }
};

/** Holds a fast lookup table for checking which arcs are inputs to others. */
template <class ArcType>
class ArcTable {
public:
    explicit ArcTable (const juce::OwnedArray<ArcType>& arcs)
    {
        for (int i = 0; i < arcs.size(); ++i) {
            const ArcType* const c = arcs.getUnchecked (i);

            int index;
            Entry* entry = findEntry (c->destNode, index);

            if (entry == nullptr) {
                entry = new Entry (c->destNode);
                entries.insert (index, entry);
            }

            entry->srcNodes.add (c->sourceNode);
        }
    }

    bool isAnInputTo (const uint32_t possibleInputId,
                      const uint32_t possibleDestinationId) const noexcept
    {
        return isAnInputToRecursive (possibleInputId, possibleDestinationId, entries.size());
    }

private:
    struct Entry {
        explicit Entry (const uint32_t destNode_) noexcept : destNode (destNode_) {}

        const uint32_t destNode;
        juce::SortedSet<uint32_t> srcNodes;

        JUCE_DECLARE_NON_COPYABLE (Entry)
    };

    juce::OwnedArray<Entry> entries;

    bool isAnInputToRecursive (const uint32_t possibleInputId,
                               const uint32_t possibleDestinationId,
                               int recursionCheck) const noexcept
    {
        int index;

        if (const Entry* const entry = findEntry (possibleDestinationId, index)) {
            const juce::SortedSet<uint32_t>& srcNodes = entry->srcNodes;

            if (srcNodes.contains (possibleInputId))
                return true;

            if (--recursionCheck >= 0) {
                for (int i = 0; i < srcNodes.size(); ++i)
                    if (isAnInputToRecursive (possibleInputId, srcNodes.getUnchecked (i), recursionCheck))
                        return true;
            }
        }

        return false;
    }

    Entry* findEntry (const uint32_t destNode, int& insertIndex) const noexcept
    {
        Entry* result = nullptr;

        int start = 0;
        int end = entries.size();

        for (;;) {
            if (start >= end) {
                break;
            } else if (destNode == entries.getUnchecked (start)->destNode) {
                result = entries.getUnchecked (start);
                break;
            } else {
                const int halfway = (start + end) / 2;

                if (halfway == start) {
                    if (destNode >= entries.getUnchecked (halfway)->destNode)
                        ++start;

                    break;
                } else if (destNode >= entries.getUnchecked (halfway)->destNode)
                    start = halfway;
                else
                    end = halfway;
            }
        }

        insertIndex = start;
        return result;
    }

    JUCE_DECLARE_NON_COPYABLE (ArcTable)
};

} // namespace element
