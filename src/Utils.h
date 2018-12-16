
#pragma once

#include "JuceHeader.h"

namespace Element {

/** Display minutes in decimal as min:sec */
inline static String minutesToString (const double input)
{
    double minutes = 0, seconds = 60.0 * modf (input, &minutes);
    String ms (roundToInt (floor (minutes)));
    String mm (roundToInt (floor (seconds)));
    return ms.paddedLeft('0', 2) + ":" + mm.paddedLeft('0', 2);
}

/** Display seconds in decimal as min:sec */
inline static String secondsToString (const double input)
{
    return minutesToString (input / 60.0);
}

}
