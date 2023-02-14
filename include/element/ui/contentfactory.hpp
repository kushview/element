#pragma once

#include <memory>

#include <element/juce/core.hpp>
#include <element/element.hpp>

namespace element {

class ContentComponent;

class ContentFactory {
public:
    virtual ~ContentFactory() = default;

    /** Create a main content by type name.
        
        Return the content specified.  If type is empty or not supported,
        you should still return a valid Content object.
    */
    virtual std::unique_ptr<ContentComponent> createMainContent (const juce::String& type) =0;

protected:
    ContentFactory() = default;

private:
    EL_DISABLE_COPY(ContentFactory)
};

}
