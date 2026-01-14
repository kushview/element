// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/element.h>
#include <element/juce/gui_extra.hpp>

namespace element {

//==============================================================================
/**

    @tags{GUI}
*/
class EL_API LuaTokeniser : public juce::CodeTokeniser
{
public:
    //==============================================================================
    LuaTokeniser();
    ~LuaTokeniser() override;

    //==============================================================================
    int readNextToken (juce::CodeDocument::Iterator&) override;
    juce::CodeEditorComponent::ColourScheme getDefaultColourScheme() override;

    /** The token values returned by this tokeniser. */
    enum TokenType
    {
        tokenType_error = 0,
        tokenType_comment,
        tokenType_keyword,
        tokenType_operator,
        tokenType_identifier,
        tokenType_integer,
        tokenType_float,
        tokenType_string,
        tokenType_bracket,
        tokenType_punctuation,
        tokenType_preprocessor
    };

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LuaTokeniser)
};

} // namespace element
