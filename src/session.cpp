// Copyright 2014-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/internalformat.hpp"
#include <element/transport.hpp>
#include <element/node.hpp>
#include <element/session.hpp>

#include <element/context.hpp>
#include "tempo.hpp"

using namespace juce;

namespace element {

static Node findNodeRecursive (const Node& start, const Uuid& uuid)
{
    if (! uuid.isNull() && uuid == start.getUuid())
        return start;

    for (int i = start.getNumNodes(); --i >= 0;)
    {
        const auto node = findNodeRecursive (start.getNode (i), uuid);
        if (node.isValid())
            return node;
    }

    return Node();
}

class Session::Impl
{
public:
    Impl (Session& s)
        : owner (s)
    {
    }

    ~Impl() {}

private:
    friend class Session;
    [[maybe_unused]] Session& owner;
};

Session::Session()
    : Model (types::Session, EL_SESSION_VERSION)
{
    impl.reset (new Impl (*this));
    setMissingProperties (true);
    objectData.addListener (this);
}

Session::~Session()
{
    objectData.removeListener (this);
    clear();
    impl.reset();
    jassert (getReferenceCount() == 0);
}

bool Session::addGraph (const Node& node, const bool setActive)
{
    jassert (! node.data().getParent().isValid());
    auto graphs = getGraphsValueTree();
    graphs.addChild (node.data(), -1, nullptr);
    if (setActive)
        graphs.setProperty (tags::active, graphs.indexOf (node.data()), nullptr);
    return true;
}

Node Session::getActiveGraph() const
{
    const int index = getActiveGraphIndex();
    if (isPositiveAndBelow (index, getNumGraphs()))
        return getGraph (index);

    ScopedFrozenLock sfl (*this);
    ValueTree graphs = getGraphsValueTree();
    graphs.setProperty (tags::active, graphs.getNumChildren() > 0 ? 0 : -1, nullptr);
    return graphs.getNumChildren() > 0 ? getGraph (0) : Node();
}

int Session::getActiveGraphIndex() const
{
    return getGraphsValueTree().getProperty (tags::active, -1);
}

void Session::clear()
{
    setMissingProperties (true);
}

bool Session::loadData (const ValueTree& data)
{
    if (! data.hasType (types::Session))
        return false;
    objectData.removeListener (this);
    objectData = data;
    setMissingProperties();
    migrateControllerMaps (objectData);
    cleanOrphanMidiMappings();
    objectData.addListener (this);
    return true;
}

std::unique_ptr<XmlElement> Session::createXml() const
{
    ValueTree saveData = objectData.createCopy();
    Node::sanitizeProperties (saveData, true);
    return saveData.createXml();
}

void Session::setMissingProperties (bool resetExisting)
{
    if (resetExisting)
    {
        objectData.removeAllProperties (nullptr);
    }

    if (! objectData.hasProperty (tags::version))
        setProperty (tags::version, EL_SESSION_VERSION);
    if (! objectData.hasProperty (tags::name))
        setProperty (tags::name, "");
    if (! objectData.hasProperty (tags::tempo))
        setProperty (tags::tempo, (double) 120.0);
    if (! objectData.hasProperty (tags::notes))
        setProperty (tags::notes, String());
    if (! objectData.hasProperty (tags::beatsPerBar))
        setProperty (tags::beatsPerBar, 4);
    if (! objectData.hasProperty (tags::beatDivisor))
        setProperty (tags::beatDivisor, (int) BeatType::QuarterNote);

    if (resetExisting)
        objectData.removeAllChildren (nullptr);

    objectData.getOrCreateChildWithName (tags::graphs, nullptr);
    objectData.getOrCreateChildWithName (tags::controllers, nullptr);
    objectData.getOrCreateChildWithName (tags::maps, nullptr);
    objectData.getOrCreateChildWithName (tags::midiMappings, nullptr);
}

void Session::addMidiMapping (const MidiMapping& mapping)
{
    auto mappings = getMidiMappingsValueTree();
    if (mapping.isValid() && mappings.indexOf (mapping.data()) < 0)
        mappings.addChild (mapping.data(), -1, nullptr);
}

void Session::removeMidiMapping (const MidiMapping& mapping)
{
    auto mappings = getMidiMappingsValueTree();
    if (mapping.data().isAChildOf (mappings))
        mappings.removeChild (mapping.data(), nullptr);
}

MidiMapping Session::findMidiMappingById (const Uuid& uuid) const
{
    const auto idStr = uuid.toString();
    for (int i = getNumMidiMappings(); --i >= 0;)
    {
        auto m = getMidiMapping (i);
        if (m.getUuidString() == idStr)
            return m;
    }
    return MidiMapping (ValueTree());
}

void Session::cleanOrphanMidiMappings()
{
    auto mappings = getMidiMappingsValueTree();
    for (int i = mappings.getNumChildren(); --i >= 0;)
    {
        MidiMapping m (mappings.getChild (i));
        if (m.getTargetType() == "parameter" && ! findNodeById (m.getNodeUuid()).isValid())
            mappings.removeChild (i, nullptr);
    }
}

Node Session::findNodeById (const Uuid& uuid)
{
    Node node;

    for (int i = getNumGraphs(); --i >= 0;)
    {
        node = findNodeRecursive (getGraph (i), uuid);
        if (node.isValid())
            break;
    }

    return node;
}

void Session::notifyChanged()
{
    if (freezeChangeNotification)
        return;
    sendChangeMessage();
}

void Session::valueTreePropertyChanged (ValueTree& tree, const Identifier& property)
{
    if (property == tags::object || (tree.hasType (types::Node) && (property == tags::state || property == tags::updater)))
    {
        return;
    }

    if (tree == objectData && property == tags::tempo)
    {
    }

    notifyChanged();
}

void Session::valueTreeChildAdded (ValueTree& parent, ValueTree& child)
{
    ignoreUnused (parent, child);
    notifyChanged();
}

void Session::valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int)
{
    ignoreUnused (parent, child);
    notifyChanged();
}

void Session::valueTreeChildOrderChanged (ValueTree& parent, int, int) {}
void Session::valueTreeParentChanged (ValueTree& tree) {}
void Session::valueTreeRedirected (ValueTree& tree) {}

void Session::saveGraphState()
{
    for (int i = 0; i < getNumGraphs(); ++i)
        getGraph (i).savePluginState();
}

void Session::restoreGraphState()
{
    for (int i = 0; i < getNumGraphs(); ++i)
        getGraph (i).restorePluginState();
}

bool Session::containsGraph (const Node& graph) const
{
    for (int i = 0; i < getNumGraphs(); ++i)
        if (getGraph (i) == graph)
            return true;
    return false;
}

void Session::forEach (ValueTreeFunction handler) const
{
    forEach (objectData, handler);
}

void Session::forEach (const ValueTree tree, ValueTreeFunction handler) const
{
    handler (tree);
    for (int i = 0; i < tree.getNumChildren(); ++i)
        forEach (tree.getChild (i), handler);
}

void migrateControllerMaps (ValueTree session)
{
    if (! session.isValid())
        return;

    auto controllers = session.getChildWithName (tags::controllers);
    auto maps = session.getChildWithName (tags::maps);
    auto mappings = session.getOrCreateChildWithName (tags::midiMappings, nullptr);

    for (int i = 0; i < maps.getNumChildren(); ++i)
    {
        const auto map = maps.getChild (i);
        if (! map.hasType (types::ControllerMap))
            continue;

        const auto controllerId = map.getProperty (tags::controller).toString();
        const auto controlId = map.getProperty (tags::control).toString();

        auto controller = controllers.getChildWithProperty (tags::uuid, controllerId);
        if (! controller.isValid())
            continue;
        auto control = controller.getChildWithProperty (tags::uuid, controlId);
        if (! control.isValid())
            continue;

        MidiMapping mapping (String {});
        mapping.setProperty (tags::device, controller.getProperty (tags::inputDevice).toString());
        mapping.setProperty (tags::name, control.getProperty (tags::name).toString());
        mapping.setProperty (tags::eventType, control.getProperty ("eventType").toString());
        mapping.setProperty (tags::eventId, (int) control.getProperty ("eventId", 0));
        mapping.setProperty (tags::midiChannel, (int) control.getProperty (tags::midiChannel, 0));
        mapping.setProperty (tags::toggle, ! (bool) control.getProperty ("momentary", false));
        mapping.setProperty (tags::targetType, "parameter");
        mapping.setProperty (tags::node, map.getProperty (tags::node).toString());
        mapping.setProperty (tags::parameter, (int) map.getProperty (tags::parameter, -1));

        mappings.addChild (mapping.data(), -1, nullptr);
    }
}

void Session::setActiveGraph (int index)
{
    if (! isPositiveAndBelow (index, getNumGraphs()))
        return;
    objectData.getChildWithName (tags::graphs)
        .setProperty (tags::active, index, nullptr);
}

bool Session::writeToFile (const File& file) const
{
    ValueTree saveData = objectData.createCopy();
    Node::sanitizeProperties (saveData, true);
    TemporaryFile tempFile (file);

    if (auto fos = tempFile.getFile().createOutputStream())
    {
        {
            GZIPCompressorOutputStream gzip (*fos, 9);
            saveData.writeToStream (gzip);
        }
        fos.reset();
        return tempFile.overwriteTargetFileWithTemporary();
    }

    return false;
}

ValueTree Session::readFromFile (const File& file)
{
    ValueTree data;
    FileInputStream fi (file);

    {
        GZIPDecompressorInputStream gzip (fi);
        data = ValueTree::readFromStream (gzip);
    }

    return data;
}

ValueTree Session::migrate (const ValueTree& oldData, String& error)
{
    error.clear();
    const Model model (oldData);
    if (model.version() == EL_SESSION_VERSION)
        return oldData.createCopy();
    if (model.version() > EL_SESSION_VERSION)
    {
        error = "Migrating from newer to older Session not supported.";
        return {};
    }

    std::clog << "[element] session migration from v" << model.version() << std::endl;

    ValueTree newData;
    if (model.version() == 0)
    {
        newData = Model::copyWithType (oldData, types::Session);

        {
            auto oldGraphs = oldData.getChildWithName (tags::graphs);
            auto newGraphs = newData.getOrCreateChildWithName (tags::graphs, 0);
            newGraphs.removeAllChildren (0);
            for (const auto& g : oldGraphs)
            {
                String error;
                auto mg = Node::migrate (g, error);
                if (error.isEmpty() && mg.isValid())
                {
                    newGraphs.addChild (mg, -1, 0);
                }
                else
                {
                    return {};
                }
            }
        }
        {
            auto oldCtl = oldData.getChildWithName (tags::controllers);
            auto newCtl = newData.getOrCreateChildWithName (tags::controllers, 0);
            newCtl.removeAllChildren (0);
            Model::copyChildrenWithType (oldCtl, newCtl, types::Controller);
            for (const auto& ctl : oldCtl)
            {
                if (! ctl.hasType (tags::control))
                    continue; // warn? error?
                auto nctl = Model::copyWithType (ctl, types::Control);
                if (nctl.isValid())
                {
                    newCtl.addChild (nctl, -1, 0);
                }
            }
        }
        {
            auto oldMaps = oldData.getChildWithName (tags::maps);
            auto newMaps = newData.getOrCreateChildWithName (tags::maps, 0);
            newMaps.removeAllChildren (0);
            Model::copyChildrenWithType (oldMaps, newMaps, types::ControllerMap);
        }
    }
    else
    {
        newData = {};
        error = "Unknown session version to migrate: v";
        error << model.version();
        std::clog << "ERROR: " << error.toStdString() << std::endl;
    }

    return newData;
}

} // namespace element
