/*
    Asset.h- This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
*/

#ifndef ELEMENT_ASSET_H
#define ELEMENT_ASSET_H

#include "ElementApp.h"
#include "session/AssetTree.h"

namespace Element {


class AssetNode :  public ObjectModel
{
public:
    AssetNode (const AssetItem& item);
    ~AssetNode();
};

}

#endif /* ELEMENT_ASSET_H */
