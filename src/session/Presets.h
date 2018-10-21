
#pragma once

#include "ElementApp.h"
#include "session/Node.h"

namespace Element {

struct PresetDescription
{
    String name;
    String identifier;
    String format;
    File file;
};

class PresetCollection
{
public:
    PresetCollection() { }
    ~PresetCollection() { }

    inline void clear()
    {
        presets.clear();
    }

    inline void getPresetsFor (const Node& node, OwnedArray<PresetDescription>& results) const
    {
        for (const auto* const preset : presets)
            if (preset->identifier == node.getIdentifier().toString() && preset->format == node.getFormat().toString())
                results.add (new PresetDescription (*preset));
    }

    inline void addPresetFor (const Node& node, const String& name)
    {
        jassertfalse;
    }

    inline void refresh()
    {
        clear();
        
        StringArray files; path.findPresetFiles (files);
        for (const auto& filename : files)
        {
            const File file (filename);
            const Node node (Node::parse (file), false);
            if (node.isValid())
            {
                std::unique_ptr<PresetDescription> item;
                item.reset (new PresetDescription());
                item->file          = file;
                item->name          = node.getName();
                if (item->name.isEmpty())
                    item->name = file.getFileNameWithoutExtension();
                item->format        = node.getFormat();
                item->identifier    = node.getIdentifier();
                if (item->format.isEmpty() || item->identifier.isEmpty())
                    continue;

                presets.add (item.release());
            }
        }

        presets.minimiseStorageOverheads();
    }

private:
    DataPath path;
    OwnedArray<PresetDescription> presets;
};

}
