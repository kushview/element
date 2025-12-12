
#if __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#endif

#include <clap/clap.h>
#include <clap/helpers/host.hh>
#include <clap/helpers/event-list.hh>
#include <clap/helpers/plugin-proxy.hh>
#include <clap/helpers/reducing-param-queue.hh>

#include <element/juce/gui_extra.hpp>
#include <element/processor.hpp>
#include <element/version.hpp>
#include <element/ui/nodeeditor.hpp>

#include "lv2/messages.hpp"

#include "appinfo.hpp"
#include "engine/clapprovider.hpp"
#include "ui/resizelistener.hpp"
#include "ui/nsviewwithparent.hpp"

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wdeprecated-declarations")

#if __APPLE__
#define EL_WINDOW_API CLAP_WINDOW_API_COCOA
#elif defined(__WIN32__)
#define EL_WINDOW_API CLAP_WINDOW_API_WIN32
#else
#define EL_WINDOW_API CLAP_WINDOW_API_X11
#endif

#if ! JUCE_WINDOWS
static void _fpreset()
{
}
// static void _clearfp() {}
#endif

//==============================================================================
// Change this to disable logging of various VST activities
#ifndef CLAP_LOGGING
#define CLAP_LOGGING JUCE_DEBUG
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
    return f.hasFileExtension ("clap") && ! f.isDirectory() && f.existsAsFile();
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

struct MainToAudioParameterValue
{
    void* cookie;
    double value;
};

struct AudioToMainParameterValue
{
    void update (const AudioToMainParameterValue& v) noexcept
    {
        if (v.hasValue)
        {
            hasValue = true;
            value = v.value;
        }

        if (v.hasGesture)
        {
            hasGesture = true;
            isBegin = v.isBegin;
        }
    }

    bool hasValue = false;
    bool hasGesture = false;
    bool isBegin = false;
    double value = 0;
};

using MainToAudioQueue = clap::helpers::ReducingParamQueue<clap_id, MainToAudioParameterValue>;
using AudioToMainQueue = clap::helpers::ReducingParamQueue<clap_id, AudioToMainParameterValue>;

//==============================================================================
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
enum class ThreadType
{
    Unknown,
    MainThread,
    AudioThread,
    AudioThreadPool,
};

static thread_local ThreadType gThreadType = ThreadType::Unknown;

using CLAPBaseHost = clap::helpers::Host<Misbehaviour, Level>;
// extern template class clap::helpers::Host<Misbehaviour, Level>;
class CLAPHost final : public CLAPBaseHost
{
public:
    CLAPHost()
        : CLAPBaseHost (EL_APP_NAME, EL_APP_AUTHOR, EL_APP_URL, ELEMENT_VERSION_STRING)
    {
    }
    ~CLAPHost() {}

    // clap_host_thread_check
    bool threadCheckIsMainThread() const noexcept override
    {
        return gThreadType == ThreadType::MainThread;
    }

    bool threadCheckIsAudioThread() const noexcept override
    {
        return gThreadType == ThreadType::AudioThread;
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

    bool enableDraftExtensions() const noexcept override { return false; }
    const void* getExtension (const char* extensionId) const noexcept
    {
        juce::ignoreUnused (extensionId);
        return nullptr;
    }

#if 0
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
#endif

    // clap_host_log
    bool implementsLog() const noexcept override { return true; }
    void logLog (clap_log_severity severity, const char* message) const noexcept override
    {
        switch (severity)
        {
            case CLAP_LOG_DEBUG:
                std::cout << String::fromUTF8 (message) << std::endl;
                break;
            case CLAP_LOG_INFO:
                Logger::writeToLog (String::fromUTF8 (message));
                break;
            case CLAP_LOG_WARNING:
                Logger::writeToLog (String::fromUTF8 (message));
                break;
            case CLAP_LOG_ERROR:
                Logger::writeToLog (String::fromUTF8 (message));
                break;
            case CLAP_LOG_FATAL:
                Logger::writeToLog (String::fromUTF8 (message));
                break;

                // These severities should be used to report misbehaviour.
                // The plugin one can be used by a layer between the plugin and the host.
            case CLAP_LOG_HOST_MISBEHAVING:
                Logger::writeToLog (String::fromUTF8 (message));
                break;
            case CLAP_LOG_PLUGIN_MISBEHAVING:
                Logger::writeToLog (String::fromUTF8 (message));
                break;
        }
    }
#if 0
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
#endif

    struct CLAPTimer : public juce::Timer
    {
        clap_id ID;
        uint32_t periodMs { 0 };
        const clap_plugin_t* plugin { nullptr };
        const clap_plugin_timer_support_t* timer { nullptr };

        void start()
        {
            startTimer (static_cast<int> (periodMs));
        }

        void timerCallback() override
        {
            if (plugin == nullptr || timer == nullptr || ID == CLAP_INVALID_ID)
            {
                stopTimer();
                return;
            }

            timer->on_timer (plugin, ID);
        }
    };

    std::unordered_map<clap_id, std::unique_ptr<CLAPTimer>> _timers;

    // clap_host_timer_support
    bool implementsTimerSupport() const noexcept override
    {
        return true;
    }

    friend class CLAPProcessor;
    using PluginProxy = clap::helpers::PluginProxy<Misbehaviour, Level>;
    using ProxyPtr = std::unique_ptr<PluginProxy>;
    ProxyPtr _proxy;
    const clap_plugin_t* _plugin { nullptr };
    const clap_plugin_timer_support_t* _timer { nullptr };

    void setPlugin (const clap_plugin_t* plugin)
    {
        if (plugin != nullptr)
        {
            _proxy = std::make_unique<PluginProxy> (*plugin, *this);
            _plugin = _proxy->clapPlugin();
        }
        else
        {
            _plugin = nullptr;
            _proxy.reset();
        }
    }

    const clap_plugin_t* clapPlugin() const noexcept { return _plugin; }
    void setPluginTimer (const clap_plugin_timer_support_t* timers) { _timer = timers; }

    bool timerSupportRegisterTimer (uint32_t periodMs, clap_id* timerId) noexcept override
    {
        if (_plugin == nullptr || _timer == nullptr)
            return false;
        static uint32_t nextId = 0;
        auto ptr = std::make_unique<CLAPTimer>();
        ptr->ID = ++nextId;
        ptr->plugin = _plugin;
        ptr->timer = _timer;
        *timerId = ptr->ID;
        ptr->start();
#if 0
        juce::String msg = "[clap] tiimer started: ";
        msg << (int) periodMs << "ms";
        CLAP_LOG (msg);
#endif
        _timers[ptr->ID] = std::move (ptr);
        return true;
    }

    bool timerSupportUnregisterTimer (clap_id timerId) noexcept override
    {
        auto i = _timers.find (timerId);
        if (i != _timers.end())
        {
            i->second->stopTimer();
            i->second.reset();
            _timers.erase (i);
            return true;
        }
        return false;
    }

#if 0
      // clap_host_tail
      virtual bool implementsTail() const noexcept { return false; }
      virtual void tailChanged() noexcept {}

      // clap_host_thread_pool
      virtual bool implementsThreadPool() const noexcept { return false; }
      virtual bool threadPoolRequestExec(uint32_t numTasks) noexcept { return false; }
#endif

private:
    clap::helpers::EventList _evIn, _evOut;

    MainToAudioQueue _appToEngineValueQueue;
    MainToAudioQueue _appToEngineModQueue;
    AudioToMainQueue _engineToAppValueQueue;
    std::unordered_map<clap_id, bool> _isAdjustingParameter;

    struct Settings
    {
        constexpr bool shouldProvideCookie() const noexcept { return true; }
    } _settings;

    void generatePluginInputEvents()
    {
        _appToEngineValueQueue.consume (
            [this] (clap_id param_id, const MainToAudioParameterValue& value) {
                clap_event_param_value ev;
                ev.header.time = 0;
                ev.header.type = CLAP_EVENT_PARAM_VALUE;
                ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
                ev.header.flags = 0;
                ev.header.size = sizeof (ev);
                ev.param_id = param_id;
                ev.cookie = _settings.shouldProvideCookie() ? value.cookie : nullptr;
                ev.port_index = 0;
                ev.key = -1;
                ev.channel = -1;
                ev.note_id = -1;
                ev.value = value.value;
                _evIn.push (&ev.header);
            });

        _appToEngineModQueue.consume ([this] (clap_id param_id, const MainToAudioParameterValue& value) {
            clap_event_param_mod ev;
            ev.header.time = 0;
            ev.header.type = CLAP_EVENT_PARAM_MOD;
            ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            ev.header.flags = 0;
            ev.header.size = sizeof (ev);
            ev.param_id = param_id;
            ev.cookie = _settings.shouldProvideCookie() ? value.cookie : nullptr;
            ev.port_index = 0;
            ev.key = -1;
            ev.channel = -1;
            ev.note_id = -1;
            ev.amount = value.value;
            _evIn.push (&ev.header);
        });
    }

    void handlePluginOutputEvents()
    {
        for (uint32_t i = 0; i < _evOut.size(); ++i)
        {
            auto h = _evOut.get (i);
            switch (h->type)
            {
                case CLAP_EVENT_PARAM_GESTURE_BEGIN: {
                    auto ev = reinterpret_cast<const clap_event_param_gesture_t*> (h);
                    bool& isAdj = _isAdjustingParameter[ev->param_id];

                    if (isAdj)
                        throw std::logic_error ("The plugin sent BEGIN_ADJUST twice");
                    isAdj = true;

                    AudioToMainParameterValue v;
                    v.hasGesture = true;
                    v.isBegin = true;
                    _engineToAppValueQueue.setOrUpdate (ev->param_id, v);
                    break;
                }

                case CLAP_EVENT_PARAM_GESTURE_END: {
                    auto ev = reinterpret_cast<const clap_event_param_gesture*> (h);
                    bool& isAdj = _isAdjustingParameter[ev->param_id];

                    if (! isAdj)
                        throw std::logic_error ("The plugin sent END_ADJUST without a preceding BEGIN_ADJUST");
                    isAdj = false;
                    AudioToMainParameterValue v;
                    v.hasGesture = true;
                    v.isBegin = false;
                    _engineToAppValueQueue.setOrUpdate (ev->param_id, v);
                    break;
                }

                case CLAP_EVENT_PARAM_VALUE: {
                    auto ev = reinterpret_cast<const clap_event_param_value*> (h);
                    AudioToMainParameterValue v;
                    v.hasValue = true;
                    v.value = ev->value;
                    _engineToAppValueQueue.setOrUpdate (ev->param_id, v);
                    break;
                }

                case CLAP_EVENT_NOTE_END:
                case CLAP_EVENT_NOTE_OFF:
                    std::cout << "CLAP_EVENT_NOTE_END\n";
                    break;
            }
        }
    }
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

    const clap_plugin_t* create (const clap_host_t* host, std::string_view pluginID)
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
            initLibrary();

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
                        moduleMain = (CLAPEntry) CFBundleGetFunctionPointerForName (bundleRef.get(), CFSTR ("clap_entry"));

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

        if (ok)
            initLibrary();

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
    [[maybe_unused]] bool _initialized { false };
    const clap_plugin_factory* _factory { nullptr };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CLAPModule)

    void initLibrary()
    {
        _initialized = moduleMain->init (file.getFullPathName().toRawUTF8());
        _factory = (clap_plugin_factory*) (_initialized
                                               ? moduleMain->get_factory (CLAP_PLUGIN_FACTORY_ID)
                                               : nullptr);
    }
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

    void syncAndNotify()
    {
        double value = 0.0;
        if (! _params->get_value (_plugin, _info.id, &value))
            return;

        value = _range.convertFrom0to1 (value);
        if (value != _value.load())
        {
            _value.store (static_cast<float> (value));
            sendValueChangedMessageToListeners (value);
        }
    }

    //==========================================================================
    int getPortIndex() const noexcept override { return _portIndex; }
    int getParameterIndex() const noexcept override { return _paramIndex; }
    float getValue() const override { return _value.load(); }

    void setValue (float newValue) override
    {
        _value.store (newValue);

        clap_event_param_value_t ev;
        ev.header.type = CLAP_EVENT_PARAM_VALUE;
        ev.header.flags = 0; //CLAP_EVENT_IS_LIVE;
        ev.header.size = sizeof (ev);
        ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
        ev.header.time = 0;
        ev.port_index = 0;
        ev.cookie = _info.cookie;
        ev.param_id = _info.id;
        ev.value = _range.convertFrom0to1 (newValue);

        ev.key = -1;
        ev.note_id = -1;
        ev.channel = -1;

        _queue.push (&ev.header);

        // std::cout << "[param] send: " << ev.value << std::endl;
    }

    float getDefaultValue() const override { return static_cast<float> (_info.default_value); }

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
class CLAPEditor : public Editor,
                   public PhysicalResizeListener,
                   private Timer
{
    bool _created = false;

public:
    CLAPEditor (const clap_plugin_t* plugin, const clap_plugin_gui_t* gui)
        : Editor(),
          _plugin (plugin),
          _gui (gui),
          _timer ((const clap_plugin_timer_support_t*) plugin->get_extension (plugin, CLAP_EXT_TIMER_SUPPORT))
    {
        setOpaque (true);
        _view = std::make_unique<ViewComponent> (*this);
        addAndMakeVisible (_view.get());

        _created = _gui->create (_plugin, EL_WINDOW_API, false);

        if (_created)
        {
            uint32_t w = 0, h = 0;
            if (_gui->get_size (_plugin, &w, &h))
                setSize ((int) w, (int) h);
            else
                setSize (1, 1);

            setResizable (false);

            auto window = _view->hostWindow();
            _gui->set_parent (_plugin, &window);

            setVisible (false);
            setVisible (true);
        }

#if JUCE_LINUX
        if (_timer != nullptr)
            startTimerHz (60);
#endif
    }

    ~CLAPEditor()
    {
        _view->prepareForDestruction();
        _view.reset();

        if (_created)
        {
            _gui->hide (_plugin);
            _gui->destroy (_plugin);
        }
    }

    void viewRequestedResizeInPhysicalPixels (int width, int height) override
    {
        _view->setSize (width, height);
        _view->forceViewToSize();
        resized();
    }

    void paint (Graphics& g) override
    {
        g.fillAll (Colours::black);
    }

    void resized() override
    {
        if (_view != nullptr)
            _view->setBounds (getLocalBounds());
    }

    void visibilityChanged() override
    {
        if (isVisible())
            _gui->show (_plugin);
        else
            _gui->hide (_plugin);
    }

private:
    const clap_plugin_t* _plugin { nullptr };
    const clap_plugin_gui_t* _gui { nullptr };
    const clap_plugin_timer_support_t* _timer { nullptr };

    void timerCallback() override
    {
#if JUCE_LINUX
        _timer->on_timer (_plugin, 0);
#else
        stopTimer();
#endif
    }

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

        clap_window_t hostWindow()
        {
            clap_window_t win;
            win.x11 = inner.getHostWindowID();
            return win;
        }

        void forceViewToSize()
        {
            inner.setSize (getWidth(), getHeight());
        }

        void fitToView()
        {
        }

        void paint (Graphics& g) override
        {
            g.fillAll (Colours::black);
        }

        //==============================================================================
        void componentMovedOrResized (bool wasMoved, bool wasResized) override
        {
            if (wasResized)
            {
                if (auto pc = findParentComponentOfClass<CLAPEditor>())
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
        clap_window_t hostWindow()
        {
            clap_window_t w;
            w.cocoa = (clap_nsview) getView();
            return w;
        }
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

        clap_window_t hostWindow()
        {
            clap_window_t w;
            w.win32 = getHWND();
            return w;
        }

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

    std::unique_ptr<ViewComponent> _view;
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
        gThreadType = ThreadType::MainThread;
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
        _plugin->activate (_plugin, sampleRate, 1U, (uint32_t) maxBufferSize);
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
        gThreadType = ThreadType::AudioThread;

        _proc.steady_time = -1;

        _proc.frames_count = (uint32_t) rc.audio.getNumSamples();
        _proc.transport = nullptr;
        clap_event_transport_t _transport = {};

        if (auto pos = getPlayHead()->getPosition())
        {
            _transport.header.flags = 0;
            _transport.header.size = sizeof (clap_event_transport_t);
            _transport.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            _transport.header.time = 0;
            _transport.header.type = CLAP_EVENT_TRANSPORT;

            _transport.flags = 0;
            if (pos->getIsPlaying())
                _transport.flags |= CLAP_TRANSPORT_IS_PLAYING;
            if (pos->getIsLooping())
                _transport.flags |= CLAP_TRANSPORT_IS_LOOP_ACTIVE;
            if (pos->getIsRecording())
                _transport.flags |= CLAP_TRANSPORT_IS_RECORDING;
            _transport.flags |= CLAP_TRANSPORT_HAS_TEMPO | CLAP_TRANSPORT_HAS_TIME_SIGNATURE;

            _transport.song_pos_beats = juce::roundToInt (*pos->getPpqPosition() * CLAP_BEATTIME_FACTOR);

            // position in seconds
            _transport.song_pos_seconds = juce::roundToInt (
                CLAP_SECTIME_FACTOR * *pos->getTimeInSeconds());

            // in bpm
            _transport.tempo = *pos->getBpm();
            _transport.tempo_inc = 0; // tempo increment for each sample and until the next
            // time info event

            // Looping
            _transport.loop_start_beats = 0;
            _transport.loop_end_beats = 0;
            _transport.loop_start_seconds = 0;
            _transport.loop_end_seconds = 0;

            // start pos of the current bar
            _transport.bar_start = juce::roundToInt (
                CLAP_BEATTIME_FACTOR * *pos->getPpqPositionOfLastBarStart());
            // bar at song pos 0 has the number 0
            _transport.bar_number = static_cast<int32_t> (*pos->getBarCount());

            auto sig = pos->getTimeSignature();
            _transport.tsig_num = static_cast<uint16_t> (sig->numerator); // time signature numerator
            _transport.tsig_denom = static_cast<uint16_t> (sig->denominator); // time signature denominator

            _proc.transport = &_transport;
        }

        _proc.in_events = _eventIn.clapInputEvents();
        _proc.out_events = _eventOut.clapOutputEvents();

        _eventOut.clear();
        _eventIn.clear();

        _queueIn.readAll ([this] (const clap_event_header_t* ev) {
            _eventIn.push (ev);
        });

        if (_notes != nullptr)
        {
            auto mb = rc.midi.getReadBuffer (0);
            for (auto i : *mb)
            {
                auto msg = i.getMessage();
                if (msg.isNoteOnOrOff())
                {
                    clap_event_note_t ne;
                    ne.header.flags = CLAP_EVENT_IS_LIVE;
                    ne.header.size = sizeof (clap_event_note_t);
                    ne.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
                    ne.header.time = static_cast<uint32_t> (i.samplePosition);
                    ne.header.type = msg.isNoteOn() ? CLAP_EVENT_NOTE_ON : CLAP_EVENT_NOTE_OFF;
                    ne.channel = msg.getChannel() - 1;
                    ne.key = msg.getNoteNumber();
                    ne.note_id = -1;
                    ne.port_index = 0;
                    ne.velocity = msg.isNoteOn() ? msg.getFloatVelocity() : 1.f;
                    _eventIn.push (&ne.header);
                }
                else if (i.numBytes >= 0 && i.numBytes <= 3)
                {
                    clap_event_midi_t ev;
                    ev.header.flags = CLAP_EVENT_IS_LIVE;
                    ev.header.size = sizeof (clap_event_midi_t);
                    ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
                    ev.header.time = static_cast<uint32_t> (i.samplePosition);
                    ev.header.type = CLAP_EVENT_MIDI;
                    ev.port_index = 0;
                    std::memcpy (ev.data, i.data, static_cast<size_t> (i.numBytes));
                    _eventIn.push (&ev.header);
                }
            }
        }

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
                           false,
                           false,
                           true);
        _tmpAudio.clear (0, rc.audio.getNumSamples());
        rcc = 0;
        for (uint32_t i = 0; i < _proc.audio_outputs_count; ++i)
        {
            auto b = &_proc.audio_outputs[i];
            for (uint32_t j = 0; j < b->channel_count; ++j)
            {
                b->data32[j] = _tmpAudio.getWritePointer (rcc++);
            }
        }

        jassert (_tmpAudio.getNumChannels() >= rcc);

        _plugin->start_processing (_plugin);
        _plugin->process (_plugin, &_proc);
        _plugin->stop_processing (_plugin);

        // Process any output events from the plugin
        for (uint32_t i = 0; i < _eventOut.size(); ++i)
        {
            auto h = _eventOut.get (i);
            // For now, just log note events to help with debugging
            switch (h->type)
            {
                case CLAP_EVENT_NOTE_END:
                case CLAP_EVENT_NOTE_OFF:
                    CLAP_LOG ("Plugin generated NOTE_OFF/END event");
                    break;
                case CLAP_EVENT_NOTE_ON:
                    CLAP_LOG ("Plugin generated NOTE_ON event");
                    break;
                default:
                    break;
            }
        }

        while (--rcc >= 0)
        {
            rc.audio.copyFrom (rcc, 0, _tmpAudio, rcc, 0, rc.audio.getNumSamples());
        }

        gThreadType = ThreadType::AudioThread;
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

    void syncParams()
    {
        for (auto param : getParameters (true))
            if (auto cp = dynamic_cast<CLAPParameter*> (param))
                cp->syncAndNotify();
    }

    void setState (const void* data, int size) override
    {
        if (auto state = (const clap_plugin_state_t*) _plugin->get_extension (_plugin, CLAP_EXT_STATE))
        {
            MemoryInputStream mi (data, (size_t) size, false);
            clap_istream_t stream;
            stream.ctx = (void*) &mi;
            stream.read = &readState;
            if (state->load (_plugin, &stream))
                syncParams();
        }
    }

    //==========================================================================
    bool hasEditor() override { return _gui != nullptr; }
    Editor* createEditor() override
    {
        return hasEditor() ? new CLAPEditor (_plugin, _gui) : nullptr;
    }

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
        _eventIn.push (ev);
        return true;
    }

private:
    const String ID;
    CLAPModule::Ptr _module { nullptr };

    CLAPHost _host;
    const clap_plugin_t* _plugin { nullptr };
    const clap_plugin_audio_ports* _audio { nullptr };
    const clap_plugin_note_ports_t* _notes { nullptr };
    const clap_plugin_params_t* _params { nullptr };
    const clap_plugin_gui_t* _gui { nullptr };

    clap_process_t _proc;
    std::vector<clap_audio_buffer_t> _audioIns, _audioOuts;
    AudioBuffer<float> _tmpAudio;

    clap::helpers::EventList _eventIn, _eventOut;
    CLAPEventQueue<lvtk::RealtimeReadTrait> _queueIn;
    CLAPEventQueue<lvtk::RealtimeWriteTrait> _queueOut;

    CLAPProcessor (CLAPModule::Ptr m, const String& i)
        : Processor (0), ID (i), _module (m)
    {
        if (auto plugin = (m != nullptr ? m->create (_host.clapHost(), ID.toRawUTF8()) : nullptr))
        {
            _host.setPlugin (plugin);
            _plugin = _host.clapPlugin();
        }
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

        if (auto gui = (const clap_plugin_gui_t*) extension (CLAP_EXT_GUI))
        {
            if (gui->is_api_supported (_plugin, EL_WINDOW_API, false))
                _gui = gui;
        }

        if (auto timer = (const clap_plugin_timer_support_t*) extension (CLAP_EXT_TIMER_SUPPORT))
        {
            _host.setPluginTimer (timer);
        }

        if (auto fd = (const clap_plugin_posix_fd_support_t*) extension (CLAP_EXT_POSIX_FD_SUPPORT))
        {
            juce::ignoreUnused (fd);
            CLAP_LOG ("plugin wants FD support.");
        }
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
        juce::ignoreUnused (owner);
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
    auto programFiles = File::getSpecialLocation (File::globalApplicationsDirectory);
    sp.add (programFiles.getChildFile ("Common Files/CLAP"));
    sp.add (File::getSpecialLocation (File::userHomeDirectory).getChildFile ("AppData/Local/Programs/Common/CLAP"));
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
    CLAP_LOG ("got the module");
    if (mod == nullptr)
    {
        CLAP_LOG ("is was null");
        return;
    }
    for (uint32_t i = 0; i < mod->size(); ++i)
    {
        const auto d = mod->descriptor (i);
        auto pd = out.add (new PluginDescription());
        *pd = detail::makeDescription (d);
        CLAP_LOG (pd->name);
        if (isFile)
            pd->fileOrIdentifier << ":" << fileOrID.trim();
    }
}

} // namespace element

#include <clap/helpers/host.hxx>
#include <clap/helpers/reducing-param-queue.hxx>
#include <clap/helpers/plugin-proxy.hxx>
