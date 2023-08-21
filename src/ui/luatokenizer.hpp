// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce.hpp>
using namespace juce; // FIXME

namespace element {

//==============================================================================
/**

    @tags{GUI}
*/
class JUCE_API LuaTokeniser : public CodeTokeniser
{
public:
    //==============================================================================
    LuaTokeniser();
    ~LuaTokeniser() override;

    //==============================================================================
    int readNextToken (CodeDocument::Iterator&) override;
    CodeEditorComponent::ColourScheme getDefaultColourScheme() override;

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
