#include "Tests.h"

namespace Element {

class PresetScanTest : public UnitTestBase {
public:
    PresetScanTest() : UnitTestBase ("Preset Scanning", "presets", "scan") { }

    virtual void initialise() override
    {
        globals.reset (new Globals());
        globals->getPluginManager().addDefaultFormats();
    }

    virtual void shutdown() override
    {
        globals.reset (nullptr);
    }

    virtual void runTest() override
    {
        beginTest ("scan presets");
        Node node;
        DataPath path;
        NodeArray nodes;
        path.findPresetsFor ("AudioUnit", "AudioUnit:Synths/aumu,samp,appl", nodes);
    }

private:
    std::unique_ptr<Globals> globals;
    AudioPluginFormatManager plugins;
    AudioProcessor* createPluginProcessor()
    {
        PluginDescription desc;
        desc.pluginFormatName = "AudioUnit";
        desc.fileOrIdentifier = "AudioUnit:Synths/aumu,samp,appl";
        String msg;
        return plugins.createPluginInstance (desc, 44100.0, 1024, msg);
    }
};

static PresetScanTest sPresetScanTest;

}
