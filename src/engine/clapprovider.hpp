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
    juce::FileSearchPath defaultSearchPath();
    juce::StringArray findTypes (const FileSearchPath& path,
                                 bool recursive,
                                 bool allowAsync) override;
    StringArray getHiddenTypes() { return {}; }

private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};
} // namespace element
