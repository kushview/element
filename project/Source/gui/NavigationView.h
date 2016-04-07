#ifndef EL_NAVIGATION_VIEW_H
#define EL_NAVIGATION_VIEW_H

#include "element/Juce.h"

namespace Element {

class NavigationList;
class NavigationTree;

class NavigationView    : public Component
{
public:
    NavigationView();
    ~NavigationView();

    void paint (Graphics&);
    void resized();

private:
    ScopedPointer<NavigationList> navList;
    ScopedPointer<NavigationTree> navTree;
    ScopedPointer<StretchableLayoutResizerBar> navBar;
    StretchableLayoutManager layout;

    void updateLayout();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NavigationView)
};

}

#endif  // EL_NAVIGATION_VIEW_H
