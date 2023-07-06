#pragma once

#include <element/nodefactory.hpp>

namespace element {

class Context;

// implemented in nodefactory.cpp
class AudioProcessorFactory : public NodeProvider
{
public:
    AudioProcessorFactory (Context&);
    ~AudioProcessorFactory();
    /** Create the instance by ID string. */
    Processor* create (const String&) override;
    /** return a list of types contained in this provider. */
    StringArray findTypes() override;

private:
    class Format;
    std::unique_ptr<Format> _format;
};

} // namespace element
