
#include <element/nodefactory.hpp>
#include <lvtk/host/world.hpp>
#include <element/lv2.hpp>

namespace element {

class LV2NodeProvider::LV2 {
public:
    LV2 (LV2NodeProvider& p) : provider (p) {
        world = std::make_unique<lvtk::World>();
        world->load_all();
    }

    void getTypes (StringArray& tps) {
        for (const auto& uri : world->plugin_uris()) {
            DBG(uri);
            tps.add (uri);
        }
    }

private:
    friend class LV2NodeProvider;
    LV2NodeProvider& provider;
    std::unique_ptr<lvtk::World> world;
    std::vector<std::string> allow;
};

LV2NodeProvider::LV2NodeProvider() {
    lv2 = std::make_unique<LV2> (*this);
}

LV2NodeProvider::~LV2NodeProvider() {
    lv2.reset();
}

NodeObject* LV2NodeProvider::create (const String& uri) {
    return nullptr;
}

StringArray LV2NodeProvider::findTypes() {
    StringArray types;
    lv2->getTypes (types);
    return types;
}

}
