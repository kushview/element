/*
    %%className%%.h - This file is part of Element
    Copyright (C) 2017 Kushview, LLC.  All rights reserved.
*/

#ifndef %%headerGuard%%
#define %%headerGuard%%

//[Headers]     -- You can add your own extra header files here --
#include "element/Juce.h"
//[/Headers]

%%includeFilesH%%

namespace Element {

/**
                                                                    //[Comments]
                                                                    //[/Comments]
*/
%%classDeclaration%%
{
public:
    //==============================================================================
    %%className%% (%%constructorParams%%);
    ~%%className%%();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    //[/UserMethods]

    %%publicMemberDeclarations%%

private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    //[/UserVariables]

    //==============================================================================
    %%privateMemberDeclarations%%

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (%%className%%)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

} /* namespace Element */

#endif   // %%headerGuard%%
