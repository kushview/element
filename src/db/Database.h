#pragma once

#include "JuceHeader.h"

namespace Element {

class Database
{
public:
    Database();
    virtual ~Database();

    void exec (const String& sql);
};

}