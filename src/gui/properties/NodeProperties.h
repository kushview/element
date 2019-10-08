#pragma once
#include "ElementApp.h"
namespace Element {

class Node;

struct NodeProperties : public Array<PropertyComponent*>
{
    NodeProperties (const Node& n, bool nodeProps = true, bool midiProps = false);
};

}
