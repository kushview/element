/*
    This file is part of the Kushview Modules for JUCE
    Copyright (c) 2014-2019  Kushview, LLC.  All rights reserved.

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

#include <element/model.hpp>

namespace element {

using namespace juce;
Model::Model (const ValueTree& data) : objectData (data) {}
Model::Model (const Identifier& type) : objectData (type) {}

bool Model::canAcceptData (const ValueTree& data)
{
    return objectData.hasType (data.getType());
}

int32 Model::countChildrenOfType (const Identifier& slug) const
{
    int32 cnt = 0;

    for (int i = getValueTree().getNumChildren(); --i >= 0;)
        if (getValueTree().getChild (i).hasType (slug))
            ++cnt;

    return cnt;
}

Value Model::getPropertyAsValue (const Identifier& property, bool updateSynchronously)
{
    return objectData.getPropertyAsValue (property, nullptr, updateSynchronously);
}

ValueTree Model::setData (const ValueTree& data)
{
    if (! canAcceptData (data))
        return data;

    setNodeData (data);
    return objectData;
}

void Model::setNodeData (const ValueTree& data)
{
    objectData = data;
}

} // namespace element
