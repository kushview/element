// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

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
        return 0;
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

void Model::removeFromParent (const juce::ValueTree& data, juce::UndoManager* undo)
{
    auto parent = data.getParent();
    if (! parent.isValid())
        return;
    parent.removeChild (data, undo);
}

void Model::removeFromParent (const Model& model, juce::UndoManager* undo)
{
    removeFromParent (model.data(), undo);
}

ValueTree Model::copyWithType (const ValueTree& tree, const Identifier& newType) noexcept
{
    juce::ValueTree ntree (newType);
    ntree.copyPropertiesAndChildrenFrom (tree, nullptr);
    return ntree;
}

void Model::copyChildrenWithType (const ValueTree& src, ValueTree& dst, const Identifier& newType) noexcept
{
    for (const auto& op : src)
    {
        auto np = Model::copyWithType (op, newType);
        if (np.isValid())
        {
            dst.addChild (np, -1, 0);
        }
    }
}

} // namespace element
