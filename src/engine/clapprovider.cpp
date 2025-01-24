#include <element/processor.hpp>

#include "clapprovider.hpp"

#if ! JUCE_WINDOWS
static void _fpreset() {}
static void _clearfp() {}
#endif

//==============================================================================
// Change this to disable logging of various VST activities
#ifndef VST_LOGGING
#define VST_LOGGING 1
#endif

#if VST_LOGGING
#define CLAP_LOG(a) Logger::writeToLog (a);
#else
#define CLAP_LOG(a)
#endif

//==============================================================================
namespace {
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

static int shellUIDToCreate = 0;
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

typedef void (*MainCall)();

namespace element {

//==============================================================================
struct ModuleHandle final : public ReferenceCountedObject
{
    File file;
    MainCall moduleMain, customMain = {};
    String pluginName;
    std::unique_ptr<XmlElement> vstXml;

    using Ptr = ReferenceCountedObjectPtr<ModuleHandle>;

    static Array<ModuleHandle*>& getActiveModules()
    {
        static Array<ModuleHandle*> activeModules;
        return activeModules;
    }

    //==============================================================================
    static Ptr findOrCreateModule (const File& file)
    {
        for (auto* module : getActiveModules())
            if (module->file == file)
                return module;

        const IdleCallRecursionPreventer icrp;
        shellUIDToCreate = 0;
        _fpreset();

        CLAP_LOG ("Attempting to load CLAP: " + file.getFullPathName());

        Ptr m = new ModuleHandle (file, nullptr);

        if (m->open())
        {
            _fpreset();
            return m;
        }

        return {};
    }

    //==============================================================================
    ModuleHandle (const File& f, MainCall customMainCall)
        : file (f), moduleMain (customMainCall)
    {
        getActiveModules().add (this);

#if JUCE_WINDOWS || JUCE_LINUX || JUCE_BSD || JUCE_IOS || JUCE_ANDROID
        fullParentDirectoryPathName = f.getParentDirectory().getFullPathName();
#elif JUCE_MAC
        FSRef ref;
        makeFSRefFromPath (&ref, f.getParentDirectory().getFullPathName());
        FSGetCatalogInfo (&ref, kFSCatInfoNone, nullptr, nullptr, &parentDirFSSpec, nullptr);
#endif
    }

    ~ModuleHandle()
    {
        getActiveModules().removeFirstMatchingValue (this);
        close();
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

        moduleMain = (MainCall) module.getFunction ("VSTPluginMain");

        if (moduleMain == nullptr)
            moduleMain = (MainCall) module.getFunction ("main");

        if (moduleMain != nullptr)
        {
            vstXml = parseXML (file.withFileExtension ("vstxml"));

#if JUCE_WINDOWS
            if (vstXml == nullptr)
                vstXml = parseXML (getDLLResource (file, "VSTXML", 1));
#endif
        }

        return moduleMain != nullptr;
    }

    void close()
    {
        _fpreset(); // (doesn't do any harm)

        module.close();
    }

#if JUCE_WINDOWS
    static String getDLLResource (const File& dllFile, const String& type, int resID)
    {
        DynamicLibrary dll (dllFile.getFullPathName());
        auto dllModule = (HMODULE) dll.getNativeHandle();

        if (dllModule != INVALID_HANDLE_VALUE)
        {
            if (auto res = FindResource (dllModule, MAKEINTRESOURCE (resID), type.toWideCharPointer()))
            {
                if (auto hGlob = LoadResource (dllModule, res))
                {
                    auto* data = static_cast<const char*> (LockResource (hGlob));
                    return String::fromUTF8 (data, (int) SizeofResource (dllModule, res));
                }
            }
        }

        return {};
    }
#endif
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
                        moduleMain = (MainCall) CFBundleGetFunctionPointerForName (bundleRef.get(), CFSTR ("main_macho"));

                        if (moduleMain == nullptr)
                            moduleMain = (MainCall) CFBundleGetFunctionPointerForName (bundleRef.get(), CFSTR ("VSTPluginMain"));

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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModuleHandle)
};

//==============================================================================
class CLAPProvider::Impl final
{
};

CLAPProvider::CLAPProvider()
{
    _impl.reset (new Impl());
}

CLAPProvider::~CLAPProvider()
{
    _impl.reset();
}

juce::String CLAPProvider::format() const { return "CLAP"; }

Processor* CLAPProvider::create (const juce::String&)
{
    return nullptr;
}

//==============================================================================
static bool maybePlugin (const File& f)
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

        if (maybePlugin (f))
        {
            isPlugin = true;
            res.add (f.getFullPathName());
        }

        if (recursive && (! isPlugin) && f.isDirectory())
            recursiveSearch (f, res, true);
    }
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
    sp.add (File ("~/.clap"));
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
        recursiveSearch (path[i], r, recursive);
    return r;
}

StringArray getHiddenTypes() { return {}; }

} // namespace element