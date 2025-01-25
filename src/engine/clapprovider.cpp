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
        _plugin->activate (_plugin, sampleRate, 16U, (uint32_t) maxBufferSize);
    }

    void releaseResources() override
    {
        if (_plugin == nullptr)
            return;
        _plugin->deactivate (_plugin);
    }

    void render (RenderContext&) override
    {
#if 0
        clap_process_t _proc;
        _plugin->process (_plugin, &_proc);
#endif
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
            stream.ctx = (void*) &data;
            stream.read = &readState;
            state->load (_plugin, &stream);
        }
    }

    //==========================================================================
    bool hasEditor() override { return false; }
    Editor* createEditor() { return nullptr; }

private:
    virtual void initialize() {}
    virtual ParameterPtr getParameter (const PortDescription& port) { return nullptr; }

private:
    const String ID;
    CLAPModule::Ptr _module { nullptr };
    clap_host_t _host;
    const clap_plugin_t* _plugin { nullptr };

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

        if (auto audio = (const clap_plugin_audio_ports_t*) extension (CLAP_EXT_AUDIO_PORTS))
        {
            juce::ignoreUnused (audio);
        }

        if (auto notes = (const clap_plugin_note_ports_t*) extension (CLAP_EXT_NOTE_PORTS))
        {
            juce::ignoreUnused (notes);
        }

        if (auto params = (const clap_plugin_params_t*) extension (CLAP_EXT_PARAMS))
        {
            juce::ignoreUnused (params);
        }

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
