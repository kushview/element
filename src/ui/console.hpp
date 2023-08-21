// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

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
