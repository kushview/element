/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   digitalelementslogo_220_png;
    const int            digitalelementslogo_220_pngSize = 7467;

    extern const char*   kushviewlogotext_220_png;
    const int            kushviewlogotext_220_pngSize = 6588;

    extern const char*   RobotoRegular_ttf;
    const int            RobotoRegular_ttfSize = 168260;

    extern const char*   PowerButton_48x48_png;
    const int            PowerButton_48x48_pngSize = 1153;

    extern const char*   Classic_elw;
    const int            Classic_elwSize = 1631;

    extern const char*   Editing_elw;
    const int            Editing_elwSize = 1820;

    extern const char*   acknowledgements_txt;
    const int            acknowledgements_txtSize = 3461;

    extern const char*   developers_txt;
    const int            developers_txtSize = 195;

    extern const char*   ElementIcon_png;
    const int            ElementIcon_pngSize = 11697;

    extern const char*   ElementIconTemplate_png;
    const int            ElementIconTemplate_pngSize = 7122;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 10;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}
