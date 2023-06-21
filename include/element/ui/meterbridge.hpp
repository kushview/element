// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/ui/view.hpp>

namespace element {

class Context;

class MeterBridge : public juce::Component {
public:
    enum Visibility : uint32 {
        AudioIns = (1u << 0u),
        AudioOuts = (1u << 1u),
        MidiIns = (1u << 2u),
        MIdiOuts = (1u << 3u)
    };

    MeterBridge() = delete;
    MeterBridge (Context&);
    ~MeterBridge();

    void resized() override;
    void paint (juce::Graphics&) override;
    void setVisibility (uint32);
    bool hasVisibility (uint32) const noexcept;
    uint32 visibility() const noexcept;

    int getMeterSize() const;
    void setMeterSize (int);

private:
    class Impl;
    friend class Impl;
    std::unique_ptr<Impl> impl;
};

class MeterBridgeView : public View {
public:
    MeterBridgeView();
    ~MeterBridgeView();
    void initializeView (Services&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

    MeterBridge& getMeterBridge();

private:
    std::unique_ptr<MeterBridge> bridge;
};

} // namespace element
