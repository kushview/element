#pragma once

#include <element/juce/core.hpp>
#include <element/nodefactory.hpp>

namespace element {

class Processor;

/** A provider of CLAP plugins. */
class CLAPProvider final : public NodeProvider
{
public:
    CLAPProvider();
    ~CLAPProvider();
    juce::String format() const override;
    Processor* create (const juce::String&) override;
    juce::FileSearchPath defaultSearchPath() override;
    juce::StringArray findTypes (const FileSearchPath& path,
                                 bool recursive,
                                 bool allowAsync) override;
    StringArray getHiddenTypes() override { return {}; }
    void scan (const String& fileOrID, OwnedArray<PluginDescription>& out) override;

private:
    class Host;
    std::unique_ptr<Host> _host;
};
} // namespace element
