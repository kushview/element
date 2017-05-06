/*
  ==============================================================================

    RackView.h
    Created: 4 Apr 2016 6:48:21pm
    Author:  Michael Fisher

  ==============================================================================
*/

#ifndef RACKVIEW_H_INCLUDED
#define RACKVIEW_H_INCLUDED

#include "ElementApp.h"

namespace Element {

class RackView :  public Component
{
public:
    RackView();
    ~RackView();

    void paint (Graphics&);
    void resized();

    void setMainComponent (Component* comp);
private:
    class Impl; friend class Impl;
    ScopedPointer<Impl> impl;
    ScopedPointer<Component> main;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RackView)
};

}

#endif  // RACKVIEW_H_INCLUDED
