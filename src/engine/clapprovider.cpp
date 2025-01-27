#include <clap/clap.h>

#include <element/processor.hpp>

#include "clapprovider.hpp"

#if ! JUCE_WINDOWS
static void _fpreset() {}
// static void _clearfp() {}
#endif

//==============================================================================
// Change this to disable logging of various VST activities
#ifndef CLAP_LOGGING
#define CLAP_LOGGING 1
#endif

#if CLAP_LOGGING
#define CLAP_LOG(a) Logger::writeToLog (a);
#else
#define CLAP_LOG(a)
#endif

//==============================================================================
namespace {

#if 0
static double clapHostTimeNanoseconds() noexcept
{
#if JUCE_WINDOWS
    return timeGetTime() * 1000000.0;
#elif JUCE_LINUX || JUCE_BSD || JUCE_IOS || JUCE_ANDROID
    timeval micro;
    gettimeofday (&micro, nullptr);
    return (double) micro.tv_usec * 1000.0;
#elif JUCE_MAC
    UnsignedWide micro;
    Microseconds (&micro);
    return micro.lo * 1000.0;
#endif
}
#endif

static int insideCLAPCallback = 0;

struct IdleCallRecursionPreventer
{
    IdleCallRecursionPreventer() : isMessageThread (juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        if (isMessageThread)
            ++insideCLAPCallback;
    }

    ~IdleCallRecursionPreventer()
    {
        if (isMessageThread)
            --insideCLAPCallback;
    }

    const bool isMessageThread;
    JUCE_DECLARE_NON_COPYABLE (IdleCallRecursionPreventer)
};

#if JUCE_MAC
static bool makeFSRefFromPath (FSRef* destFSRef, const String& path)
{
    return FSPathMakeRef (reinterpret_cast<const UInt8*> (path.toRawUTF8()), destFSRef, nullptr) == noErr;
}
#endif
} // namespace

namespace element {
namespace detail {
static bool isCLAP (const File& f)
{
#if JUCE_MAC
    return f.isDirectory() && f.hasFileExtentsion ("clap") && f.exists();
#else
    return f.hasFileExtension ("clap") && f.existsAsFile();
#endif
}

static void recursiveSearch (const File& dir, StringArray& res, bool recursive)
{
    for (const auto& iter : RangedDirectoryIterator (dir, false, "*", File::findFilesAndDirectories))
    {
        auto f = iter.getFile();
        bool isPlugin = false;

        if (isCLAP (f))
        {
            isPlugin = true;
            res.add (f.getFullPathName());
        }

        if (recursive && (! isPlugin) && f.isDirectory())
            recursiveSearch (f, res, true);
    }
}

static clap_host_t makeHost()
{
    return {
        .clap_version = CLAP_VERSION_INIT,
        .host_data = nullptr,
        .name = "Element",
        .vendor = "Kushview",
        .url = "https://kushview.net/element",
        .version = "1.0.0",
        .get_extension = nullptr,
        .request_restart = nullptr,
        .request_process = nullptr,
        .request_callback = nullptr,
    };
}

static PluginDescription makeDescription (const clap_plugin_descriptor_t* d)
{
    PluginDescription desc;
    desc.pluginFormatName = "CLAP";
    desc.name = String::fromUTF8 (d->name);
    desc.fileOrIdentifier = String::fromUTF8 (d->id);
    desc.manufacturerName = String::fromUTF8 (d->vendor);
    desc.uniqueId = 0;
    return desc;
}

} // namespace detail

class CLAPEventBuffer
{
public:
    using size_type = uint32_t;

    CLAPEventBuffer()
        : _data (8192),
          _ptr ((uint8_t*) _data.data())
    {
        _ins.ctx = _outs.ctx = this;
        _ins.get = &_get;
        _ins.size = &_size;
        _outs.try_push = &_tryPush;
    }

    ~CLAPEventBuffer() {}

    clap_event_header_t* next() noexcept
    {
        return (clap_event_header_t*) (_ptr + _used);
    }

    bool push (const clap_event_header_t* ev)
    {
        if (ev->size + _used > _capacity)
            return false;

        const auto size_needed = ev->size;
        auto dst = (clap_event_header_t*) (_ptr + _used);
        bool dirty = false;
#if 1
        {
            auto i = (clap_event_header_t*) _ptr;
            uint32_t ib = 0;
            while (i != nullptr)
            {
                if (i->time > ev->time)
                {
                    std::memmove (((uint8_t*) i) + size_needed,
                                  i,
                                  (uint8_t*) dst - (uint8_t*) i);
                    dst = i;
                    dirty = true;
                    break;
                }

                ib += i->size;
                i = ib < _used
                        ? (clap_event_header_t*) ((uint8_t*) i + i->size)
                        : nullptr;
            }
        }
#endif

        assert (dst != nullptr);
        std::memcpy (dst, ev, ev->size);
        _used += dst->size;
        ++_count;
        if (dirty)
            remap();
        return true;
    }

    uint32_t capacity() const noexcept { return _capacity; }

    uint32_t size() const noexcept { return _count; }

    const clap_event_header_t* get (uint32_t index) const noexcept
    {
        assert (index < _bytemap.size() && index < _count);
        return (const clap_event_header_t*) (_ptr + _bytemap[index]);
    }

    const clap_input_events_t* inputs() const noexcept { return &_ins; }
    const clap_output_events_t* outputs() const noexcept { return &_outs; }

    void remap() noexcept
    {
        _bytemap.resize (_count);
        auto i = (clap_event_header_t*) _ptr;
        uint32_t idx = 0, ib = 0;
        while (idx < _count && i != nullptr)
        {
            _bytemap[idx] = ib;

            ++idx;
            ib += i->size;
            i = ib < _used
                    ? (clap_event_header_t*) ((uint8_t*) i + i->size)
                    : nullptr;
        }
    }

    void clear() noexcept
    {
        _count = _used = 0;
        _bytemap.resize (0);
    }

private:
    AlignedData<8UL> _data;
    uint8_t* _ptr { nullptr };
    uint32_t _capacity { 0 },
        _used { 0 },
        _count { 0 };
    std::vector<uint32_t> _bytemap;
    clap_input_events_t _ins;
    clap_output_events_t _outs;

    static uint32_t _size (const clap_input_events_t* list)
    {
        return ((CLAPEventBuffer*) list->ctx)->_count;
    }

    static const clap_event_header_t* _get (const clap_input_events_t* list, uint32_t index)
    {
        auto self = (CLAPEventBuffer*) list->ctx;
        return self->get (index);
    }

    static bool _tryPush (const clap_output_events_t* oe, const clap_event_header_t* ev)
    {
        auto self = (CLAPEventBuffer*) oe->ctx;
        return self->push (ev);
    }
};

//==============================================================================
using CLAPEntry = const clap_plugin_entry_t*;

//==============================================================================
struct CLAPModule final : public ReferenceCountedObject
{
    File file;
    CLAPEntry moduleMain;
    String pluginName;
    std::unique_ptr<XmlElement> vstXml;

    using Ptr = ReferenceCountedObjectPtr<CLAPModule>;

    static Array<CLAPModule*>& activeModules()
    {
        static Array<CLAPModule*> activeModules;
        return activeModules;
    }

    //==============================================================================
    static Ptr findOrCreate (const File& file)
    {
        for (auto* module : activeModules())
            if (module->file == file)
                return module;

        const IdleCallRecursionPreventer icrp;
        _fpreset();

        CLAP_LOG ("Attempting to load CLAP: " + file.getFullPathName());

        Ptr m = new CLAPModule (file);

        if (m->open())
        {
            _fpreset();
            return m;
        }

        return {};
    }

    static Ptr findByID (std::string_view ID)
    {
        Ptr mod;
        for (auto m : activeModules())
        {
            for (uint32_t i = 0; i < m->size(); ++i)
            {
                auto dp = m->descriptor (i);

                if (std::strcmp (dp->id, ID.data()) == 0)
                    return m;
            }
        }
        return nullptr;
    }

    //==============================================================================
    CLAPModule (const File& f)
        : file (f), moduleMain (nullptr)
    {
        activeModules().add (this);

#if JUCE_WINDOWS || JUCE_LINUX || JUCE_BSD || JUCE_IOS || JUCE_ANDROID
        fullParentDirectoryPathName = f.getParentDirectory().getFullPathName();
#elif JUCE_MAC
        FSRef ref;
        makeFSRefFromPath (&ref, f.getParentDirectory().getFullPathName());
        FSGetCatalogInfo (&ref, kFSCatInfoNone, nullptr, nullptr, &parentDirFSSpec, nullptr);
#endif
    }

    ~CLAPModule()
    {
        activeModules().removeFirstMatchingValue (this);
        close();
    }

    uint32_t size() const noexcept
    {
        return _factory->get_plugin_count (_factory);
    }

    const clap_plugin_descriptor_t* descriptor (uint32_t index) const noexcept
    {
        return _factory->get_plugin_descriptor (_factory, index);
    }

    const clap_plugin_t* create (clap_host_t* host, std::string_view pluginID)
    {
        if (auto plug = _factory->create_plugin (_factory, host, pluginID.data()))
        {
            if (! plug->init (plug))
            {
                plug->destroy (plug);
                return nullptr;
            }

            return plug;
        }

        return nullptr;
    }

    //==============================================================================
#if ! JUCE_MAC
    String fullParentDirectoryPathName;
#endif

#if JUCE_WINDOWS || JUCE_LINUX || JUCE_BSD || JUCE_ANDROID
    DynamicLibrary module;

    bool open()
    {
        if (moduleMain != nullptr)
            return true;

        pluginName = file.getFileNameWithoutExtension();

        module.open (file.getFullPathName());

        moduleMain = (CLAPEntry) module.getFunction ("clap_entry");

        if (moduleMain)
        {
            _initialized = moduleMain->init (file.getFullPathName().toRawUTF8());
            _factory = (clap_plugin_factory*) (_initialized
                                                   ? moduleMain->get_factory (CLAP_PLUGIN_FACTORY_ID)
                                                   : nullptr);
        }

        return _initialized;
    }

    void close()
    {
        if (moduleMain != nullptr)
        {
            if (_initialized && moduleMain->deinit)
                moduleMain->deinit();
            moduleMain = nullptr;
        }

        _initialized = false;
        _fpreset(); // (doesn't do any harm)
        module.close();
    }

#else
    Handle resHandle = {};
    CFUniquePtr<CFBundleRef> bundleRef;

#if JUCE_MAC
    CFBundleRefNum resFileId = {};
    FSSpec parentDirFSSpec;
#endif

    bool open()
    {
        if (moduleMain != nullptr)
            return true;

        bool ok = false;

        if (file.hasFileExtension (".vst"))
        {
            auto* utf8 = file.getFullPathName().toRawUTF8();

            if (auto url = CFUniquePtr<CFURLRef> (CFURLCreateFromFileSystemRepresentation (nullptr, (const UInt8*) utf8, (CFIndex) strlen (utf8), file.isDirectory())))
            {
                bundleRef.reset (CFBundleCreate (kCFAllocatorDefault, url.get()));

                if (bundleRef != nullptr)
                {
                    if (CFBundleLoadExecutable (bundleRef.get()))
                    {
                        moduleMain = (CLAPEntry) CFBundleGetFunctionPointerForName (bundleRef.get(), CFSTR ("main_macho"));

                        if (moduleMain == nullptr)
                            moduleMain = (CLAPEntry) CFBundleGetFunctionPointerForName (bundleRef.get(), CFSTR ("VSTPluginMain"));

                        JUCE_VST_WRAPPER_LOAD_CUSTOM_MAIN

                        if (moduleMain != nullptr)
                        {
                            if (CFTypeRef name = CFBundleGetValueForInfoDictionaryKey (bundleRef.get(), CFSTR ("CFBundleName")))
                            {
                                if (CFGetTypeID (name) == CFStringGetTypeID())
                                {
                                    char buffer[1024];

                                    if (CFStringGetCString ((CFStringRef) name, buffer, sizeof (buffer), CFStringGetSystemEncoding()))
                                        pluginName = buffer;
                                }
                            }

                            if (pluginName.isEmpty())
                                pluginName = file.getFileNameWithoutExtension();

#if JUCE_MAC
                            resFileId = CFBundleOpenBundleResourceMap (bundleRef.get());
#endif

                            ok = true;

                            auto vstXmlFiles = file
#if JUCE_MAC
                                                   .getChildFile ("Contents")
                                                   .getChildFile ("Resources")
#endif
                                                   .findChildFiles (File::findFiles, false, "*.vstxml");

                            if (! vstXmlFiles.isEmpty())
                                vstXml = parseXML (vstXmlFiles.getReference (0));
                        }
                    }

                    if (! ok)
                    {
                        CFBundleUnloadExecutable (bundleRef.get());
                        bundleRef = nullptr;
                    }
                }
            }
        }

        return ok;
    }

    void close()
    {
        if (bundleRef != nullptr)
        {
#if JUCE_MAC
            CFBundleCloseBundleResourceMap (bundleRef.get(), resFileId);
#endif

            if (CFGetRetainCount (bundleRef.get()) == 1)
                CFBundleUnloadExecutable (bundleRef.get());

            if (CFGetRetainCount (bundleRef.get()) > 0)
                bundleRef = nullptr;
        }
    }

    void closeEffect (Vst2::AEffect* eff)
    {
        eff->dispatcher (eff, Vst2::effClose, 0, 0, nullptr, 0);
    }

#endif

private:
    bool _initialized { false };
    const clap_plugin_factory* _factory;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CLAPModule)
};

//==============================================================================
class CLAPParameter : public Parameter
{
    const clap_plugin_t* _plugin;
    const clap_plugin_params_t* _params;
    const clap_param_info_t _info;
    const int _portIndex, _paramIndex;
    std::atomic<float> _value;
    juce::NormalisableRange<double> _range;

public:
    explicit CLAPParameter (const clap_plugin_t* plugin,
                            const clap_plugin_params_t* params,
                            const clap_param_info_t* pi,
                            int portIndex,
                            int paramIndex)
        : _plugin (plugin), _params (params), _info (*pi), _portIndex (portIndex), _paramIndex (paramIndex)
    {
        _range.start = _info.min_value;
        _range.end = _info.max_value;
        auto dv = _range.convertTo0to1 (_info.default_value);
        _value.store (static_cast<float> (dv));
    }

    ~CLAPParameter() {}

    //==========================================================================
    int getPortIndex() const noexcept override { return _portIndex; }
    int getParameterIndex() const noexcept override { return _paramIndex; }
    float getValue() const override { return _value.load(); }

    void setValue (float newValue) override
    {
        _value.store (newValue);
        const auto clapVal = _range.convertFrom0to1 (newValue);
    }

    float getDefaultValue() const { return static_cast<float> (_info.default_value); }

    float getValueForText (const juce::String& text) const override
    {
        double val { 0.0 };
        _params->text_to_value (_plugin, _info.id, text.toRawUTF8(), &val);
        return static_cast<float> (val);
    }

    juce::String getText (float normalisedValue, int maxLen) const override
    {
        std::unique_ptr<char[]> _chars;
        _chars.reset (new char[maxLen + 1]);
        memset (_chars.get(), 0, (size_t) maxLen);

        const auto value = _range.convertFrom0to1 (normalisedValue);
        _params->value_to_text (_plugin, _info.id, value, _chars.get(), (uint32_t) maxLen);

        return String::fromUTF8 (_chars.get());
    }

    juce::String getName (int maximumStringLength) const override { return String::fromUTF8 (_info.name); }
    juce::String getLabel() const override { return {}; }

    int getNumSteps() const override { return Parameter::defaultNumSteps(); }
    bool isDiscrete() const override { return false; }
    bool isBoolean() const override { return false; }

    bool isOrientationInverted() const override
    {
        return Parameter::isOrientationInverted();
    }

    bool isAutomatable() const override
    {
        return true;
    }

    bool isMetaParameter() const override
    {
        return false;
    }

    Category getCategory() const override
    {
        return Parameter::getCategory();
    }

#if 0
    //==============================================================================
    /** Returns the current value of the parameter as a juce::String.

        This function can be called when you are hosting plug-ins to get a
        more specialsed textual represenation of the current value from the
        plug-in, for example "On" rather than "1.0".

        If you are implementing a plug-in then you should ignore this function
        and instead override getText.
    */
    virtual juce::String getCurrentValueAsText() const;

    /** Returns the set of strings which represent the possible states a parameter
        can be in.

        If you are hosting a plug-in you can use the result of this function to
        populate a ComboBox listing the allowed values.

        If you are implementing a plug-in then you do not need to override this.
    */
    virtual juce::StringArray getValueStrings() const;
#endif
};

//==============================================================================
class CLAPProcessor : public Processor
{
public:
    ~CLAPProcessor()
    {
        if (_plugin != nullptr)
        {
            _plugin->destroy (_plugin);
            _plugin = nullptr;
        }

        for (auto& ib : _audioIns)
        {
            if (ib.data32 != nullptr)
                std::free (ib.data32);
            if (ib.data64 != nullptr)
                std::free (ib.data64);
        }
        _audioIns.clear();

        for (auto& ob : _audioOuts)
        {
            if (ob.data32 != nullptr)
                std::free (ob.data32);
            if (ob.data64 != nullptr)
                std::free (ob.data64);
        }
        _audioOuts.clear();
    }

    static std::unique_ptr<CLAPProcessor> create (const String& pluginID, double r, int b)
    {
        std::unique_ptr<CLAPProcessor> ptr;
        auto ID = pluginID.upToFirstOccurrenceOf (":", false, false);
        auto path = pluginID.fromFirstOccurrenceOf (":", false, false);
        auto mod = CLAPModule::findByID (ID.toRawUTF8());

        if (mod == nullptr && File::isAbsolutePath (path))
            mod = CLAPModule::findOrCreate (File (path));

        if (mod != nullptr)
        {
            ptr.reset (new CLAPProcessor (mod, ID));
            if (ptr->_plugin == nullptr || ! ptr->init())
                ptr.reset();
        }

        return ptr;
    }

    //==========================================================================
    void setPlayHead (AudioPlayHead* playhead)
    {
        Processor::setPlayHead (playhead);
    }

    AudioPlayHead* getPlayHead() const noexcept
    {
        return Processor::getPlayHead();
    }

    //==========================================================================
    void prepareToRender (double sampleRate, int maxBufferSize) override
    {
        if (_plugin == nullptr)
            return;
        _plugin->start_processing (_plugin);
        _plugin->activate (_plugin, sampleRate, 16U, (uint32_t) maxBufferSize);
    }

    void releaseResources() override
    {
        if (_plugin == nullptr)
            return;
        _plugin->deactivate (_plugin);
        _plugin->stop_processing (_plugin);
    }

    void render (RenderContext& rc) override
    {
        _proc.steady_time = -1;
        _proc.frames_count = rc.audio.getNumSamples();
        _proc.transport = nullptr;
        _eventIn.clear();
        if (_notes != nullptr)
        {
        }
        _proc.in_events = _eventIn.inputs();
        _eventOut.clear();
        _proc.out_events = _eventOut.outputs();

        int rcc = 0;
        for (uint32_t i = 0; i < _proc.audio_inputs_count; ++i)
        {
            auto b = &_proc.audio_inputs[i];
            for (uint32_t j = 0; j < b->channel_count; ++j)
            {
                b->data32[j] = (float*) rc.audio.getReadPointer (rcc++);
            }
        }

        rcc = 0;
        for (uint32_t i = 0; i < _proc.audio_outputs_count; ++i)
        {
            auto b = &_proc.audio_outputs[i];
            for (uint32_t j = 0; j < b->channel_count; ++j)
            {
                b->data32[j] = rc.audio.getWritePointer (rcc++);
            }
        }

        _plugin->process (_plugin, &_proc);
    }

    void renderBypassed (RenderContext&) override {}

    //==========================================================================
    void refreshPorts() override {}

    //==========================================================================
    void getPluginDescription (PluginDescription& desc) const override
    {
        Processor::getPluginDescription (desc);
        desc = detail::makeDescription (_plugin->desc);
    }

    //==========================================================================
    static int64_t writeState (const clap_ostream_t* stream, const void* buffer, uint64_t size)
    {
        auto& mo = *(MemoryOutputStream*) stream->ctx;
        if (! mo.write (buffer, size))
            return -1;
        return static_cast<int64_t> (size);
    }

    void getState (MemoryBlock& block) override
    {
        if (auto state = (const clap_plugin_state_t*) _plugin->get_extension (_plugin, CLAP_EXT_STATE))
        {
            MemoryOutputStream mo (block, true);
            clap_ostream_t stream;
            stream.ctx = (void*) &mo,
            stream.write = &writeState;
            state->save (_plugin, &stream);
        }
    }

    static int64_t readState (const clap_istream* stream, void* buffer, uint64_t size)
    {
        auto& mi = *(MemoryInputStream*) stream->ctx;
        return mi.read (buffer, (int) size);
    }

    void setState (const void* data, int size) override
    {
        if (auto state = (const clap_plugin_state_t*) _plugin->get_extension (_plugin, CLAP_EXT_STATE))
        {
            MemoryInputStream mi (data, (size_t) size, false);
            clap_istream_t stream;
            stream.ctx = (void*) &mi;
            stream.read = &readState;
            state->load (_plugin, &stream);
        }
    }

    //==========================================================================
    bool hasEditor() override { return false; }
    Editor* createEditor() { return nullptr; }

protected:
    void initialize() override
    {
        Processor::initialize();
    }

    //==========================================================================
    ParameterPtr getParameter (const PortDescription& port) override
    {
        if (port.type != PortType::Control || _plugin == nullptr || _params == nullptr)
            return nullptr;
        clap_param_info_t info;
        _params->get_info (_plugin, (uint32_t) port.channel, &info);
        return new CLAPParameter (_plugin, _params, &info, port.index, port.channel);
    }

private:
    const String ID;
    CLAPModule::Ptr _module { nullptr };

    clap_host_t _host;
    const clap_plugin_t* _plugin { nullptr };
    const clap_plugin_audio_ports* _audio { nullptr };
    const clap_plugin_note_ports_t* _notes { nullptr };
    const clap_plugin_params_t* _params { nullptr };

    clap_process_t _proc;
    std::vector<clap_audio_buffer_t> _audioIns, _audioOuts;
    CLAPEventBuffer _eventIn, _eventOut;

    CLAPProcessor (CLAPModule::Ptr m, const String& i)
        : Processor (0), ID (i), _module (m)
    {
        _host = detail::makeHost();
        _host.host_data = this;
        _host.get_extension = &get_extension;
        _host.request_callback = &request_callback;
        _host.request_process = &request_process;
        _host.request_restart = &request_restart;
        if (auto plugin = (m != nullptr ? m->create (&_host, ID.toRawUTF8()) : nullptr))
            _plugin = plugin;
    }

    const void* extension (const char* ext) const noexcept
    {
        return _plugin != nullptr ? _plugin->get_extension (_plugin, ext)
                                  : nullptr;
    }

    //==========================================================================
    bool init()
    {
        if (_plugin == nullptr)
            return false;
        if (! _plugin->init (_plugin))
            return false;

        PortCount pc;
        if (auto audio = (const clap_plugin_audio_ports_t*) extension (CLAP_EXT_AUDIO_PORTS))
        {
            _audio = audio;

            _proc.audio_inputs_count = _audio->count (_plugin, true);
            for (uint32_t i = 0; i < _proc.audio_inputs_count; ++i)
            {
                clap_audio_port_info_t info;
                audio->get (_plugin, i, true, &info);
                pc.inputs[PortType::Audio] += info.channel_count;
                _audioIns.push_back ({});
                auto& buf = _audioIns.back();
                buf.channel_count = info.channel_count;
                buf.data32 = (float**) calloc (buf.channel_count, sizeof (float*));
                buf.data64 = nullptr;
                buf.constant_mask = 0;
                buf.latency = 0;
            }
            _proc.audio_inputs = _audioIns.data();

            _proc.audio_outputs_count = _audio->count (_plugin, false);
            for (uint32_t i = 0; i < audio->count (_plugin, false); ++i)
            {
                clap_audio_port_info_t info;
                audio->get (_plugin, i, false, &info);
                pc.outputs[PortType::Audio] += info.channel_count;
                _audioOuts.push_back ({});
                auto& buf = _audioOuts.back();
                buf.channel_count = info.channel_count;
                buf.data32 = (float**) calloc (buf.channel_count, sizeof (float*));
                buf.data64 = nullptr;
                buf.constant_mask = 0;
                buf.latency = 0;
            }

            _proc.audio_outputs = _audioOuts.data();
        }

        if (auto notes = (const clap_plugin_note_ports_t*) extension (CLAP_EXT_NOTE_PORTS))
        {
            _notes = notes;
            for (uint32_t i = 0; i < notes->count (_plugin, true); ++i)
            {
                clap_note_port_info_t info;
                notes->get (_plugin, i, true, &info);
                ++pc.outputs[PortType::Midi];
            }

            for (uint32_t i = 0; i < notes->count (_plugin, false); ++i)
            {
                clap_note_port_info_t info;
                notes->get (_plugin, i, false, &info);
                ++pc.outputs[PortType::Midi];
            }
        }

        if (auto params = (const clap_plugin_params_t*) extension (CLAP_EXT_PARAMS))
        {
            _params = params;
            for (uint32_t i = 0; i < params->count (_plugin); ++i)
            {
                clap_param_info_t info;
                params->get_info (_plugin, i, &info);
                ++pc.inputs[PortType::Control];
            }
        }

        auto list = pc.toPortList();
        setPorts (list);

        return true;
    }

    //==========================================================================
    static const void* get_extension (const struct clap_host* host, const char* extensionID)
    {
        juce::ignoreUnused (host, extensionID);
        return nullptr;
    }

    static void request_restart (const struct clap_host* host)
    {
        juce::ignoreUnused (host);
    }

    static void request_process (const struct clap_host* host)
    {
        juce::ignoreUnused (host);
    }

    static void request_callback (const struct clap_host* host)
    {
        juce::ignoreUnused (host);
    }
};

//==============================================================================
class CLAPProvider::Host final
{
public:
    Host (CLAPProvider& o)
        : owner (o)
    {
    }

private:
    CLAPProvider& owner;
};

CLAPProvider::CLAPProvider()
{
    _host.reset (new Host (*this));
}

CLAPProvider::~CLAPProvider()
{
    _host.reset();
}

juce::String CLAPProvider::format() const { return "CLAP"; }

Processor* CLAPProvider::create (const juce::String& ID)
{
    std::unique_ptr<CLAPProcessor> result;
    double sampleRate = 44100.0;
    int blockSize = 1024;

    result = CLAPProcessor::create (ID, sampleRate, blockSize);

    String errorMsg;

    if (result == nullptr)
        errorMsg = TRANS ("Unable to load XXX plug-in file").replace ("XXX", "CLAP");

    return result.release();
}

FileSearchPath CLAPProvider::defaultSearchPath()
{
    FileSearchPath sp;

#if JUCE_MAC
    sp.add ("~/Library/Audio/Plug-Ins/CLAP");
    sp.add ("/Library/Audio/Plug-Ins/CLAP");
#elif JUCE_WINDOWS
    auto programFiles = File::getSpecialLocation (File::globalApplicationsDirectory).getFullPathName();
    sp.addIfNotAlreadyThere (programFiles + "\\CLAP");
    sp.removeRedundantPaths();
#else
    sp.add (File::getSpecialLocation (File::userHomeDirectory).getChildFile (".clap"));
    sp.add (File ("/usr/local/lib/clap"));
    sp.add (File ("/usr/lib/clap"));
#endif

    return sp;
}

StringArray CLAPProvider::findTypes (const FileSearchPath& path, bool recursive, bool allowAsync)
{
    juce::ignoreUnused (allowAsync);
    StringArray r;
    for (int i = 0; i < path.getNumPaths(); ++i)
        detail::recursiveSearch (path[i], r, recursive);
    return r;
}

StringArray getHiddenTypes() { return {}; }

void CLAPProvider::scan (const String& fileOrID, OwnedArray<PluginDescription>& out)
{
    const bool isFile = File::isAbsolutePath (fileOrID);
    auto mod = isFile ? CLAPModule::findOrCreate (File (fileOrID))
                      : CLAPModule::findByID (fileOrID.toRawUTF8());
    if (mod == nullptr)
        return;

    for (uint32_t i = 0; i < mod->size(); ++i)
    {
        const auto d = mod->descriptor (i);
        auto pd = out.add (new PluginDescription());
        *pd = detail::makeDescription (d);
        if (isFile)
            pd->fileOrIdentifier << ":" << fileOrID.trim();
    }
}

} // namespace element
