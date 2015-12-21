
#include "element/Juce.h"

namespace Element {
  class Controller {
  public:
    Controller() : parent (nullptr) { }
    virtual ~Controller()
    {
        parent = nullptr;
    }

    inline Controller* getParent() const { return parent; }

    template<class T> inline T* findParent() {
        auto* ctl = this;
        while (nullptr != ctl)
        {
            if (auto* parent = dynamic_cast<T*> (ctl->parent))
                return parent;
            ctl = ctl->parent;
        }
        return nullptr;
    }

    template<class T> inline T* findChild()
    {
        for (auto const* c : children)
            if (T const* t = dynamic_cast<T*> (c))
                return t;
        return nullptr;
    }

    inline void addChild (Controller* c)
    {
        if (auto* added = children.add (c))
            added->parent = this;
    }

  private:
    OwnedArray<Controller> children;
    Controller* parent;
  };
}
