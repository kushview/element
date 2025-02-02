// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <lv2/midi/midi.h>
#include <lv2/time/time.h>
#include <lv2/patch/patch.h>

#include <lvtk/lvtk.hpp>
#include <lvtk/ext/atom.hpp>
#include <lvtk/host/world.hpp>
#include <lvtk/host/instance.hpp>

#include <element/juce/core.hpp>
#include <element/juce/gui_basics.hpp>
#include <element/juce/gui_extra.hpp>

#include <element/context.hpp>
#include <element/filesystem.hpp>
#include <element/lv2.hpp>
#include <element/symbolmap.hpp>
#include <element/ui/nodeeditor.hpp>
#include <element/nodefactory.hpp>

#include "lv2/logfeature.hpp"
#include "lv2/module.hpp"
#include "lv2/world.hpp"
#include "lv2/workthread.hpp"
#include "lv2/workerfeature.hpp"
#include "lv2/native.hpp"
#include "engine/portbuffer.hpp"

#if JUCE_MAC
#include "ui/nsviewwithparent.hpp"
#endif

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wdeprecated-declarations",
                                     "-Wunused-variable")

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
#if 0
LV2_ATOM_SEQUENCE_FOREACH ((LV2_Atom_Sequence*) atomIn->getPortData(), ev)
{
    if (lv2_atom_forge_is_object_type (&forge, ev->body.type))
    {
        auto obj = (LV2_Atom_Object*) &ev->body;
        if (obj->body.otype == urids.patch_Set)
        {
            std::clog << "SET\n";
            // Get the property and value of the set message
            const LV2_Atom* property = NULL;
            const LV2_Atom* value = NULL;

            // clang-format off
            lv2_atom_object_get(obj,
                                urids.patch_property, &property,
                                urids.patch_value,    &value,
                                0);
            // clang-format on

            if (! property)
            {
                std::clog << "Set message with no property\n";
            }

            else if (property->type != urids.atom_URID)
            {
                std::clog << "Set property is not a URID\n";
                return;
            }

            const uint32_t key = ((const LV2_Atom_URID*) property)->body;
            std::clog << "key=" << module->getWorld().unmap (key) << std::endl;
        }
    }
}
#endif

template <typename Other, typename Word>
static auto wordCast (Word word)
{
    static_assert (sizeof (word) == sizeof (Other), "Word sizes must match");
    return juce::readUnaligned<Other> (&word);
}

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
class LV2PatchParameter : public PatchParameter
{
public:
    LV2PatchParameter (LV2Module& m, const LV2PatchInfo& i, int idx, LV2_URID_Map* map)
        : PatchParameter (PatchParameter::RangePath),
          module (m),
          patch (i),
          protocol (m.map (LV2_ATOM__eventTransfer)),
          patch_Get (m.map (LV2_PATCH__Get)),
          patch_Set (m.map (LV2_PATCH__Set)),
          patch_property (m.map (LV2_PATCH__property)),
          subject_key (i.subject),
          patch_value (m.map (LV2_PATCH__value)),
          atom_Path (m.map (LV2_ATOM__Path)),
          atom_URID (m.map (LV2_ATOM__URID)),
          _index (idx)
    {
        memset (buffer, 0, 1024);
        lv2_atom_forge_init (&forge, map);
    }

    constexpr auto notifyPort() const noexcept { return patch.notifyPort; }
    int getPortIndex() const noexcept override { return static_cast<int> (patch.port); }
    int getParameterIndex() const noexcept override { return _index; }

    String getName (int maxLen) const override
    {
        return { patch.label.substr (0, (size_t) maxLen) };
    }

    // Units e.g. Hz
    String getLabel() const override { return {}; }

    void request() { write (PatchParameter::Get, 0, nullptr); }

    void update (const LV2_Atom_Object* obj)
    {
        if (obj->body.otype == patch_Set)
        {
            // Get the property and value of the set message
            const LV2_Atom* property = NULL;
            const LV2_Atom* value = NULL;

            // clang-format off
            lv2_atom_object_get(obj,
                                patch_property, &property,
                                patch_value,    &value,
                                0);
            // clang-format on

            if (value == nullptr || property == nullptr || property->type != atom_URID)
                return;

            if (subject_key != ((const LV2_Atom_URID*) property)->body)
                return;
#if 0
            std::clog << "value type: " << module.getWorld().unmap (value->type) << std::endl;
            std::clog << "value size: " << (int) value->size << std::endl;
#endif
            buffer2Size = value->size;
            memset (buffer2, 0, sizeof (buffer2));
            memcpy (buffer2, LV2_ATOM_BODY (value), value->size);

            sigChanged();
        }
    }

    juce::String getCurrentValueAsText() const override
    {
        return buffer2Size > 0 ? juce::String::fromUTF8 ((const char*) buffer2, buffer2Size)
                               : juce::String();
    }

    void write (Operation op, uint32_t size, const void* data) override
    {
        lv2_atom_forge_set_buffer (&forge, buffer, 1024);
        LV2_Atom_Forge_Frame frame;
        // clang-format off
        const auto otype = op == Get ? patch_Get 
                         : op == Set ? patch_Set : 0;
        // clang-format on
        if (otype)
        {
            LV2_Atom_Forge_Ref set =
                lv2_atom_forge_object (&forge, &frame, 0, otype);

            lv2_atom_forge_key (&forge, patch_property);
            lv2_atom_forge_urid (&forge, subject_key);

            if (otype == patch_Set)
            {
                lv2_atom_forge_key (&forge, patch_value);

                switch (range())
                {
                    case RangePath:
                        lv2_atom_forge_path (&forge, (const char*) data, size);
                        break;
                    default:
                        lv2_atom_forge_raw (&forge, data, size);
                        break;
                }
            }

            lv2_atom_forge_pop (&forge, &frame);

            auto atom = (LV2_Atom*) set;
            module.write (patch.port,
                          lv2_atom_total_size (atom),
                          protocol,
                          atom);
        }
    }

private:
    LV2Module& module;
    const LV2PatchInfo patch;
    const uint32_t protocol;
    const uint32_t patch_Get;
    const uint32_t patch_Set;
    const uint32_t patch_property;
    const uint32_t subject_key;
    const uint32_t patch_value;
    const uint32_t atom_Path;
    const uint32_t atom_URID;

    const int _index;
    const uint32_t atomTransfer = [&] { return module.map (LV2_ATOM__atomTransfer); }();
    const uint32_t eventTransfer = [&] { return module.map (LV2_ATOM__eventTransfer); }();
    uint8_t buffer[1024];
    LV2_Atom_Forge forge;
    uint8_t buffer2[1024];
    uint32_t buffer2Size = 0;
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
          tempBuffer (1, 1),
          module (module_),
          urids (world.symbols())
    {
        setName (module->getName());

        numPorts = module->getNumPorts();
        atomControlIn = module->bestAtomPort (true);
        atomControlOut = module->bestAtomPort (false);

        auto& sym = world.symbols();
        forge.init (sym);

        wantsMidiMessages = atomControlIn != EL_INVALID_PORT;
        sendsMidiMessages = atomControlOut != EL_INVALID_PORT;

        setPorts (module->ports());

        int pi = 0;
        for (const auto& patch : module->getPatches())
            addPatch (new LV2PatchParameter (*module, patch, pi++, sym));

        for (auto* p : getPatches())
            if (auto patch = dynamic_cast<LV2PatchParameter*> (p))
                patch->request();

        const ChannelConfig& channels (module->getChannelConfig());
        totalAudioIn = channels.getNumAudioInputs();
        totalAudioOut = channels.getNumAudioOutputs();
        totalAtomIn = channels.getNumAtomInputs();
        totalAtomOut = channels.getNumAtomOutputs();

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
        if (protocol == 0)
        {
            // clang-format off
            for (auto param : getParameters()) {
                if (auto lv2param = dynamic_cast<LV2Parameter*> (param))
                    if (lv2param->getPort() == port)
                        { lv2param->update (*(float*) data, true); break; }
            }
            // clang-format on
        }
        else if (protocol == urids.atom_eventTransfer)
        {
            auto atom = reinterpret_cast<const LV2_Atom*> (data);
            if (atom->type == urids.atom_Object)
            {
                auto obj = (const LV2_Atom_Object*) atom;
                if (obj->body.otype == urids.patch_Set)
                {
                    for (auto* p : getPatches())
                        if (auto patch = dynamic_cast<LV2PatchParameter*> (p))
                            if (patch->notifyPort() == port)
                                patch->update (obj);
                }
            }
        }
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
        desc.isInstrument = module->isInstrument();
        desc.lastInfoUpdateTime = Time::getCurrentTime();
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

        wantsMidiMessages = atomControlIn != EL_INVALID_PORT;
        sendsMidiMessages = atomControlOut != EL_INVALID_PORT;
        initialised = true;
        setLatencySamples (0);
    }

    //==============================================================================
    void prepareToRender (double sampleRate, int blockSize) override
    {
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

    void writeTimeInfoToPort (AtomBuffer& port)
    {
        if (! module->wantsTime())
            return;

        auto* playhead = getPlayHead();
        if (playhead == nullptr)
        {
            return;
        }

        // Write timing info to the control port
        const auto info = playhead->getPosition();
        if (! info.hasValue())
            return;

        lv2_atom_forge_set_buffer (&forge, timeBuf, sizeof (timeBuf));
        LV2_Atom_Forge_Frame frame;
        auto ref = (LV2_Atom*) lv2_atom_forge_object (&forge, &frame, 0, urids.time_Position);

        if (const auto bar = info->getBarCount())
        {
            lv2_atom_forge_key (&forge, urids.time_bar);
            lv2_atom_forge_long (&forge, *bar);
        }

        if (const auto beat = info->getPpqPosition())
        {
            if (const auto barStart = info->getPpqPositionOfLastBarStart())
            {
                lv2_atom_forge_key (&forge, urids.time_barBeat);
                lv2_atom_forge_float (&forge, (float) (*beat - *barStart));
            }

            lv2_atom_forge_key (&forge, urids.time_beat);
            lv2_atom_forge_double (&forge, *beat);
        }

        if (const auto sig = info->getTimeSignature())
        {
            lv2_atom_forge_key (&forge, urids.time_beatUnit);
            lv2_atom_forge_int (&forge, sig->denominator);

            lv2_atom_forge_key (&forge, urids.time_beatsPerBar);
            lv2_atom_forge_float (&forge, (float) sig->numerator);
        }

        if (const auto bpm = info->getBpm())
        {
            lv2_atom_forge_key (&forge, urids.time_beatsPerMinute);
            lv2_atom_forge_float (&forge, (float) *bpm);
        }

        if (const auto samples = info->getTimeInSamples())
        {
            lv2_atom_forge_key (&forge, urids.time_frame);
            lv2_atom_forge_long (&forge, *samples);
        }

        lv2_atom_forge_key (&forge, urids.time_speed);
        lv2_atom_forge_float (&forge, info->getIsPlaying() ? 1.0f : 0.0f);

        lv2_atom_forge_pop (&forge, &frame);
        port.insert (0, ref->size, ref->type, LV2_ATOM_BODY (ref));
    }

    void renderBypassed (RenderContext& rc) override
    {
        const auto numSamples = rc.audio.getNumSamples();
        rc.midi.clear();
        for (int adc = totalAudioIn; adc < totalAudioOut; ++adc)
            rc.audio.clear (adc, 0, numSamples);
        for (int atc = totalAtomIn; atc < totalAtomOut; ++atc)
            rc.atom.clear (atc, 0, numSamples);
    }

    void render (RenderContext& rc) override
    {
        const auto numSamples = rc.audio.getNumSamples();

        if (! initialised)
        {
            for (int i = 0; i < totalAudioOut; ++i)
                rc.audio.clear (i, 0, numSamples);
            rc.midi.clear (0, numSamples);
            rc.atom.clear (0, numSamples);
            return;
        }

        if (auto const atomIn = wantsMidiMessages ? rc.atom.writeBuffer (0) : nullptr)
        {
            writeTimeInfoToPort (*atomIn);
#if 0
            LV2_ATOM_SEQUENCE_FOREACH (atomIn->sequence(), ev)
            {

                if (ev->body.type == urids.midi_MidiEvent) {
                    MidiMessage msg (LV2_ATOM_BODY (&ev->body), ev->body.size, 0.0);
                    std::clog << msg.getDescription() << std::endl;
                }

            }
#endif
        }

        module->referBuffers (rc);
        module->processEvents();
        module->run ((uint32) numSamples);
        rc.midi.clear();
        rc.atom.clear();

        if (auto const atomIn = sendsMidiMessages ? module->getPortBuffer (atomControlOut) : nullptr)
        {
            const auto ch = module->ports().getChannelForPort ((int) atomControlOut);
            auto seq = (LV2_Atom_Sequence*) atomIn->getPortData();
            auto out = rc.atom.writeBuffer (ch);

            LV2_ATOM_SEQUENCE_FOREACH (seq, ev)
            {
#if 0
                if (ev->body.type == urids.midi_MidiEvent) {
                    MidiMessage msg (LV2_ATOM_BODY (&ev->body), ev->body.size, 0.0);
                    std::clog << msg.getDescription() << std::endl;
                }
#endif
                out->insert (ev->time.frames,
                             ev->body.size,
                             ev->body.type,
                             LV2_ATOM_BODY (&ev->body));
            }

            atomIn->clear();
        }
    }

    //==========================================================================
    bool wantsContext() const noexcept override { return true; }

    double getTailLengthSeconds() const { return 0.0f; }
    void* getPlatformSpecificData() { return module->getHandle(); }

    bool silenceInProducesSilenceOut() const { return false; }
    bool acceptsMidi() const { return wantsMidiMessages; }
    bool producesMidi() const { return sendsMidiMessages; }

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
        initialised;
    mutable StringArray programNames;

    AudioSampleBuffer tempBuffer;
    std::unique_ptr<LV2Module> module;
    OwnedArray<PortBuffer> buffers;

    uint32 numPorts { 0 };
    uint32 atomControlIn { EL_INVALID_PORT };
    uint32 atomControlOut { EL_INVALID_PORT };
    uint8_t timeBuf[512] = { 0 };

    int totalAudioIn { 0 },
        totalAudioOut { 0 },
        totalAtomIn { 0 },
        totalAtomOut { 0 };

    struct URIDs
    {
        template <class Sym>
        URIDs (Sym&& sym)
            : atom_eventTransfer (sym.map (LV2_ATOM__eventTransfer)),
              atom_Object (sym.map (LV2_ATOM__Object)),
              atom_Sequence (sym.map (LV2_ATOM__Sequence)),
              atom_URID (sym.map (LV2_ATOM__URID)),
              midi_MidiEvent (sym.map (LV2_MIDI__MidiEvent)),
              patch_Set (sym.map (LV2_PATCH__Set)),
              patch_property (sym.map (LV2_PATCH__property)),
              patch_value (sym.map (LV2_PATCH__value)),
              time_Position (sym.map (LV2_TIME__Position)),
              time_speed (sym.map (LV2_TIME__speed)),
              time_frame (sym.map (LV2_TIME__frame)),
              time_bar (sym.map (LV2_TIME__bar)),
              time_barBeat (sym.map (LV2_TIME__barBeat)),
              time_beat (sym.map (LV2_TIME__beat)),
              time_beatUnit (sym.map (LV2_TIME__beatUnit)),
              time_beatsPerBar (sym.map (LV2_TIME__beatsPerBar)),
              time_beatsPerMinute (sym.map (LV2_TIME__beatsPerMinute))
        {
        }

        const LV2_URID
            atom_eventTransfer,
            atom_Object,
            atom_Sequence,
            atom_URID,
            midi_MidiEvent,
            patch_Set,
            patch_property,
            patch_value,
            time_Position,
            time_speed,
            time_frame,
            time_bar,
            time_barBeat,
            time_beat,
            time_beatUnit,
            time_beatsPerBar,
            time_beatsPerMinute;
    };

    const URIDs urids;
    lvtk::Forge forge;

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

extern bool getNativeWindowSize (void* window, int& width, int& height);

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
            getNativeWindowSize (ui->getWidget(), w, h);
        }

        setSize (w > 0 ? w : 640, h > 0 ? h : 360);
        startTimerHz (60);

        setResizable (ui->haveClientResize());

        if (ui->haveClientResize())
            ui->onClientResize = []() -> int { return 0; };

        nativeViewSetup = true;
    }

    ~LV2NativeEditor()
    {
        nativeViewSetup = false;
        stopTimer();

        view->prepareForDestruction();
        view.reset();

        if (ui != nullptr)
        {
            ui->unload();
            ui = nullptr;
        }
    }

    void viewRequestedResizeInPhysicalPixels (int width, int height) override
    {
    }

    void timerCallback() override
    {
        if (! nativeViewSetup || ! ui || ! ui->isNative())
            return stopTimer();

        if (ui->haveIdleInterface())
        {
            if (ui->idle() != 0)
            {
                stopTimer();
            }
            else
            {
#if JUCE_WINDOWS || JUCE_MAC
                int w = 0, h = 0;
                if (ui != nullptr)
                    if (getNativeWindowSize (ui->getWidget(), w, h))
                        if (w != getWidth() || h != getHeight())
                            setSize (w, h);
#endif
            }
        }
        else
        {
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
    [[maybe_unused]] LV2Processor& plugin;
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
                           public XEmbedComponent,
                           public ComponentMovementWatcher
    {
        explicit ViewComponent (PhysicalResizeListener& l)
            : XEmbedComponent ((unsigned long) inner.getPeer()->getNativeHandle(), true, false),
              ComponentMovementWatcher (&inner),
              listener (inner, l)
        {
            setOpaque (true);
        }

        ~ViewComponent()
        {
            prepareForDestruction();
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

        //==============================================================================
        void componentMovedOrResized (bool wasMoved, bool wasResized) override
        {
            if (wasResized)
            {
                if (auto pc = findParentComponentOfClass<LV2NativeEditor>())
                    pc->setSize (inner.getWidth(), inner.getHeight());
            }
        }

        void componentPeerChanged() override {}
        void componentVisibilityChanged() override {};

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
            {
                setHWND (peer->getNativeHandle());
            }
        }

        ~ViewComponent()
        {
            setHWND (nullptr);
        }

        void paint (Graphics& g) override { g.fillAll (Colours::black); }

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
    LV2 (LV2NodeProvider& p, SymbolMap& s)
        : _symowned (false),
          _symbols (&s),
          provider (p)
    {
        init();
    }

    LV2 (LV2NodeProvider& p)
        : _symowned (true),
          _symbols (new SymbolMap()),
          provider (p)
    {
        init();
    }

    ~LV2()
    {
        world.reset();
        if (_symowned && _symbols != nullptr)
            delete _symbols;
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
    bool _symowned = false;
    SymbolMap* _symbols { nullptr };
    [[maybe_unused]] LV2NodeProvider& provider;
    std::unique_ptr<World> world;
    void init()
    {
        auto path = provider.defaultSearchPath();
        world = std::make_unique<World> (*_symbols, path);
    }
};

LV2NodeProvider::LV2NodeProvider (SymbolMap& s)
{
    lv2 = std::make_unique<LV2> (*this, s);
}

LV2NodeProvider::LV2NodeProvider()
{
    lv2 = std::make_unique<LV2> (*this);
}

LV2NodeProvider::~LV2NodeProvider()
{
    lv2.reset();
}

void LV2NodeProvider::scan (const String& uri, OwnedArray<PluginDescription>& out)
{
    if (auto i = create (uri))
    {
        auto d = out.add (new PluginDescription());
        i->getPluginDescription (*d);
        delete i;
    }
}

Processor* LV2NodeProvider::create (const String& uri)
{
    return lv2->instantiate (uri);
}

FileSearchPath LV2NodeProvider::defaultSearchPath()
{
    FileSearchPath paths;
    [[maybe_unused]] const auto userHome = File::getSpecialLocation (File::userHomeDirectory);

#if JUCE_MAC
    paths.add (File::getSpecialLocation (File::userHomeDirectory).getChildFile ("Library/Audio/Plug-Ins/LV2"));
    paths.add (File ("/Library/Audio/Plug-Ins/LV2"));
#elif JUCE_WINDOWS
    auto programFiles = File::getSpecialLocation (File::globalApplicationsDirectory);
    paths.add (programFiles.getChildFile ("Common Files/LV2"));
    paths.add (File::getSpecialLocation (File::userHomeDirectory).getChildFile ("AppData/Local/LV2"));
#elif JUCE_LINUX || JUCE_BSD
    paths.add (userHome.getChildFile (".lv2"));
    paths.add (File ("/usr/local/lib/lv2"));
    paths.add (File ("/usr/lib/lv2"));
#endif

    paths.removeRedundantPaths();

    return paths;
}

StringArray LV2NodeProvider::findTypes (const juce::FileSearchPath&, bool, bool)
{
    StringArray types;
    lv2->getTypes (types);
    return types;
}

String LV2NodeProvider::nameForURI (const String& uri) const noexcept
{
    auto plugin = lv2->world->getPlugin (uri);
    return plugin != nullptr
               ? juce::String (lvtk::Node (lilv_plugin_get_name (plugin)).as_string())
               : String();
}

} // namespace element

JUCE_END_IGNORE_WARNINGS_GCC_LIKE
