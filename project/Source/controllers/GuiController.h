
#ifndef EL_GUI_CONTROLLER_H
#define EL_GUI_CONTROLLER_H

#include "controllers/Controller.h"

namespace Element {
    
    class GuiController : public Controller
    {
    public:
        GuiController() { }
        ~GuiController() { }
        
        void activate() override;
        void deactivate() override;
    };
}

#endif  // EL_GUI_CONTROLLER_H
