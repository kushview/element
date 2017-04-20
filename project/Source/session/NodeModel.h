/*
    NodeModel.h - This file is part of Element
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

#ifndef ELEMENT_NODE_MODEL_H
#define ELEMENT_NODE_MODEL_H

#include "ElementApp.h"

namespace Element {

class Node : public ObjectModel
{
public:
    Node (const ValueTree& data, const bool setMissing = true)
        : ObjectModel (data)
    {
        jassert (data.hasType (Tags::node));
        if (setMissing)
            setMissingProperties();
    }
    
    Node (const Identifier& nodeType)
        : ObjectModel (Tags::node)
    {
        setMissingProperties();
    }
    
private:
    inline void setMissingProperties()
    {
        stabilizePropertyString (Slugs::type, "default");
    }
};

typedef Node NodeModel;

}

#endif // ELEMENT_NODE_MODEL_H
