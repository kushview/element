
#ifndef EL_VIEW_HELPERS_H
#define EL_VIEW_HELPERS_H

#include "element/Juce.h"

namespace Element {
namespace ViewHelpers {

void drawBasicTextRow (const String& text, Graphics& g, int w, int h, bool selected, int padding = 10);
void drawVerticalTextRow (const String& text, Graphics& g, int w, int h, bool selected);
    
}
}



#endif  // EL_VIEW_HELPERS_H
