/*
    This file is part of Element
    Copyright (C) 2023  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

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
    void initializeView (ServiceManager&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

    MeterBridge& getMeterBridge();

private:
    std::unique_ptr<MeterBridge> bridge;
};

} // namespace element
