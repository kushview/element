// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include "ElementApp.h"
namespace element {

class Node;

struct NodeProperties : public Array<PropertyComponent*>
{
    enum Groups
    {
        General = 1 << 0,
        Midi = 1 << 1,
        ALL = General | Midi
    };

    NodeProperties (const Node& n, int groups);
    NodeProperties (const Node& n, bool nodeProps = true, bool midiProps = false);
};

} // namespace element
