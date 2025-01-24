#include <element/processor.hpp>

#include "clapprovider.hpp"

namespace element {

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

StringArray CLAPProvider::findTypes()
{
    StringArray r;
    FileSearchPath path;
    for (int i = 0; i < path.getNumPaths(); ++i)
        recursiveSearch (path[i], r, true);
    return r;
}

StringArray getHiddenTypes() { return {}; }

} // namespace element