/*
    NewSessionView.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.

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

#ifndef __JUCE_HEADER_C8F4A1A75D11B38A__
#define __JUCE_HEADER_C8F4A1A75D11B38A__

//[Headers]     -- You can add your own extra header files here --
#include "element/Juce.h"
#include "session/Session.h"
//[/Headers]



namespace Element {

/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class NewSessionView  : public Component
{
public:
    //==============================================================================
    NewSessionView (Session& s);
    ~NewSessionView();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    //[/UserMethods]

    void paint (Graphics& g);
    void resized();



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<PropertyPanel> properties;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NewSessionView)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

} /* namespace Element */

#endif   // __JUCE_HEADER_C8F4A1A75D11B38A__
