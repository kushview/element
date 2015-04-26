/*
    %%className%%.cpp - This file is part of Element
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

//[Headers] You can add your own extra header files here...
//[/Headers]

%%includeFilesCPP%%

namespace Element {

//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
%%className%%::%%className%% (%%constructorParams%%)
%%initialisers%%{
    %%constructor%%

    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

%%className%%::~%%className%%()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    %%destructor%%

    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
%%methodDefinitions%%

//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so 
    make changes in here at your peril!

BEGIN_JUCER_METADATA

%%metadata%%
END_JUCER_METADATA
*/
#endif

%%staticMemberDefinitions%%
//[EndFile] You can add extra defines here...
//[/EndFile]

} /* namespace Element */
