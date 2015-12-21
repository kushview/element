#ifndef ELEMENT_COMMAND_MANAGER_H
#define ELEMENT_COMMAND_MANAGER_H

#include "element/Juce.h"

namespace Element {

class CommandManager :  public ApplicationCommandManager
{
public:
    CommandManager() { }
    ~CommandManager() { }
};

}

#endif // ELEMENT_COMMAND_MANAGER_H

