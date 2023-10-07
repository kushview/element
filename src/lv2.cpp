// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <lv2/midi/midi.h>

#include <lvtk/lvtk.hpp>
#include <lvtk/symbols.hpp>
#include <lvtk/ext/atom.hpp>
#include <lvtk/host/world.hpp>
#include <lvtk/host/instance.hpp>

#include <element/juce/core.hpp>
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wdeprecated-declarations",
                                     "-Wunused-variable")

#include <element/lv2.hpp>
#include "lv2/logfeature.hpp"
#include "lv2/module.hpp"
#include "lv2/world.hpp"
#include "lv2/workthread.hpp"
#include "lv2/workerfeature.hpp"
#include "lv2/native.hpp"

#include <element/ui/nodeeditor.hpp>
#include <element/nodefactory.hpp>
#include "engine/portbuffer.hpp"
#include <element/juce/gui_basics.hpp>
#include <element/juce/gui_extra.hpp>

#if JUCE_MAC
#include "ui/nsviewwithparent.hpp"
#endif

// Change this to enable logging of various LV2 activities
#ifndef EL_LV2_LOGGING
#define EL_LV2_LOGGING 0
#endif

#if EL_LV2_LOGGING
#define EL_LV2_LOG(a) DBG ("[lv2] " << a);
#else
#define EL_LV2_LOG(a)
#endif

namespace element {

template <typename Other, typename Word>
static auto wordCast (Word word)
{
    static_assert (sizeof (word) == sizeof (Other), "Word sizes must match");
    return juce::readUnaligned<Other> (&word);
}

//==============================================================================
// Node factory....
//==============================================================================

class LV2Parameter : public Parameter
{
public:
    LV2Parameter (uint32 port, int param, LV2Module& _module)
        : module (_module),
          portIdx (port),
          paramIdx (param),
          name (module.getPortName (port))
    {
        module.getPortRange (port, minValue, maxValue, defaultValue);
    }

    virtual ~LV2Parameter() = default;

    static LV2Parameter* create (const PortDescription&, LV2Module& module);

    // String getParameterID() const override { return String ((int) getPort()); }

    constexpr uint32 getPort() const noexcept { return portIdx; }
    int getPortIndex() const noexcept override { return static_cast<int> (portIdx); }
    int getParameterIndex() const noexcept override { return paramIdx; }

    /** Returns the LV2 control port min value */
    constexpr float getPortMin() const noexcept { return minValue; }

    /** Returns the LV2 control port max value */
    constexpr float getPortMax() const noexcept { return maxValue; }

    /** Returns the LV2 control port default value */
    float getPortDefault() const { return defaultValue; }

    /** update the value, but dont write to port */
    void update (float newValue, bool notifyListeners = true)
    {
        newValue = convertTo0to1 (newValue);
        if (newValue == value.get())
            return;

        value.set (newValue);
        if (notifyListeners)
            sendValueChangedMessageToListeners (value.get());
    }

    /** Convert the input LV2 "ranged" value to 0 to 1 */
    virtual float convertTo0to1 (float input) const = 0;

    /** Convert the 0 to 1 input to a LV2 "ranged" value */
    virtual float convertFrom0to1 (float input) const = 0;

    //=========================================================================
    float getValue() const override
    {
        return value.get();
    }

    /** Will write to Port with correct min max ratio conversion */
    void setValue (float newValue) override
    {
        value.set (newValue);
        const auto expanded = convertFrom0to1 (newValue);
        module.write (portIdx, sizeof (float), 0, &expanded);
    }

    float getDefaultValue() const override { return convertTo0to1 (defaultValue); }
    String getName (int maxLen) const override { return name.substring (0, maxLen); }

    // Units e.g. Hz
    String getLabel() const override { return {}; }

    String getText (float normalisedValue, int /*maximumStringLength*/) const override
    {
        return String (convertFrom0to1 (normalisedValue), 2);
    }

#if 0
   /** Parse a string and return the appropriate value for it. */
    virtual float getValueForText (const String& text) const;
    virtual int getNumSteps() const;
    virtual bool isDiscrete() const;
    virtual StringArray getAllValueStrings() const;
    virtual bool isBoolean() const;
    virtual bool isOrientationInverted() const;
    virtual bool isAutomatable() const;
    virtual bool isMetaParameter() const;
    virtual Category getCategory() const;
    virtual String getCurrentValueAsText() const;
#endif

private:
    LV2Module& module;
    const uint32 portIdx;
    const int paramIdx;
    const String name;
    float minValue, maxValue, defaultValue;
    Atomic<float> value;
};

//=============================================================================
class LV2ParameterFloat : public LV2Parameter
{
public:
    LV2ParameterFloat (uint32 port, int param, LV2Module& module)
        : LV2Parameter (port, param, module)
    {
        range.start = getPortMin();
        range.end = getPortMax();
    }

    float convertTo0to1 (float input) const override { return range.convertTo0to1 (input); }
    float convertFrom0to1 (float input) const override { return range.convertFrom0to1 (input); }

    /** Parse a string and return the appropriate value for it. */
    float getValueForText (const String& text) const override
    {
        return convertTo0to1 (text.getFloatValue());
    }

private:
    NormalisableRange<float> range;
};

//=============================================================================
class LV2ParameterChoice : public LV2Parameter
{
public:
    LV2ParameterChoice (uint32 port, int param, LV2Module& module, const ScalePoints& sps)
        : LV2Parameter (port, param, module),
          points (sps)
    {
        ScalePoints::Iterator iter (points);
        while (iter.next())
        {
            labels.add (iter.getLabel());
            values.add (iter.getValue());
        }
    }

    float convertTo0to1 (float input) const override
    {
        auto index = static_cast<float> (values.indexOf (input));
        return index / static_cast<float> (values.size() - 1);
    }

    float convertFrom0to1 (float input) const override
    {
        int index = roundToInt (input * (float) (values.size() - 1));
        return isPositiveAndBelow (index, values.size())
                   ? values.getUnchecked (index)
                   : getPortDefault();
    }

    String getCurrentValueAsText() const override
    {
        int index = roundToInt (getValue() * (float) (values.size() - 1));
        return labels[index];
    }

    /** Parse a string and return the appropriate JUCE parameter value for it. */
    float getValueForText (const String& text) const override
    {
        int index = labels.indexOf (text);
        return isPositiveAndBelow (index, values.size())
                   ? (float) index / (float) (values.size() - 1)
                   : getDefaultValue();
    }

    int getNumSteps() const override { return points.isNotEmpty() ? points.size() : Parameter::getNumSteps(); }
    bool isDiscrete() const override { return points.isNotEmpty() ? true : Parameter::isDiscrete(); }
    StringArray getValueStrings() const override { return labels; }

private:
    const ScalePoints points;
    StringArray labels;
    Array<float> values;
};

//=============================================================================
LV2Parameter* LV2Parameter::create (const PortDescription& info, LV2Module& module)
{
    const auto port = static_cast<uint32> (info.index);
    std::unique_ptr<LV2Parameter> param;
    const auto scalePoints = module.getScalePoints (port);
    const auto enumeration = module.isPortEnumerated (port);

    if (enumeration && scalePoints.isNotEmpty())
    {
        param.reset (new LV2ParameterChoice (port, info.channel, module, scalePoints));
    }
    else
    {
        param.reset (new LV2ParameterFloat (port, info.channel, module));
    }

    if (param != nullptr)
    {
        param->value.set (param->getDefaultValue());
    }

    return param != nullptr ? param.release() : nullptr;
}

//==============================================================================
class LV2Processor : public Processor
{
public:
    LV2Processor (World& world, LV2Module* module_)
        : Processor (0),
          wantsMidiMessages (false),
          initialised (false),
          isPowerOn (false),
          tempBuffer (1, 1),
          module (module_)
    {
        atomSequence = module->map (LV2_ATOM__Sequence);
        midiEvent = module->map (LV2_MIDI__MidiEvent);
        numPorts = module->getNumPorts();
        midiPort = module->getMidiPort();
        notifyPort = module->getNotifyPort();
        wantsMidiMessages = midiPort != EL_INVALID_PORT;
        sendsMidiMessages = notifyPort != EL_INVALID_PORT;

        setPorts (module->ports());
        if (sendsMidiMessages && getNumPorts (PortType::Midi, false) <= 0)
        {
            auto newPorts = portList();
            newPorts.add (PortType::Midi, newPorts.size(), 0, "element_midi_output", "MIDI Out", false);
            setPorts (newPorts);
        }

        const ChannelConfig& channels (module->getChannelConfig());
        totalAudioIn = channels.getNumAtomInputs();
        totalAudioOut = channels.getNumAudioOutputs();

        // if (! module->hasEditor())
        {
            jassert (module->onPortNotify == nullptr);
            namespace ph = std::placeholders;
            module->onPortNotify = std::bind (&LV2Processor::portEvent,
                                              this,
                                              ph::_1,
                                              ph::_2,
                                              ph::_3,
                                              ph::_4);
        }
    }

    ~LV2Processor()
    {
        module->onPortNotify = nullptr;
        module = nullptr;
    }

    LV2Module& getModule() { return *module; }

    ParameterPtr getParameter (const PortDescription& port) override
    {
        return port.type == PortType::Control // && port.input
                   ? LV2Parameter::create (port, getModule())
                   : nullptr;
    }

    void portEvent (uint32 port, uint32 size, uint32 protocol, const void* data)
    {
        if (protocol != 0)
            return;

        // clang-format off
        for (auto param : getParameters()) {
            if (auto lv2param = dynamic_cast<LV2Parameter*> (param))
                if (lv2param->getPort() == port)
                    { lv2param->update (*(float*) data, true); break; }
        }
        // clang-format on
    };

    //=========================================================================
    void getPluginDescription (PluginDescription& desc) const override
    {
        desc.name = module->getName();

        desc.descriptiveName = String();
        if (desc.descriptiveName.isEmpty())
            desc.descriptiveName = desc.name;

        desc.fileOrIdentifier = module->getURI();
        desc.uniqueId = desc.fileOrIdentifier.hashCode();

        //        desc.lastFileModTime = 0;
        desc.pluginFormatName = "LV2";

        desc.category = module->getClassLabel();
        desc.manufacturerName = module->getAuthorName();
        desc.version = "";

        desc.numInputChannels = totalAudioIn;
        desc.numOutputChannels = totalAudioOut;
        desc.isInstrument = midiPort != LV2UI_INVALID_PORT_INDEX;
    }

    void initialize() override
    {
        if (initialised)
            return;

#if JUCE_WINDOWS
        // On Windows it's highly advisable to create your plugins using the message thread,
        // because many plugins need a chance to create HWNDs that will get their
        // messages delivered by the main message thread, and that's not possible from
        // a background thread.
        jassert (MessageManager::getInstance()->isThisTheMessageThread());
#endif

        wantsMidiMessages = midiPort != EL_INVALID_PORT;
        sendsMidiMessages = notifyPort != EL_INVALID_PORT;
        initialised = true;
        setLatencySamples (0);
    }

    double getTailLengthSeconds() const { return 0.0f; }
    void* getPlatformSpecificData() { return module->getHandle(); }
    const String getName() const { return module->getName(); }
    bool silenceInProducesSilenceOut() const { return false; }
    bool acceptsMidi() const { return wantsMidiMessages; }
    bool producesMidi() const { return notifyPort != LV2UI_INVALID_PORT_INDEX; }

    //==============================================================================
    void prepareToRender (double sampleRate, int blockSize) override
    {
#if 0 // FIXME?
        const ChannelConfig& channels (module->getChannelConfig());
        setPlayConfigDetails (channels.getNumAudioInputs(),
                              channels.getNumAudioOutputs(),
                              sampleRate,
                              blockSize);
#endif
        initialize();

        if (initialised)
        {
            module->setSampleRate (sampleRate);
            tempBuffer.setSize (std::max (1, std::max (totalAudioIn, totalAudioOut)), blockSize);
            module->activate();
        }
    }

    void releaseResources() override
    {
        if (initialised)
            module->deactivate();

        tempBuffer.setSize (1, 1);
    }

    bool wantsMidiPipe() const override { return true; }

    void renderBypassed (AudioSampleBuffer& a, MidiPipe& m, AudioSampleBuffer& cv) override
    {
        Processor::renderBypassed (a, m, cv);
    }

    void render (AudioSampleBuffer& audio, MidiPipe& midi, AudioSampleBuffer& cv) override
    {
        const auto numSamples = audio.getNumSamples();

        if (! initialised)
        {
            for (int i = 0; i < totalAudioOut; ++i)
                audio.clear (i, 0, numSamples);
            midi.clear();
            return;
        }

        if (wantsMidiMessages)
        {
            PortBuffer* const buf = module->getPortBuffer (midiPort);
            buf->reset();

            for (auto m : *midi.getReadBuffer (0))
            {
                buf->addEvent (m.samplePosition,
                               static_cast<uint32_t> (m.numBytes),
                               midiEvent,
                               m.data);
            }
        }

        module->referAudioReplacing (audio, cv);
        module->run ((uint32) numSamples);

        midi.clear();
        if (sendsMidiMessages)
        {
            PortBuffer* const buf = module->getPortBuffer (notifyPort);
            jassert (buf != nullptr);

            LV2_ATOM_SEQUENCE_FOREACH ((LV2_Atom_Sequence*) buf->getPortData(), ev)
            {
                if (ev->body.type == midiEvent)
                {
                    midi.getWriteBuffer (0)->addEvent (
                        LV2_ATOM_BODY_CONST (&ev->body),
                        ev->body.size,
                        (int) ev->time.frames);
                }
            }
        }
    }

    bool hasEditor() override { return module->hasEditor(); }
    Editor* createEditor() override;

    //==============================================================================
    int getNumPrograms() const override { return 1; }
    int getCurrentProgram() const override { return 0; }
    void setCurrentProgram (int /*index*/) override {}
    const String getProgramName (int /*index*/) const override { return String ("Default"); }

    //==============================================================================
    void getState (MemoryBlock& mb) override
    {
        const auto state = module->getStateString();
        mb.append (state.toRawUTF8(), state.length());
    }

    void setState (const void* data, int size) override
    {
        MemoryInputStream stream (data, (size_t) size, false);
        module->setStateString (stream.readEntireStreamAsString());
    }

    //==============================================================================
    void timerCallback() {}

    void handleAsyncUpdate()
    {
        // indicates that something about the plugin has changed..
        // updateHostDisplay();
    }

private:
    CriticalSection lock, midiInLock;
    bool wantsMidiMessages, sendsMidiMessages,
        initialised, isPowerOn;
    mutable StringArray programNames;

    AudioSampleBuffer tempBuffer;
    std::unique_ptr<LV2Module> module;
    OwnedArray<PortBuffer> buffers;

    uint32 numPorts { 0 };
    uint32 midiPort { EL_INVALID_PORT };
    uint32 notifyPort { EL_INVALID_PORT };
    uint32 atomSequence { 0 },
        midiEvent { 0 };

    int totalAudioIn { 0 },
        totalAudioOut { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LV2Processor)
};

//=============================================================================
struct PhysicalResizeListener
{
    virtual ~PhysicalResizeListener() = default;
    virtual void viewRequestedResizeInPhysicalPixels (int width, int height) = 0;
};

struct LogicalResizeListener
{
    virtual ~LogicalResizeListener() = default;
    virtual void viewRequestedResizeInLogicalPixels (int width, int height) = 0;
};

struct ViewSizeListener : private ComponentMovementWatcher
{
    ViewSizeListener (Component& c, PhysicalResizeListener& l)
        : ComponentMovementWatcher (&c), listener (l)
    {
    }

    void componentMovedOrResized (bool, bool wasResized) override
    {
        if (wasResized)
        {
            const auto physicalSize = Desktop::getInstance().getDisplays().logicalToPhysical (getComponent()->localAreaToGlobal (getComponent()->getLocalBounds()));
            const auto width = physicalSize.getWidth();
            const auto height = physicalSize.getHeight();

            if (width > 10 && height > 10)
                listener.viewRequestedResizeInPhysicalPixels (width, height);
        }
    }

    void componentPeerChanged() override {}
    void componentVisibilityChanged() override {}

    using ComponentMovementWatcher::componentMovedOrResized;
    using ComponentMovementWatcher::componentVisibilityChanged;

    PhysicalResizeListener& listener;
};

extern bool getNativeWinodwSize (void* window, int& width, int& height);

class LV2NativeEditor : public Editor,
                        public juce::Timer,
                        public PhysicalResizeListener
{
public:
    LV2NativeEditor (LV2Processor* p, LV2ModuleUI::Ptr _ui)
        : Editor(),
          plugin (*p),
          ui (_ui)
    {
        setOpaque (true);
        jassert (ui && ui->isNative());

        view = std::make_unique<ViewComponent> (*this);
        addAndMakeVisible (view.get());

        ui->setParent ((intptr_t) view->getWidget());
        ui->instantiate();
        p->getModule().sendPortEvents();

        nativeViewSetup = false;

        // some plugins will have called UI resize to define its size.
        int w = ui->getClientWidth(), h = ui->getClientHeight();

        if (w <= 0 || h <= 0)
        {
            // if not, try to query the window size.
            w = h = 0;
            getNativeWinodwSize (ui->getWidget(), w, h);
        }

        setSize (w > 0 ? w : 640, h > 0 ? h : 360);
        startTimerHz (60);

        setResizable (ui->haveClientResize());

        ui->onClientResize = [this]() -> int {
            return 0;
        };
    }

    ~LV2NativeEditor()
    {
        view->prepareForDestruction();

        stopTimer();

        if (ui != nullptr)
        {
            ui->unload();
            ui = nullptr;
        }

        view.reset();
    }

    void viewRequestedResizeInPhysicalPixels (int width, int height) override
    {
    }

    void timerCallback() override
    {
        if (! ui || ! ui->isNative())
            return stopTimer();

        if (! nativeViewSetup)
        {
            if (ui->loaded())
            {
                int w = 0, h = 0;
                nativeViewSetup = true;
            }
        }
        if (nativeViewSetup)
        {
            if (ui->haveIdleInterface())
                ui->idle();
            else
                stopTimer();
        }
    }

    void paint (Graphics& g) override
    {
        g.fillAll (Colours::black);
    }

    void resized() override
    {
        if (view != nullptr)
            view->setBounds (getLocalBounds());
    }

private:
    LV2Processor& plugin;
    LV2ModuleUI::Ptr ui = nullptr;
    bool nativeViewSetup = false;

#if JUCE_LINUX || JUCE_BSD
    struct InnerHolder
    {
        struct Inner : public XEmbedComponent
        {
            Inner() : XEmbedComponent (true, true)
            {
                setOpaque (true);
                addToDesktop (0);
            }

            void paint (Graphics& g) override
            {
                g.fillAll (Colours::black);
            }
        };

        Inner inner;
    };

    struct ViewComponent : public InnerHolder,
                           public XEmbedComponent
    {
        explicit ViewComponent (PhysicalResizeListener& l)
            : XEmbedComponent ((unsigned long) inner.getPeer()->getNativeHandle(), true, false),
              listener (inner, l)
        {
            setOpaque (true);
        }

        ~ViewComponent()
        {
            removeClient();
        }

        void prepareForDestruction()
        {
            inner.removeClient();
        }

        LV2UI_Widget getWidget()
        {
            return element::wordCast<LV2UI_Widget> (inner.getHostWindowID());
        }

        void forceViewToSize() {}
        void fitToView() {}

        void paint (Graphics& g) override
        {
            g.fillAll (Colours::black);
        }

        ViewSizeListener listener;
    };
#elif JUCE_MAC

    struct ViewComponent : public NSViewComponentWithParent
    {
        explicit ViewComponent (PhysicalResizeListener&)
            : NSViewComponentWithParent (WantsNudge::no) {}
        LV2UI_Widget getWidget() { return getView(); }
        void forceViewToSize() {}
        void fitToView() { resizeToFitView(); }
        void prepareForDestruction() {}
    };
#elif JUCE_WINDOWS
    struct ViewComponent : public HWNDComponent
    {
        explicit ViewComponent (PhysicalResizeListener&)
        {
            setOpaque (true);
            inner.addToDesktop (0);

            if (auto* peer = inner.getPeer())
                setHWND (peer->getNativeHandle());
        }

        void paint (Graphics& g) override { g.fillAll (Colours::black); }

        int getInnerWidth() { return inner.getWidth(); }
        int getInnerHeight() { return inner.getHeight(); }

        LV2UI_Widget getWidget() { return getHWND(); }

        void forceViewToSize() { updateHWNDBounds(); }
        void fitToView() { resizeToFit(); }

        void prepareForDestruction() {}

    private:
        struct Inner : public Component
        {
            Inner() { setOpaque (true); }
            void paint (Graphics& g) override { g.fillAll (Colours::black); }
        };

        Inner inner;
    };
#else
    struct ViewComponent : public Component
    {
        explicit ViewComponent (PhysicalResizeListener&) {}
        void* getWidget() { return nullptr; }
        void forceViewToSize() {}
        void fitToView() {}
        void prepareForDestruction() {}
    };
#endif

    std::unique_ptr<ViewComponent> view;
};

//==============================================================================
Editor* LV2Processor::createEditor()
{
    jassert (module->hasEditor());
    LV2ModuleUI::Ptr ui = module->hasEditor() ? module->createEditor() : nullptr;
    if (ui == nullptr)
        return nullptr;
    return ui->requiresShowInterface()
               ? nullptr
               : new LV2NativeEditor (this, ui);
}

//==============================================================================
class LV2NodeProvider::LV2
{
public:
    LV2 (LV2NodeProvider& p) : provider (p)
    {
        world = std::make_unique<World>();
    }

    ~LV2()
    {
        world.reset();
    }

    void getTypes (StringArray& tps)
    {
        world->getSupportedPlugins (tps);
    }

    LV2Processor* instantiate (const String& uri)
    {
        LV2Processor* proc = nullptr;

        if (LV2Module* module = world->createModule (uri))
        {
            Result res (module->instantiate (44100.0));
            if (res.wasOk())
            {
                proc = new LV2Processor (*world, module);
            }
            else
            {
                std::clog << "[element] lv2 instantiation error: "
                          << res.getErrorMessage().toStdString()
                          << std::endl;
                delete module;
                module = nullptr;
            }
        }
        else
        {
            EL_LV2_LOG ("Failed creating LV2 plugin instance");
        }

        return proc;
    }

private:
    friend class LV2NodeProvider;
    LV2NodeProvider& provider;
    std::unique_ptr<World> world;
};

LV2NodeProvider::LV2NodeProvider()
{
    lv2 = std::make_unique<LV2> (*this);
}

LV2NodeProvider::~LV2NodeProvider()
{
    lv2.reset();
}

Processor* LV2NodeProvider::create (const String& uri)
{
    return lv2->instantiate (uri);
}

StringArray LV2NodeProvider::findTypes()
{
    StringArray types;
    lv2->getTypes (types);
    return types;
}

#if 0

/** Implements a plugin format manager for LV2 plugins in Juce Apps. */
class LV2PluginFormat final : public AudioPluginFormat
{
public:
    LV2PluginFormat();
    ~LV2PluginFormat();

    String getName() const override { return "LV2"; }
    void findAllTypesForFile (OwnedArray<PluginDescription>& descrips, const String& identifier) override;
    bool fileMightContainThisPluginType (const String& fileOrIdentifier) override;
    String getNameOfPluginFromIdentifier (const String& fileOrIdentifier) override;
    bool pluginNeedsRescanning (const PluginDescription&) override { return false; }
    bool doesPluginStillExist (const PluginDescription&) override;
    bool canScanForPlugins() const override { return true; }
    StringArray searchPathsForPlugins (const FileSearchPath&, bool recursive, bool allowPluginsWhichRequireAsynchronousInstantiation = false) override;
    FileSearchPath getDefaultLocationsToSearch() override;
    bool isTrivialToScan() const override { return true; }

protected:
    void createPluginInstance (const PluginDescription&,
                               double initialSampleRate,
                               int initialBufferSize,
                               PluginCreationCallback) override;

    bool requiresUnblockedMessageThreadDuringCreation (const PluginDescription&) const noexcept override { return false; }

private:
    class Internal;
    std::unique_ptr<Internal> priv;
};

//=============================================================================
class LV2AudioProcessorEditor : public AudioProcessorEditor
{
public:
    LV2AudioProcessorEditor (LV2Processor* p, LV2ModuleUI::Ptr _ui)
        : AudioProcessorEditor (p), plugin (*p), ui (_ui)
    {
        jassert (ui != nullptr);
        ui->instantiate();
        jassert (ui->loaded());
        setOpaque (true);
    }

    virtual ~LV2AudioProcessorEditor() {}

    virtual void paint (Graphics& g) override
    {
        g.fillAll (Colours::black);
    }

protected:
    LV2Processor& plugin;
    LV2ModuleUI::Ptr ui;

    void cleanup()
    {
        plugin.editorBeingDeleted (this);
        ui->unload();
        ui = nullptr;
    }
};

//==============================================================================
class LV2PluginFormat::Internal : private Timer
{
public:
    Internal()
    {
        useExternalData = false;
        init();
        world.setOwned (new World());
        startTimerHz (60);
    }

    Internal (World& w)
    {
        useExternalData = true;
        init();
        world.setNonOwned (&w);
    }

    ~Internal()
    {
        world.clear();
        stopTimer();
    }

    LV2Module* createModule (const String& uri)
    {
        return world->createModule (uri);
    }

    OptionalScopedPointer<World> world;
    lvtk::Symbols symbols;

private:
    bool useExternalData;

    void init() {}

    void timerCallback() override
    {
        stopTimer();
    }
};

//=============================================================================
LV2PluginFormat::LV2PluginFormat()
{
    priv.reset (new Internal());
}

LV2PluginFormat::~LV2PluginFormat()
{
    priv.reset();
}

//=============================================================================
void LV2PluginFormat::findAllTypesForFile (OwnedArray<PluginDescription>& results,
                                           const String& fileOrIdentifier)
{
    if (! fileMightContainThisPluginType (fileOrIdentifier))
    {
        return;
    }

    std::unique_ptr<PluginDescription> desc (new PluginDescription());
    desc->fileOrIdentifier = fileOrIdentifier;
    desc->pluginFormatName = String ("LV2");
    desc->uniqueId = 0;

    try
    {
        auto instance (createInstanceFromDescription (*desc.get(), 44100.0, 1024));
        if (LV2Processor* const p = dynamic_cast<LV2Processor*> (instance.get()))
        {
            p->fillInPluginDescription (*desc.get());
            results.add (desc.release());
        }
    } catch (...)
    {
        EL_LV2_LOG ("crashed: " + String (desc->name));
    }
}

bool LV2PluginFormat::fileMightContainThisPluginType (const String& fileOrIdentifier)
{
    bool maybe = fileOrIdentifier.contains ("http:") || fileOrIdentifier.contains ("https:") || fileOrIdentifier.contains ("urn:");

    if (! maybe && File::isAbsolutePath (fileOrIdentifier))
    {
        const File file (fileOrIdentifier);
        maybe = file.getChildFile ("manifest.ttl").existsAsFile();
    }

    return maybe;
}

String LV2PluginFormat::getNameOfPluginFromIdentifier (const String& fileOrIdentifier)
{
    const auto name = priv->world->getPluginName (fileOrIdentifier);
    return name.isEmpty() ? fileOrIdentifier : name;
}

StringArray LV2PluginFormat::searchPathsForPlugins (const FileSearchPath& paths, bool, bool)
{
    if (paths.getNumPaths() > 0)
    {
#if JUCE_WINDOWS
        // putenv ("LV2_PATH", paths.toString().toRawUTF8(), 0);
        // setenv ("LV2_PATH", paths.toString().toRawUTF8(), 0);
#else
        setenv ("LV2_PATH", paths.toString().replace (";", ":").toRawUTF8(), 0);
#endif
    }

    StringArray list;
    priv->world->getSupportedPlugins (list);
    return list;
}

FileSearchPath LV2PluginFormat::getDefaultLocationsToSearch()
{
    FileSearchPath paths;
#if JUCE_LINUX
    paths.add (File ("/usr/lib/lv2"));
    paths.add (File ("/usr/local/lib/lv2"));
#elif JUCE_MAC
    paths.add (File ("/Library/Audio/Plug-Ins/LV2"));
    paths.add (File::getSpecialLocation (File::userHomeDirectory)
                   .getChildFile ("Library/Audio/Plug-Ins/LV2"));
#endif
    return paths;
}

bool LV2PluginFormat::doesPluginStillExist (const PluginDescription& desc)
{
    StringArray plugins (searchPathsForPlugins (FileSearchPath(), true));
    return plugins.contains (desc.fileOrIdentifier);
}

void LV2PluginFormat::createPluginInstance (const PluginDescription& desc, double initialSampleRate, int initialBufferSize, PluginCreationCallback callback)
{
    if (desc.pluginFormatName != String ("LV2"))
    {
        callback (nullptr, "Not an LV2 plugin");
        return;
    }

    if (LV2Module* module = priv->createModule (desc.fileOrIdentifier))
    {
        Result res (module->instantiate (initialSampleRate));
        if (res.wasOk())
        {
            AudioPluginInstance* i = new LV2Processor (*priv->world, module);
            callback (std::unique_ptr<AudioPluginInstance> (i), {});
        }
        else
        {
            deleteAndZero (module);
            callback (nullptr, res.getErrorMessage());
        }
    }
    else
    {
        EL_LV2_LOG ("Failed creating LV2 plugin instance");
        callback (nullptr, "Failed creating LV2 plugin instance");
    }
}

#endif

} // namespace element

JUCE_END_IGNORE_WARNINGS_GCC_LIKE
