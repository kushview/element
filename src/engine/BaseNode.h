#pragma once

namespace Element {

class BaseNode
{
public:
    virtual ~BaseNode() {}

    virtual std::string getName() const =0;

protected:
    BaseNode() {}

private:
};

}
