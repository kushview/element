
#pragma once

namespace Element {

class ScriptInstance
{
public:
    ScriptInstance() = default;
    virtual ~ScriptInstance() = default;
    
    void cleanup()
    {
        switch (object.get_type()) {
            case sol::type::table:
            {
                auto tbl = object.as<sol::table>();
                if (sol::function f = tbl ["cleanup"])
                    f (object);
                break;
            }
            default:
                break;
        }
    }

private:
    sol::object object;
};

}
