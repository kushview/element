#include <element/processor.hpp>

#include "clapprovider.hpp"

namespace element {

class CLAPProvider::Impl final {

};

CLAPProvider::CLAPProvider()
{
}

CLAPProvider::~CLAPProvider()
{
}

juce::String CLAPProvider::format() const { return "CLAP"; }

Processor* CLAPProvider::create (const juce::String&)
{
    return nullptr;
}

juce::StringArray CLAPProvider::findTypes()
{
    juce::StringArray res;
    return res;
}

StringArray getHiddenTypes() { return {}; }

} // namespace element