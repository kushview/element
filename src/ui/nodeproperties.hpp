// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include "ElementApp.h"
namespace element {

class Node;

/** Implemented by a PropertyComponent that wants to absorb whatever vertical
    space the rest of its panel leaves unused, instead of being laid out at a
    fixed height. Only has an effect inside a FillingPropertyPanel.
*/
struct FillingProperty
{
    virtual ~FillingProperty() = default;

    /** Returns the smallest height this property may be laid out at. */
    virtual int minimumHeight() const = 0;
};

/** A PropertyPanel whose FillingProperty, if it has one, reaches the bottom
    of the view.

    A plain PropertyPanel stacks its components at their preferred heights and
    leaves any remaining space blank, which is wrong for a property that is
    itself a list. This lays out normally, hands the leftover height to the
    filling property, then lays out once more.
*/
class FillingPropertyPanel : public juce::PropertyPanel
{
public:
    void resized() override;
};

struct NodeProperties : public Array<PropertyComponent*>
{
    enum Groups
    {
        General = 1 << 0,
        Midi = 1 << 1,
        Programs = 1 << 2,
        ALL = General | Midi | Programs
    };

    NodeProperties (const Node& n, int groups);
    NodeProperties (const Node& n, bool nodeProps = true, bool midiProps = false, bool programsProps = false);
};

} // namespace element
