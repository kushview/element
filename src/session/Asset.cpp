/*
    Asset.cpp - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
*/

#include "session/Asset.h"
#include "session/AssetTree.h"

namespace Element {
AssetNode::AssetNode (const AssetItem& item)
    : ObjectModel (Slugs::asset)
{ }

AssetNode::~AssetNode() { }
}
