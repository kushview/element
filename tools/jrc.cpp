/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 7 End-User License
   Agreement and JUCE Privacy Policy.

   End User License Agreement: www.juce.com/juce-7-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

/*
  ==============================================================================

   Utility to turn a bunch of binary files into a .cpp file and .h file full of
   data so they can be built directly into an executable.

   Use this code at your own risk! It carries no warranty!

  ==============================================================================
*/

#include <element/juce/config.h>
#include <juce_core/juce_core.h>
using namespace juce;

//==============================================================================
static int addFile (const File& file,
                    const String& classname,
                    OutputStream& headerStream,
                    OutputStream& cppStream)
{
    MemoryBlock mb;
    file.loadFileAsData (mb);

    const String name (file.getFileName()
                           .replaceCharacter (' ', '_')
                           .replaceCharacter ('.', '_')
                           .retainCharacters ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_0123456789"));

    // std::cout << "Adding " << name << ": "
    //           << (int) mb.getSize() << " bytes" << std::endl;

    headerStream << "    extern const char*  " << name << ";\r\n"
                    "    const int           " << name << "Size = "
                 << (int) mb.getSize() << ";\r\n\r\n";

    static int tempNum = 0;

    cppStream << "static const unsigned char temp" << ++tempNum << "[] = {";

    size_t i = 0;
    const uint8* const data = (const uint8*) mb.getData();

    while (i < mb.getSize() - 1)
    {
        if ((i % 40) != 39)
            cppStream << (int) data[i] << ",";
        else
            cppStream << (int) data[i] << ",\r\n  ";

        ++i;
    }

    cppStream << (int) data[i] << ",0,0};\r\n";

    cppStream << "const char* " << classname << "::" << name
              << " = (const char*) temp" << tempNum << ";\r\n\r\n";

    return (int) mb.getSize();
}

#if 0
static bool isHiddenFile (const File& f, const File& root)
{
    return f.getFileName().endsWithIgnoreCase (".scc")
         || f.getFileName() == ".svn"
         || f.getFileName().startsWithChar ('.')
         || (f.getSize() == 0 && ! f.isDirectory())
         || (f.getParentDirectory() != root && isHiddenFile (f.getParentDirectory(), root));
}
#endif

//==============================================================================
int main (int argc, char* argv[])
{
    if (argc < 3) {
        std::cout << 
" Usage: jrc  <input> <output> <namespace>\n\n"
" jrc will encode the <input> file as two files called (inputfile).cpp\n"
" and (<inputfile>).h in the target <output> directory.";

        return -1;
    }

    const File sourceFile (String (argv[1]).unquoted());

    if (! sourceFile.existsAsFile())
    {
        std::cout << sourceFile.getFullPathName() << " does not exist." << std::endl;
        return -2;
    }

    const File destDirectory (File::getCurrentWorkingDirectory()
                                   .getChildFile (String (argv[2]).unquoted()));

    if (! destDirectory.isDirectory())
    {
        std::cout << destDirectory.getFullPathName() << " does not exist" << std::endl;
        return -3;
    }

    String className (argv[3]);
    className = className.trim();
    String baseName (sourceFile.getFileName());
    baseName = baseName.trim();

    const File headerFile (destDirectory.getChildFile (baseName + ".h"));
    const File cppFile    (destDirectory.getChildFile (baseName + ".cpp"));

    // std::cout << "Creating " << headerFile.getFullPathName()
    //           << " and " << cppFile.getFullPathName()
    //           << " from files in " << sourceFile.getFullPathName()
    //           << "..." << std::endl << std::endl;

    headerFile.deleteFile();
    cppFile.deleteFile();

    std::unique_ptr<OutputStream> header (headerFile.createOutputStream());

    if (header == nullptr)
    {
        std::cout << "Couldn't open "
                  << headerFile.getFullPathName() << " for writing" << std::endl << std::endl;
        return -4;
    }

    std::unique_ptr<OutputStream> cpp (cppFile.createOutputStream());

    if (cpp == nullptr)
    {
        std::cout << "Couldn't open "
                  << cppFile.getFullPathName() << " for writing" << std::endl << std::endl;
        return -5;
    }

    *header << "/* (Auto-generated binary data file). */\r\n\r\n"
               "#pragma once\r\n\r\n"
               "namespace " << className << "\r\n"
               "{\r\n";

    *cpp << "/* (Auto-generated binary data file). */\r\n\r\n"
            "#include \"" << baseName << ".h\"\r\n\r\n";

    int totalBytes = 0;

    Array<File> files; files.add (sourceFile);
    for (int i = 0; i < files.size(); ++i)
    {
        const File file (files[i]);

        // (avoid source control files and hidden files..)
        // if (! isHiddenFile (file, sourceFile))
        {
#if 0
            if (file.getParentDirectory() != sourceFile)
            {
                *header << "  #ifdef " << file.getParentDirectory().getFileName().toUpperCase() << "\r\n";
                *cpp << "#ifdef " << file.getParentDirectory().getFileName().toUpperCase() << "\r\n";
DBG("adding file");
                totalBytes += addFile (file, className, *header, *cpp);
DBG("file added");
                *header << "  #endif\r\n";
                *cpp << "#endif\r\n";
            }
            else
#endif
            {
                totalBytes += addFile (file, className, *header, *cpp);
            }
        }
    }

    *header << "}\r\n";

    header = nullptr;
    cpp = nullptr;
    return 0;
}
