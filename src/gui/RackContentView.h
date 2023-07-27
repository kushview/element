// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#ifndef RACKVIEW_H_INCLUDED
#define RACKVIEW_H_INCLUDED

#include "ElementApp.h"

namespace element {

class RackView : public Component
{
public:
    RackView();
    ~RackView();

    void paint (Graphics&);
    void resized();

    void setMainComponent (Component* comp);

private:
    class Impl;
    friend class Impl;
    std::unique_ptr<Impl> impl;
    std::unique_ptr<Component> main;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RackView)
};

} // namespace element

#endif // RACKVIEW_H_INCLUDED
