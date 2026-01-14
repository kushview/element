// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

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
    String format() const override { return EL_NODE_FORMAT_NAME; }
    /** Create the instance by ID string. */
    Processor* create (const String&) override;
    /** return a list of types contained in this provider. */
    StringArray findTypes (const juce::FileSearchPath&, bool, bool) override;

private:
    class Format;
    std::unique_ptr<Format> _format;
};

} // namespace element
