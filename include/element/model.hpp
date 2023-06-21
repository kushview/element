// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/element.h>
#include <element/juce/data_structures.hpp>

namespace juce {
class UndoManager;
}

namespace element {

/** A thin wrapper around a juce juce::ValueTree */
class EL_API Model {
public:
    explicit Model (const juce::ValueTree& data = juce::ValueTree());
    Model (const juce::Identifier& slugId);
    virtual ~Model() {}

    /** Get a property from the underlying juce::ValueTree */
    inline juce::var getProperty (const juce::Identifier& id, const juce::var& d = juce::var()) const
    {
        return objectData.getProperty (id, d);
    }

    /** Get a property as a juce::Value from the juce::ValueTree */
    juce::Value getPropertyAsValue (const juce::Identifier& property, bool updateSynchronously = false);

    /** Set a property */
    inline Model& setProperty (const juce::Identifier& property, const juce::var& val)
    {
        objectData.setProperty (property, val, nullptr);
        return *this;
    }

    /** Returns true if the property exists */
    inline bool hasProperty (const juce::Identifier& property) const { return objectData.hasProperty (property); }

    /** Get the juce::ValueTree's type */
    inline juce::Identifier getType() const { return objectData.getType(); }

    /** Determine this objects juce::ValueTree type */
    inline bool hasType (const juce::Identifier& type) const { return objectData.hasType (type); }

    /** Access to the underlying juce::ValueTree (const version) */
    inline const juce::ValueTree& getValueTree() const noexcept { return objectData; }

    /** Access to the underlying juce::ValueTree */
    inline juce::ValueTree getValueTree() noexcept { return objectData; }

    /** Returns a juce::XmlElement representation of this Model. */
    virtual std::unique_ptr<juce::XmlElement> createXml() const { return objectData.createXml(); }

    /** Returns an Xml string of this Model. */
    juce::String toXmlString() const noexcept { return objectData.toXmlString(); }

    /** Replace this objects juce::ValueTree with another
        If you need to do something special when data is set, then override
        the canAcceptData and setNodeData methods

        @param data new data to use
    */
    juce::ValueTree setData (const juce::ValueTree& data);

    /** Returns the number of children the underlying juce::ValueTree has */
    juce::int32 getNumChildren() const { return objectData.getNumChildren(); }

    /** Count the number of children with a type */
    juce::int32 countChildrenOfType (const juce::Identifier& slug) const;

    inline static void removeFromParent (const juce::ValueTree& data, juce::UndoManager* undo = nullptr)
    {
        auto parent = data.getParent();
        if (! parent.isValid())
            return;
        parent.removeChild (data, undo);
    }

    inline static void removeFromParent (const Model& model, juce::UndoManager* undo = nullptr)
    {
        removeFromParent (model.getValueTree(), undo);
    }

protected:
    /** Override this to handle special data validation This is called
        during setData

        @param data The new data to set
    */
    virtual bool canAcceptData (const juce::ValueTree& data);

    /** Override this to handle special data setting. This is called during
        setData and only if canAcceptData returns true

        @param data The data being setData
    */
    virtual void setNodeData (const juce::ValueTree& data);
    juce::ValueTree objectData;

    template <typename POD>
    inline void stabilizePropertyPOD (const juce::Identifier& prop, const POD& defaultValue = juce::var())
    {
        if (objectData.isValid())
            objectData.setProperty (prop, (POD) objectData.getProperty (prop, defaultValue), nullptr);
    }

    inline void stabilizePropertyString (const juce::Identifier& prop, const juce::String& defaultValue = juce::String())
    {
        if (objectData.isValid())
            objectData.setProperty (prop, objectData.getProperty (prop, defaultValue).toString(), nullptr);
    }

    inline void stabilizeProperty (const juce::Identifier& prop, const juce::var& defaultValue)
    {
        if (objectData.isValid())
            objectData.setProperty (prop, objectData.getProperty (prop, defaultValue), nullptr);
    }
};

/** Alias for backward compatibility. */
using ObjectModel = Model;

} // namespace element
