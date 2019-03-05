
#pragma once

#include "JuceHeader.h"

namespace Element {
namespace Util {

static juce_wchar defaultPasswordChar() noexcept
{
#if JUCE_LINUX
    return 0x2022;
#else
    return 0x25cf;
#endif
}

/** Display minutes in decimal as min:sec */
inline static String minutesToString (const double input)
{
    double minutes = 0, seconds = 60.0 * modf (input, &minutes);
    String ms (roundToInt (floor (minutes)));
    String mm (roundToInt (floor (seconds)));
    return ms.paddedLeft('0', 2) + ":" + mm.paddedLeft ('0', 2);
}

/** Display seconds in decimal as min:sec */
inline static String secondsToString (const double input)
{
    return minutesToString (input / 60.0);
}

/** Returns the application name depending on product.
    Element Lite, Element Solo, or Element
 */
inline static String appName (const String& beforeText = String())
{
    String name;
    if (beforeText.isNotEmpty())
        name << beforeText << " Element";
    else
        name = "Element";
   
   #if defined (EL_FREE)
    name << " LT";
   #elif defined (EL_SOLO)
    name << " SE";
   #endif

    return name;
}

inline static bool isGmailExtended (const String& email)
{
    if (! URL::isProbablyAnEmailAddress (email))
        return false;
    if (! email.toLowerCase().contains ("gmail.com"))
        return false;
    if (! email.toLowerCase().upToFirstOccurrenceOf("gmail.com", false, true).containsAnyOf(".+"))
        return false;
    
    return email.fromFirstOccurrenceOf ("+", true, true)
                .upToFirstOccurrenceOf ("@gmail.com", false, true)
                .length() >= 1  ||
            email.fromFirstOccurrenceOf (".", true, true)
                .upToFirstOccurrenceOf ("@gmail.com", false, true)
                .length() >= 1; 
}

StringArray getFullVesrionPluginIdentifiers();

}
}
