/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

#include <element/juce.hpp>

namespace element {

class Console : public juce::Component
{
public:
    explicit Console (const String& name = String());
    virtual ~Console();

    /** Clears the console and/or history
        @param  buffer     If true, clears the display buffer
        @param  history    If true, clears the command history
    */
    void clear (bool buffer = true, bool history = false);

    /** Add some text to the display buffer
        @param  text       The text to add
        @param  prefix     If true, appends the comand prefix before adding
    */
    void addText (const String& text, bool prefix = false);

    /** Show or hide the text prompt */
    void setPromptVisible (bool visible);

    /** Override this to handle when text is entered on the prompt. The default
        implementation just adds entered text to the display buffer */
    virtual void textEntered (const String& text);

    /** @internal */
    void resized() override;
    /** @internal */
    void paint (juce::Graphics&) override;

private:
    class Content;
    friend class Content;
    std::unique_ptr<Content> content;
    void handleTextEntry (const String& text);
};

} // namespace element
