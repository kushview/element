
#if __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#endif

#include <clap/clap.h>
#include <clap/helpers/host.hh>
#include <element/processor.hpp>

#include "clapprovider.hpp"
#include "lv2/messages.hpp"

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

namespace element {
namespace detail {
#if JUCE_MAC
static bool makeFSRefFromPath (FSRef* destFSRef, const String& path)
{
    return FSPathMakeRef (reinterpret_cast<const UInt8*> (path.toRawUTF8()), destFSRef, nullptr) == noErr;
}
#endif

static bool isCLAP (const File& f)
{
#if JUCE_MAC
    return f.isDirectory() && f.hasFileExtension ("clap") && f.exists();
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

static uint32_t totalChannels (const std::vector<clap_audio_buffer_t>& bufs)
{
    uint32_t c = 0;
    for (const auto& buf : bufs)
        c += buf.channel_count;
    return c;
}

static void freeAudioBuffers (std::vector<clap_audio_buffer_t>& bufs)
{
    for (auto& b : bufs)
    {
        if (b.data32 != nullptr)
            std::free (b.data32);
        if (b.data64 != nullptr)
            std::free (b.data64);
    }
    bufs.clear();
}
} // namespace detail

constexpr auto Misbehaviour = clap::helpers::MisbehaviourHandler::Terminate;
constexpr auto Level = clap::helpers::CheckingLevel::Maximal;

#if __APPLE__
template <typename CFType>
struct CFObjectDeleter
{
    void operator() (CFType object) const noexcept
    {
        if (object != nullptr)
            CFRelease (object);
    }
};

template <typename CFType>
using CFUniquePtr = std::unique_ptr<std::remove_pointer_t<CFType>, CFObjectDeleter<CFType>>;
#endif

#if 1
//==============================================================================
using CLAPBaseHost = clap::helpers::Host<Misbehaviour, Level>;
// extern template class clap::helpers::Host<Misbehaviour, Level>;
class CLAPHost final : public CLAPBaseHost
{
public:
    CLAPHost()
        : CLAPBaseHost ("Element", "Kushview", "https://kushview.net/element", "1.0.0")
    {
    }
    ~CLAPHost() {}

    // clap_host_thread_check
    bool threadCheckIsMainThread() const noexcept override
    {
        return juce::MessageManager::getInstance()->isThisTheMessageThread();
    }

    bool threadCheckIsAudioThread() const noexcept
    {
        return false;
    }

protected:
    // clap_host
    void requestRestart() noexcept override
    {
        CLAP_LOG ("host:requestRestart()")
    }
    void requestProcess() noexcept override
    {
        CLAP_LOG ("host:requestProcess()")
    }
    void requestCallback() noexcept override
    {
        CLAP_LOG ("host:requestCallback()")
    }

#if 0
      virtual bool enableDraftExtensions() const noexcept { return false; }
      virtual const void* getExtension (std::string_view extensionId) const noexcept { return nullptr; }

      // clap_host_audio_ports
      virtual bool implementsAudioPorts() const noexcept { return false; }
      virtual bool audioPortsIsRescanFlagSupported(uint32_t flag) noexcept { return false; }
      virtual void audioPortsRescan(uint32_t flags) noexcept {}

      // clap_host_gui
      virtual bool implementsGui() const noexcept { return false; }
      virtual void guiResizeHintsChanged() noexcept {}
      virtual bool guiRequestResize(uint32_t width, uint32_t height) noexcept { return false; }
      virtual bool guiRequestShow() noexcept { return false; }
      virtual bool guiRequestHide() noexcept { return false; }
      virtual void guiClosed(bool wasDestroyed) noexcept {}

      // clap_host_latency
      virtual bool implementsLatency() const noexcept { return false; }
      virtual void latencyChanged() noexcept {}

      // clap_host_log
      virtual bool implementsLog() const noexcept { return false; }
      virtual void logLog(clap_log_severity severity, const char *message) const noexcept {}

      // clap_host_params
      virtual bool implementsParams() const noexcept { return false; }
      virtual void paramsRescan(clap_param_rescan_flags flags) noexcept {}
      virtual void paramsClear(clap_id paramId, clap_param_clear_flags flags) noexcept {}
      virtual void paramsRequestFlush() noexcept {}

      // clap_host_posix_fd_support
      virtual bool implementsPosixFdSupport() const noexcept { return false; }
      virtual bool posixFdSupportRegisterFd(int fd, clap_posix_fd_flags_t flags) noexcept { return false; }
      virtual bool posixFdSupportModifyFd(int fd, clap_posix_fd_flags_t flags) noexcept { return false; }
      virtual bool posixFdSupportUnregisterFd(int fd) noexcept { return false; }

      // clap_host_remote_controls
      virtual bool implementsRemoteControls() const noexcept { return false; }
      virtual void remoteControlsChanged() noexcept {}
      virtual void remoteControlsSuggestPage(clap_id pageId) noexcept {}

      // clap_host_state
      virtual bool implementsState() const noexcept { return false; }
      virtual void stateMarkDirty() noexcept {}

      // clap_host_timer_support
      virtual bool implementsTimerSupport() const noexcept { return false; }
      virtual bool timerSupportRegisterTimer(uint32_t periodMs, clap_id *timerId) noexcept { return false; }
      virtual bool timerSupportUnregisterTimer(clap_id timerId) noexcept { return false; }

      // clap_host_tail
      virtual bool implementsTail() const noexcept { return false; }
      virtual void tailChanged() noexcept {}

      // clap_host_thread_pool
      virtual bool implementsThreadPool() const noexcept { return false; }
      virtual bool threadPoolRequestExec(uint32_t numTasks) noexcept { return false; }
#endif
};
#endif

//==============================================================================
template <typename Locks>
class CLAPEventQueue final
{
public:
    CLAPEventQueue() { data.reserve (initialSize); }

    template <typename Event>
    void push (const Event* ev)
    {
        auto eh = (const clap_event_header_t*) ev;
        write (mutex, [&] {
            // const auto chars = lvtk::to_chars (uint32_t { ev->size });
            // data.insert (data.end(), chars.begin(), chars.end());
            const auto charbuf = (const char*) (eh);
            data.insert (data.end(), charbuf, charbuf + eh->size);
        });
    }

    template <typename Callback>
    void readAll (Callback&& callback)
    {
        read (mutex, [&] {
            if (data.empty())
                return;

            const auto end = data.data() + data.size();
            for (auto ptr = data.data(); ptr < end;)
            {
                const auto ev = (const clap_event_header_t*) ptr;
                callback (ev);
                ptr += ev->size;
            }

            data.resize (0);
        });
    }

private:
    using Read = typename Locks::Read;
    Read read;

    using Write = typename Locks::Write;
    Write write;

    static constexpr auto initialSize = 8192;
    lvtk::SpinLock mutex;
    std::vector<char> data;
};

//==============================================================================
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
        _capacity = _data.size();
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

    void dump()
    {
        for (uint32_t i = 0; i < size(); ++i)
        {
            auto ev = get (i);
            if (ev->type == CLAP_EVENT_PARAM_VALUE)
            {
                auto pv = (const clap_event_param_value_t*) ev;
                std::clog << "[param] event: "
                          << (int64_t) pv->param_id << ": "
                          << pv->value << std::endl;
            }
        }
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
        // auto ev = self->get (index);
        // if (ev->type == CLAP_EVENT_PARAM_VALUE) {
        //     std::cout << "clap::_get(): " << (int) index
        //         << " value: " << ((const clap_event_param_value_t*)ev)->value << std::endl;

        // }

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
        detail::makeFSRefFromPath (&ref, f.getParentDirectory().getFullPathName());
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
    using Handle = void*;
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

        if (file.hasFileExtension (".clap"))
        {
            auto* utf8 = file.getFullPathName().toRawUTF8();

            if (auto url = CFUniquePtr<CFURLRef> (CFURLCreateFromFileSystemRepresentation (nullptr, (const UInt8*) utf8, (CFIndex) strlen (utf8), file.isDirectory())))
            {
                bundleRef.reset (CFBundleCreate (kCFAllocatorDefault, url.get()));

                if (bundleRef != nullptr)
                {
                    if (CFBundleLoadExecutable (bundleRef.get()))
                    {
                        moduleMain = (CLAPEntry) CFBundleGetDataPointerForName (bundleRef.get(), CFSTR ("clap_entry"));

                        if (moduleMain == nullptr)
                            moduleMain = (CLAPEntry) CFBundleGetFunctionPointerForName (bundleRef.get(), CFSTR ("VSTPluginMain"));

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
#endif

private:
    bool _initialized { false };
    const clap_plugin_factory* _factory { nullptr };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CLAPModule)
};

//==============================================================================
class CLAPParameter : public Parameter
{
    using Queue = CLAPEventQueue<lvtk::RealtimeReadTrait>;
    Queue& _queue;
    const clap_plugin_t* _plugin;
    const clap_plugin_params_t* _params;
    const clap_param_info_t _info;
    const int _portIndex, _paramIndex;
    std::atomic<float> _value;
    juce::NormalisableRange<double> _range;

public:
    explicit CLAPParameter (Queue& events,
                            const clap_plugin_t* plugin,
                            const clap_plugin_params_t* params,
                            const clap_param_info_t* pi,
                            int portIndex,
                            int paramIndex)
        : _queue (events),
          _plugin (plugin),
          _params (params),
          _info (*pi),
          _portIndex (portIndex),
          _paramIndex (paramIndex)
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

        clap_event_param_value_t ev;
        ev.header.type = CLAP_EVENT_PARAM_VALUE;
        ev.header.flags = CLAP_EVENT_IS_LIVE;
        ev.header.size = sizeof (clap_event_param_value_t);
        ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
        ev.header.time = 1;

        ev.cookie = _info.cookie;
        ev.param_id = _info.id;
        ev.value = _range.convertFrom0to1 (newValue);

        ev.key = -1;
        ev.note_id = -1;
        ev.port_index = 0;
        ev.channel = -1;

        _queue.push (&ev);

        // std::cout << "[param] send: " << ev.value << std::endl;
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

        detail::freeAudioBuffers (_audioIns);
        detail::freeAudioBuffers (_audioOuts);
        _tmpAudio.setSize (1, 1, false, false, false);
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
    void setPlayHead (AudioPlayHead* playhead) override
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
        const auto maxChans = std::max ((int) detail::totalChannels (_audioIns),
                                        (int) detail::totalChannels (_audioOuts));
        _tmpAudio.setSize (maxChans, maxBufferSize);
        _plugin->activate (_plugin, sampleRate, 16U, (uint32_t) maxBufferSize);
    }

    void releaseResources() override
    {
        if (_plugin == nullptr)
            return;
        _plugin->deactivate (_plugin);
        _tmpAudio.setSize (1, 1);
    }

    void render (RenderContext& rc) override
    {
        _proc.steady_time = -1;
        _proc.frames_count = (uint32_t) rc.audio.getNumSamples();
        _proc.transport = nullptr;

        _eventIn.clear();

        _queueIn.readAll ([this] (const clap_event_header_t* ev) {
            // std::cout << "[read] " << ((const clap_event_param_value_t*) ev)->value << std::endl;
            _eventIn.push (ev);
        });
        _eventIn.remap();
        _eventIn.dump();

        if (_notes != nullptr)
        {
            auto mb = rc.midi.getReadBuffer (0);
            for (auto i : *mb)
            {
                if (i.numBytes != 3)
                    continue;
                clap_event_midi_t ev;
                ev.header.flags = CLAP_EVENT_IS_LIVE;
                ev.header.size = sizeof (clap_event_midi_t);
                ev.header.space_id = 0;
                ev.header.time = static_cast<uint32_t> (i.samplePosition);
                ev.header.type = CLAP_EVENT_MIDI;
                ev.port_index = 0;
                std::memcpy (ev.data, i.data, 3);
                _eventIn.push ((const clap_event_header_t*) &ev);
            }
        }
        _eventIn.remap();
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

        _tmpAudio.setSize (rc.audio.getNumChannels(),
                           rc.audio.getNumSamples(),
                           true,
                           false,
                           true);

        rcc = 0;
        for (uint32_t i = 0; i < _proc.audio_outputs_count; ++i)
        {
            auto b = &_proc.audio_outputs[i];
            for (uint32_t j = 0; j < b->channel_count; ++j)
            {
                b->data32[j] = _tmpAudio.getWritePointer (rcc++);
            }
        }

        _plugin->start_processing (_plugin);
        _plugin->process (_plugin, &_proc);
        _plugin->stop_processing (_plugin);
        _eventOut.remap();

        while (--rcc >= 0)
        {
            rc.audio.copyFrom (rcc, 0, _tmpAudio, rcc, 0, rc.audio.getNumSamples());
        }
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
    Editor* createEditor() override { return nullptr; }

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
        return new CLAPParameter (_queueIn,
                                  _plugin,
                                  _params,
                                  &info,
                                  port.index,
                                  port.channel);
    }

    bool write (const clap_event_header_t* ev) noexcept
    {
        return _eventIn.push (ev);
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
    AudioBuffer<float> _tmpAudio;

    CLAPEventQueue<lvtk::RealtimeReadTrait> _queueIn;
    CLAPEventQueue<lvtk::RealtimeWriteTrait> _queueOut;

    CLAPProcessor (CLAPModule::Ptr m, const String& i)
        : Processor (0), ID (i), _module (m)
    {
        _host = detail::makeHost();
        _host.host_data = this;
        _host.get_extension = &getExtension;
        _host.request_callback = &requestCallback;
        _host.request_process = &requestProcess;
        _host.request_restart = &requestRestart;
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

        PortCount pc;
        if (auto audio = (const clap_plugin_audio_ports_t*) extension (CLAP_EXT_AUDIO_PORTS))
        {
            _audio = audio;

            initAudioBuffers (_audioIns, true);
            _proc.audio_inputs_count = _audio->count (_plugin, true);
            _proc.audio_inputs = _audioIns.data();
            pc.inputs[PortType::Audio] = detail::totalChannels (_audioIns);
            jassert (_proc.audio_inputs_count == _audioIns.size());

            initAudioBuffers (_audioOuts, false);
            _proc.audio_outputs_count = _audio->count (_plugin, false);
            _proc.audio_outputs = _audioOuts.data();
            pc.outputs[PortType::Audio] = detail::totalChannels (_audioOuts);
            jassert (_proc.audio_outputs_count == _audioOuts.size());
        }

        if (auto notes = (const clap_plugin_note_ports_t*) extension (CLAP_EXT_NOTE_PORTS))
        {
            _notes = notes;
            for (uint32_t i = 0; i < notes->count (_plugin, true); ++i)
            {
                clap_note_port_info_t info;
                notes->get (_plugin, i, true, &info);
                ++pc.inputs[PortType::Midi];
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

    void initAudioBuffers (std::vector<clap_audio_buffer_t>& bufs,
                           bool isInput)
    {
        auto numBuses = _audio->count (_plugin, isInput);
        for (uint32_t i = 0; i < numBuses; ++i)
        {
            uint32_t numChannels = 0;
            clap_audio_port_info_t info;
            _audio->get (_plugin, i, isInput, &info);

            const auto name = std::string (info.name);
            int ID = (int) info.id;
            bool inplace = info.in_place_pair != CLAP_INVALID_ID;
            juce::ignoreUnused (ID, inplace);

            if (info.port_type == nullptr)
            {
                numChannels = info.channel_count;
            }
            else if (std::strcmp (info.port_type, CLAP_PORT_MONO) == 0)
            {
                numChannels = 1;
            }
            else if (std::strcmp (info.port_type, CLAP_PORT_STEREO) == 0)
            {
                numChannels = 2;
            }
            else if (std::strcmp (info.port_type, CLAP_PORT_SURROUND) == 0)
            {
                jassertfalse;
                numChannels = info.channel_count;
            }
            else if (std::strcmp (info.port_type, CLAP_PORT_AMBISONIC) == 0)
            {
                jassertfalse;
                numChannels = info.channel_count;
            }
            else
            {
                jassertfalse;
                numChannels = info.channel_count;
            }

#if 0
            std::cout << "[clap] audio port: " << (int) i << ": " << name
                      << "  nc=" << (int) numChannels
                      << "  inplace=" << (int) inplace
                      << "  kind=" << std::string (info.port_type != nullptr ? info.port_type : "nullptr")
                      << std::endl;
#endif

            bufs.push_back ({});
            auto& buf = bufs.back();
            buf.channel_count = numChannels;
            buf.data32 = (float**) calloc (buf.channel_count, sizeof (float*));
            buf.data64 = nullptr;
            buf.constant_mask = 0;
            buf.latency = 0;
        }
    }

    //==========================================================================
    static const void* getExtension (const struct clap_host* host, const char* extensionID)
    {
        juce::ignoreUnused (host, extensionID);
        return nullptr;
    }

    static void requestRestart (const struct clap_host* host)
    {
        juce::ignoreUnused (host);
    }

    static void requestProcess (const struct clap_host* host)
    {
        juce::ignoreUnused (host);
    }

    static void requestCallback (const struct clap_host* host)
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
    sp.add (File::getSpecialLocation (File::userHomeDirectory).getChildFile ("Library/Audio/Plug-Ins/CLAP"));
    sp.add (File ("/Library/Audio/Plug-Ins/CLAP"));
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

#include <clap/helpers/host.hxx>
