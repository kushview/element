
#include "engine/nodes/LuaNode.h"
#include "LuaUnitTest.h"

using namespace Element;

static const String nodeScript = R"(
function node_io_ports()
    return {
        audio_ins   = 2,
        audio_outs  = 2,
        midi_ins    = 1,
        midi_outs   = 0
    }
end

-- Return parameters
function node_params()
    return {
        {
            name    = "Volume",
            label   = "dB",
            type    = "float",
            flow    = "input",
            min     = -90.0,
            max     = 24.0,
            default = 0.0
        }
    }
end

prepared_flag = false

function node_prepare (sample_rate, block_size)
    print ("prepare test node")
    prepared_flag = true
end

function node_render (a, m)
end

function node_release()
    prepared_flag = false
end
)";

static const String globalSyntaxError = R"(

bad global syntax in script

function node_io_ports()
    return {
        audio_ins   = 2,
        audio_outs  = 2,
        midi_ins    = 1,
        midi_outs   = 0
    }
end

-- Return parameters
function node_params()
    return {
        {
            name    = "Volume",
            label   = "dB",
            type    = "float",
            flow    = "input",
            min     = -90.0,
            max     = 24.0,
            default = 0.0
        }
    }
end

prepared_flag = false

function node_prepare (sample_rate, block_size)
    print ("prepare test node")
    prepared_flag = true
end

function node_render (details)
    print (details)
end

function node_release()
    prepared_flag = false
end
)";

static const String funcSyntaxError = R"(
function node_io_ports()
    return {
        audio_ins   = 2,
        audio_outs  = 2,
        midi_ins    = 1,
        midi_outs   = 0
    }
    return --- can't do this in Lua
end

-- Return parameters
function node_params()
    return {
        {
            name    = "Volume",
            label   = "dB",
            type    = "float",
            flow    = "input",
            min     = -90.0,
            max     = 24.0,
            default = 0.0
        }
    }
end

prepared_flag = false

function node_prepare (sample_rate, block_size)
    print ("prepare test node")
    prepared_flag = true
end

function node_render (details)
    print(details)
end

function node_release()
    prepared_flag = false
end
)";

static const String accessNilObject = R"(

function node_io_ports()
    return {
        audio_ins   = 2,
        audio_outs  = 2,
        midi_ins    = 0,
        midi_outs   = 0
    }
end

-- Return parameters
function node_params()
    return {}
end

prepared_flag = false

function node_prepare (sample_rate, block_size)
    prepared_flag = true
end

function node_render (details)
    print(details)
end

function node_release()
end
)";

//=============================================================================
class LuaNodeLifecycleTest : public LuaUnitTest
{
public:
    LuaNodeLifecycleTest() : LuaUnitTest ("Lua Node Lifecycle", "LuaNode", "lifecycle") {}
    ~LuaNodeLifecycleTest() override = default;

    void runTest() override
    {
        auto graph = std::make_unique<GraphProcessor>();
        graph->prepareToPlay (44100.0, 1024);
        auto* node = new LuaNode();
        
        beginTest ("validate");
        auto result = node->loadScript (nodeScript);
        expect (result.wasOk());
        if (! result.wasOk())
            return;

        beginTest ("prepare");
        graph->addNode (node);

        beginTest ("ports");
        expect (node->getNumPorts() == 6);
        expect (node->getNumPorts (kv::PortType::Audio,   true)  == 2);
        expect (node->getNumPorts (kv::PortType::Audio,   false) == 2);
        expect (node->getNumPorts (kv::PortType::Midi,    true)  == 1);
        expect (node->getNumPorts (kv::PortType::Midi,    false) == 0);
        expect (node->getNumPorts (kv::PortType::Control, true)  == 1);
        
        beginTest ("release");
        graph->releaseResources();

        node = nullptr;
        graph = nullptr;
    }

private:
    sol::state lua;
};

static LuaNodeLifecycleTest sLuaNodeLifecycleTest;

//=============================================================================
class LuaNodeValidateTest : public UnitTestBase
{
public:
    LuaNodeValidateTest() : UnitTestBase ("Lua Node Validation", "LuaNode", "validate") { }
    virtual ~LuaNodeValidateTest() {}

    void runTest() override
    {
        auto node = std::make_unique<LuaNode>();

        beginTest ("validation ok");
        auto result = node->loadScript (nodeScript);
        expect (result.wasOk());
        return;

        beginTest ("global syntax error");
        result = node->loadScript (globalSyntaxError);
        expect (result.failed());

        beginTest ("runtime syntax error");
        node = std::make_unique<LuaNode>();
        result = node->loadScript (funcSyntaxError);
        expect (result.failed());

        beginTest ("access nil midi buffer");
        node = std::make_unique<LuaNode>();
        result = node->loadScript (accessNilObject);
        expect (result.failed());

        beginTest ("no code");
        node = std::make_unique<LuaNode>();
        result = node->loadScript ({});
        expect (result.failed());
    }

private:
    sol::state lua;
};

static LuaNodeValidateTest sLuaNodeValidateTest;
