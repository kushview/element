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
#include <element/tags.hpp>

namespace element {

using namespace juce;
Model::Model (const ValueTree& data) : objectData (data) {}
Model::Model (const Identifier& type) : objectData (type) {}
Model::Model (const juce::Identifier& type, int dataVersion)
    : objectData (type)
{
    jassert (dataVersion >= 0);
    setProperty (tags::version, dataVersion);
}

int Model::version() const noexcept
{
    if (! objectData.hasProperty (tags::version))
        return -1;
    const auto vers = (int) objectData.getProperty (tags::version);
    jassert (vers >= 0);
    return vers;
}

int Model::countChildrenOfType (const Identifier& slug) const
{
    int cnt = 0;

    for (int i = data().getNumChildren(); --i >= 0;)
        if (data().getChild (i).hasType (slug))
            ++cnt;

    return cnt;
}

Value Model::getPropertyAsValue (const Identifier& property, bool updateSynchronously)
{
    return objectData.getPropertyAsValue (property, nullptr, updateSynchronously);
}

} // namespace element
