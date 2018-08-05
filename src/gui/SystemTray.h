
#include "ElementApp.h"

namespace Element {

class SystemTray : public SystemTrayIconComponent {
public:
    SystemTray() { }
    ~SystemTray() { }

    void mouseDown (const MouseEvent&) override;
    void mouseUp (const MouseEvent&) override;

private:
    int mouseUpAction = -1;
};

}