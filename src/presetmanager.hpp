// Copyright 2014-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/node.hpp>
#include <element/presets.hpp>

#include "datapath.hpp"

namespace element {

class PresetManager
{
public:
    struct SortByName
    {
        int compareElements (PresetInfo* first, PresetInfo* second) const
        {
            if (first->name < second->name)
                return -1;
            if (first->name == second->name)
                return 0;
            if (first->name > second->name)
                return 1;
            return 0;
        }
    };

    PresetManager() {}
    ~PresetManager() {}

    inline void clear()
    {
        presets.clear();
    }

    inline void getPresetsFor (const Node& node, OwnedArray<PresetInfo>& results) const
    {
        SortByName sorter;
        for (const auto* const preset : presets)
            if (preset->ID == node.getIdentifier().toString() && preset->format == node.getFormat().toString())
                results.addSorted (sorter, new PresetInfo (*preset));
    }

    inline void addPresetFor (const Node& node, const String& name)
    {
        jassertfalse;
    }

    inline void refresh()
    {
        clear();

        StringArray files;
        path.findPresetFiles (files);

        for (const auto& filename : files)
        {
            const File file (filename);
            const Node node (Node::parse (file), false);
            if (node.isValid())
            {
                std::unique_ptr<PresetInfo> item;
                item.reset (new PresetInfo());
                item->file = file.getFullPathName().toStdString();
                item->name = node.getName().toStdString();
                if (item->name.empty())
                    item->name = file.getFileNameWithoutExtension().toStdString();
                item->format = node.getFormat().toString().toStdString();
                item->ID = node.getIdentifier().toString().toStdString();
                if (item->format.empty() || item->ID.empty())
                    continue;

                presets.add (item.release());
            }
        }

        presets.minimiseStorageOverheads();
    }

private:
    DataPath path;
    OwnedArray<PresetInfo> presets;
};

} // namespace element
