// Copyright 2014-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/core.hpp>

namespace element {

/** A simple interface for implenting LV2 Features */
class LV2Feature
{
public:
    LV2Feature() {}
    virtual ~LV2Feature() {}

    /** Get the LV2_Feature c-type.
        The returned object should be owned by the subclass.  It is the
        subclass' responsibility to free any associated feature data. */
    virtual const LV2_Feature* getFeature() const = 0;

    /** Get the features URI as a string */
    virtual const juce::String& getURI() const = 0;
};

/** An array of lv2 features */
class LV2FeatureArray final
{
public:
    LV2FeatureArray() : needsBuilt (true) {}
    ~LV2FeatureArray() {}

    inline void clear() noexcept
    {
        array.realloc (1);
        array[0] = nullptr;
        features.clearQuick (true);
        needsBuilt = false;
    }

    /** Add a new feature to the array.  The passed LV2Feature will be owned
        by this class */
    inline void add (LV2Feature* feature, bool rebuildArray = true)
    {
        std::unique_ptr<LV2Feature> f (feature);
        if (f && ! contains (f->getURI()))
        {
            features.add (f.release());
            if (rebuildArray)
                buildArray();
        }
    }

    /** Returns true if a feature has been added to the array */
    inline bool contains (const juce::String& featureURI) const
    {
        for (int i = features.size(); --i >= 0;)
            if (features[i]->getURI() == featureURI)
                return true;
        return false;
    }

    inline LV2Feature* getFeature (const juce::String& uri) const
    {
        for (int i = 0; i < features.size(); ++i)
            if (uri == features.getUnchecked (i)->getURI())
                return features.getUnchecked (i);
        return nullptr;
    }

    template <class FeatureType>
    inline FeatureType* getFeature() const
    {
        for (int i = features.size(); --i >= 0;)
            if (FeatureType* f = dynamic_cast<FeatureType*> (features.getUnchecked (i)))
                return f;
        return nullptr;
    }

    /** Get a C-Style array of feautres */
    inline LV2_Feature* const* getFeatures() const
    {
        jassert (needsBuilt == false);
        return array.getData();
    }

    inline void getFeatures (Array<const LV2_Feature*>& feats, bool nullTerminated = false) const
    {
        for (auto* const f : features)
            if (f != nullptr)
                feats.add (f->getFeature());
        if (nullTerminated)
            feats.add (nullptr);
    }

    inline int size() const { return features.size(); }
    inline LV2_Feature* begin() const { return (LV2_Feature*) array[0]; }
    inline LV2_Feature* end() const { return (LV2_Feature*) array[features.size() - 1]; }
    inline operator LV2_Feature* const*() const { return array.getData(); }

    inline void listFeatures() const
    {
        for (int i = 0; i < features.size() + 1; ++i)
            if (array[i] != nullptr)
                juce::Logger::writeToLog ("feature: " + juce::String (i) + juce::String (" ") + array[i]->URI);
            else
                juce::Logger::writeToLog ("feature: NULL termination");
    }

private:
    juce::OwnedArray<LV2Feature> features;
    juce::HeapBlock<LV2_Feature*> array;
    bool needsBuilt;

    inline void buildArray()
    {
        needsBuilt = false;

        array.calloc (features.size() + 1);
        for (int i = 0; i < features.size(); ++i)
            array[i] = const_cast<LV2_Feature*> (features[i]->getFeature());

        array[features.size()] = nullptr;
    }
};

} // namespace element
