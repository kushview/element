/*
    This file is part of Element
    Copyright (C) 2020  Kushview, LLC.  All rights reserved.

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

#include "JuceHeader.h"
#include "sol/forward.hpp"

namespace Element {

using ScriptResult = sol::protected_function_result;

class ScriptObject final : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<ScriptObject>;

    ScriptObject();
    ~ScriptObject();

    bool load (sol::state_view& view, const String& buffer);
    ScriptResult call();
    ScriptResult loadAndCall (sol::state_view& view, const String& buffer);

    String getName()        const;
    String getType()        const;
    String getAuthor()      const;
    String getDescription() const;
    String getSource()      const;

    bool hasError() const;
    String getErrorMessage() const;

protected:
    virtual void loaded() {}
    virtual bool validateExport (const sol::reference& ref);

private:
    struct Impl; friend struct Impl;
    std::unique_ptr<Impl> impl;
};

}
