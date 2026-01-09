# Integrating Sparkle and WinSparkle with Element

## Overview
Element is a modular audio plugin host built with JUCE (C++). The project already has:
- A custom `Updater` class (`element::Updater`) that checks for updates from a custom XML repository
- Update-related settings infrastructure (check for updates, update channels, update keys)
- A `GuiService::UpdateManager` that manages update checking on the UI side
- Platform-specific build configuration via CMake

To integrate Sparkle (macOS) and WinSparkle (Windows), we would enhance this existing infrastructure rather than replace it.

---

## Current Update Infrastructure

### Existing Components

1. **Updater Class** ([include/element/ui/updater.hpp](include/element/ui/updater.hpp), [src/ui/updater.cpp](src/ui/updater.cpp))
   - Fetches updates from a configurable XML repository
   - Parses `latest.xml` format from Kushview's repo
   - Uses boost signals for async notifications
   - Platform-aware repository URLs: `/osx`, `/windows`, `/linux`

2. **Update Settings** ([src/settings.cpp](src/settings.cpp) lines 14, 39-42)
   - `checkForUpdatesKey`: Enable/disable update checks
   - `updateChannelKey`: Select update channel (stable, beta, etc.)
   - `updateKeyTypeKey`: Update key type/slug
   - `updateKeyKey`: The actual update key
   - `updateKeyUserKey`: User identifier for updates

3. **GuiService::UpdateManager** ([src/services/guiservice.cpp](src/services/guiservice.cpp) lines 162-210)
   - Initializes updater with version and repository
   - Shows alert dialogs when updates available
   - Connects to update signals

### Repository Structure
```
ELEMENT_UPDATES_HOST = "https://repo.kushview.net"
ELEMENT_UPDATES_PATH = "/element/1/stable"
Platform-specific paths: /osx, /windows, /linux
Expected endpoint: https://repo.kushview.net/element/1/stable/{platform}/latest.xml
```

---

## Integration Strategy

### Option 1: Use Sparkle/WinSparkle Directly (Recommended)

This approach replaces the custom update checking with native frameworks while keeping the existing UI integration.

#### Advantages:
- Professional, battle-tested auto-update frameworks
- Automatic delta updates
- Better security (code signing verification)
- Native progress UI (can be customized)
- Handles launch-on-exit for updates seamlessly

#### Disadvantages:
- Adds external native dependencies
- Sparkle/WinSparkle have specific requirements for app signing
- Requires changes to build system and installation procedures

### Option 2: Hybrid Approach (Medium Effort)

Keep custom updater for checking, use Sparkle/WinSparkle only for downloading/installing.

#### Advantages:
- Leverages existing update check infrastructure
- Easier gradual migration
- More flexible over custom behavior

#### Disadvantages:
- More code to maintain
- Less benefit from native frameworks

### Option 3: Custom XML Feed (Current + Enhancements)

Keep the existing `Updater` but enhance it to be Sparkle-compatible.

#### Advantages:
- Minimal changes to existing code
- Works on all platforms
- Full control over behavior

#### Disadvantages:
- Manual implementation of download/installation
- No native delta updates
- Requires custom progress UI

---

## Implementation Steps for Option 1 (Recommended)

### 1. Add Dependencies via CMake

Create [cmake/FindSparkle.cmake](cmake/FindSparkle.cmake) and [cmake/FindWinSparkle.cmake](cmake/FindWinSparkle.cmake) or add via package managers:

**macOS (Sparkle):**
```cmake
if(APPLE)
    # Option A: Use CocoaPods/Homebrew
    find_package(Sparkle REQUIRED)
    
    # Option B: Download from GitHub releases
    # set(SPARKLE_FRAMEWORK_PATH "deps/Sparkle")
endif()
```

**Windows (WinSparkle):**
```cmake
if(WINDOWS)
    find_package(WinSparkle REQUIRED)
    # or
    # set(WINSPARKLE_PATH "deps/winsparkle")
endif()
```

### 2. Update CMakeLists.txt

Modify the main [CMakeLists.txt](CMakeLists.txt) to link frameworks and bundle them:

```cmake
# Around line 57-75, update element_app target:
juce_add_gui_app(element_app
    # ... existing properties ...
)

target_sources(element_app PRIVATE src/main.cc)
target_link_libraries(element_app PRIVATE kv::element)

# Add update framework integration
if(APPLE)
    find_package(Sparkle REQUIRED)
    target_link_libraries(element_app PRIVATE ${SPARKLE_FRAMEWORKS})
    
    # CRITICAL: Copy Sparkle.framework into app bundle
    add_custom_command(TARGET element_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${SPARKLE_FRAMEWORK_PATH}/Sparkle.framework"
                "$<TARGET_BUNDLE_CONTENT_DIR:element_app>/Frameworks/Sparkle.framework"
        COMMENT "Copying Sparkle.framework into app bundle"
    )
    
    # Re-sign the app after copying frameworks
    add_custom_command(TARGET element_app POST_BUILD
        COMMAND codesign --deep --force --verify --verbose --sign -
                "$<TARGET_BUNDLE_DIR:element_app>"
        COMMENT "Code-signing app with embedded frameworks"
    )
    
elseif(WINDOWS)
    find_package(WinSparkle REQUIRED)
    target_link_libraries(element_app PRIVATE ${WINSPARKLE_LIB})
    
    # Copy WinSparkle DLL to app directory
    add_custom_command(TARGET element_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                "${WINSPARKLE_DLL}"
                "$<TARGET_FILE_DIR:element_app>/WinSparkle.dll"
        COMMENT "Copying WinSparkle.dll"
    )
endif()
```

#### Important Notes on Framework Bundling:

**macOS (Sparkle):**
- Sparkle is a **dynamic framework** and must be embedded in the app bundle
- Copy location: `Element.app/Contents/Frameworks/Sparkle.framework`
- The framework path is resolved at runtime from the app bundle
- Code signing order matters: frameworks must be signed first, then the app
- Use `codesign --deep` to recursively sign all frameworks
- When using Developer ID signing, all embedded frameworks must be signed with the same certificate

**Windows (WinSparkle):**
- WinSparkle is typically a **DLL** (not a static library)
- Copy to the same directory as the executable or add to `PATH`
- For MSI installers, WinSparkle.dll goes into the installation directory
- Simpler than macOS since no framework nesting required

**Recommended: Find Script for Sparkle**

Create [cmake/FindSparkle.cmake](cmake/FindSparkle.cmake):

```cmake
# Find Sparkle framework on macOS
if(NOT APPLE)
    return()
endif()

# Check common installation locations
find_framework(Sparkle
    PATHS
        /Library/Frameworks
        ~/Library/Frameworks
        /usr/local/Frameworks
    REQUIRED
)

if(Sparkle_FOUND)
    set(SPARKLE_FRAMEWORKS ${Sparkle_FRAMEWORK})
    set(SPARKLE_FRAMEWORK_PATH "${Sparkle_FRAMEWORK}")
    message(STATUS "Found Sparkle framework: ${SPARKLE_FRAMEWORK_PATH}")
else()
    message(FATAL_ERROR "Sparkle framework not found. Install via:")
    message("    brew install sparkle")
    message("    or download from https://github.com/sparkle-project/Sparkle")
endif()
```

### 3. Create Sparkle Wrapper Classes

Create [src/ui/sparkle_updater.hpp](src/ui/sparkle_updater.hpp):

```cpp
#pragma once
#include <element/signals.hpp>

namespace element {

#if JUCE_MAC
class SparkleUpdater {
public:
    SparkleUpdater();
    ~SparkleUpdater();
    
    void setFeedURL(const juce::String& url);
    void setUserAgentString(const juce::String& userAgent);
    void checkForUpdates();
    void checkForUpdatesInBackground();
    
    Signal<void(bool)> sigUpdatesAvailable;  // true = updates available
    Signal<void(int)> sigProgressUpdate;      // 0-100
    
private:
    class Impl;
    std::unique_ptr<Impl> impl;
};
#endif

} // namespace element
```

Create [src/ui/sparkle_updater.mm](src/ui/sparkle_updater.mm):

```objcpp
#include "sparkle_updater.hpp"
#import <Sparkle/Sparkle.h>
#include <element/juce/core.hpp>

namespace element {

class SparkleUpdater::Impl : public NSObject <SPUUpdaterDelegate> {
    // Implement SPUUpdaterDelegate methods
    // Handle update callbacks
};

SparkleUpdater::SparkleUpdater() : impl(std::make_unique<Impl>()) {}
SparkleUpdater::~SparkleUpdater() {}

void SparkleUpdater::setFeedURL(const juce::String& url) {
    // Set [SUUpdater sharedUpdater].feedURL
}

void SparkleUpdater::checkForUpdates() {
    // Call [[SUUpdater sharedUpdater] checkForUpdates:nil]
}

} // namespace element
```

### 4. Create WinSparkle Wrapper

Create [src/ui/winsparkle_updater.hpp](src/ui/winsparkle_updater.hpp) and [src/ui/winsparkle_updater.cpp](src/ui/winsparkle_updater.cpp):

```cpp
#pragma once
#include <element/signals.hpp>

namespace element {

#if JUCE_WINDOWS
class WinSparkleUpdater {
public:
    WinSparkleUpdater();
    ~WinSparkleUpdater();
    
    void setAppDetails(const juce::String& appName,
                       const juce::String& appVersion,
                       const juce::String& companyName);
    void setFeedURL(const juce::String& url);
    void checkForUpdates();
    
    Signal<void()> sigUpdatesAvailable;
    Signal<void()> sigUpdateDownloading;
    Signal<void()> sigUpdateReady;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl;
};
#endif

} // namespace element
```

### 5. Prepare Appcast/Feed Format

Create endpoint at: `https://repo.kushview.net/element/1/stable/{platform}/appcast.xml`

**Format for Sparkle (macOS):**
```xml
<?xml version="1.0" encoding="utf-8"?>
<rss version="2.0" xmlns:sparkle="http://www.andymatuschak.org/xml-namespaces/sparkle">
  <channel>
    <title>Element Updates</title>
    <link>https://kushview.net/element</link>
    <description>Updates for Element audio plugin host</description>
    <item>
      <title>Element 1.0.0</title>
      <description>Bug fixes and improvements</description>
      <pubDate>Wed, 01 Jan 2025 00:00:00 +0000</pubDate>
      <sparkle:version>1.0.0</sparkle:version>
      <sparkle:shortVersionString>1.0.0</sparkle:shortVersionString>
      <enclosure url="https://repo.kushview.net/downloads/element-1.0.0.dmg"
                 sparkle:version="1.0.0"
                 sparkle:shortVersionString="1.0.0"
                 length="123456789"
                 type="application/octet-stream"
                 sparkle:edSignature="..." />
      <sparkle:minimumSystemVersion>10.8</sparkle:minimumSystemVersion>
    </item>
  </channel>
</rss>
```

**Format for WinSparkle (Windows):**
```xml
<?xml version="1.0" encoding="UTF-8"?>
<rss version="2.0" xmlns:winspark="http://www.codehaus.org/winsparkle/updates/2.0">
  <channel>
    <title>Element Updates</title>
    <link>https://kushview.net/element</link>
    <description>Updates for Element audio plugin host</description>
    <item>
      <title>1.0.0</title>
      <link>https://repo.kushview.net/downloads/element-1.0.0.msi</link>
      <description>Bug fixes and improvements</description>
      <pubDate>Wed, 01 Jan 2025 00:00:00 +0000</pubDate>
      <winspark:Version>1.0.0</winspark:Version>
      <enclosure url="https://repo.kushview.net/downloads/element-1.0.0.msi"
                 sparkle:version="1.0.0"
                 length="123456789"
                 type="application/octet-stream" />
    </item>
  </channel>
</rss>
```

### 6. Update GuiService Integration

Modify [src/services/guiservice.cpp](src/services/guiservice.cpp):

```cpp
class GuiService::UpdateManager {
public:
    UpdateManager() {
        setupUpdater();
    }
    
    ~UpdateManager() {
        _conn.disconnect();
    }
    
    void check() {
#if JUCE_MAC
        if (sparkleUpdater)
            sparkleUpdater->checkForUpdates();
#elif JUCE_WINDOWS
        if (winsparkleUpdater)
            winsparkleUpdater->checkForUpdates();
#else
        updater.check(true);  // Fallback to custom updater for Linux
#endif
    }
    
private:
    void setupUpdater() {
#if JUCE_MAC
        sparkleUpdater = std::make_unique<SparkleUpdater>();
        sparkleUpdater->setFeedURL(ELEMENT_UPDATES_URL);
        // Connect signals...
#elif JUCE_WINDOWS
        winsparkleUpdater = std::make_unique<WinSparkleUpdater>();
        winsparkleUpdater->setAppDetails(...);
        winsparkleUpdater->setFeedURL(ELEMENT_UPDATES_URL);
        // Connect signals...
#else
        // Linux uses custom updater
        updater.setRepository(ELEMENT_UPDATES_URL);
#endif
    }
    
#if JUCE_MAC
    std::unique_ptr<SparkleUpdater> sparkleUpdater;
#elif JUCE_WINDOWS
    std::unique_ptr<WinSparkleUpdater> winsparkleUpdater;
#else
    element::Updater updater;
#endif
    
    boost::signals2::connection _conn;
};
```

### 7. Framework Distribution & Bundling

**macOS - Sparkle Framework Bundling:**

The Sparkle framework is a dynamic framework (~3-5 MB) that must be bundled with your app. There are three approaches:

1. **Recommended: CMake Copy + Sign** (see CMakeLists.txt section above)
   ```bash
   # Manual verification:
   ls -la Element.app/Contents/Frameworks/Sparkle.framework
   codesign -v Element.app/Contents/Frameworks/Sparkle.framework
   ```

2. **Alternative: Manual Copy (for DMG distribution)**
   ```bash
   # Before creating DMG:
   cp -r /Library/Frameworks/Sparkle.framework Element.app/Contents/Frameworks/
   codesign --deep -s - Element.app
   ```

3. **CocoaPods Approach** (if using CocoaPods for dependency management)
   - Add `pod 'Sparkle'` to Podfile
   - CocoaPods handles framework embedding and build phases

**Important Bundling Considerations:**
- Sparkle must be in `Contents/Frameworks` directory of `.app` bundle
- Framework is found via `@rpath` or `@loader_path` at runtime
- Total app size increases by ~3-5 MB
- DMG distribution works fine with bundled framework
- Notarization on macOS may require additional handling

**Windows - WinSparkle DLL Distribution:**

WinSparkle is a single DLL (~1 MB) that's much simpler:

```cmake
# In CMakeLists.txt:
add_custom_command(TARGET element_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            "${WINSPARKLE_DLL}"
            "$<TARGET_FILE_DIR:element_app>/WinSparkle.dll"
)
```

- For portable/ZIP: Include WinSparkle.dll in same directory as .exe
- For MSI installer: Include WinSparkle.dll in installation package
- For Windows Store: Use embedded/bundled approach via WiX

---

### 8. Code Signing Requirements

**macOS:**
- All builds must be signed with a Developer ID certificate
- **Critical**: Sign frameworks before signing the app
- Signing order:
  1. Sign Sparkle.framework
  2. Sign element_app (uses `--deep` to sign all contents)
  3. Verify with `codesign -v --deep element_app`
- Sparkle verifies signatures for security
- Generate EdDSA signing keys for delta updates:
  ```bash
  generate_keys
  # Store public key in app bundle
  # Store private key securely for build server
  ```
- Notarization required for distribution (macOS Catalina+)

**Windows:**
- Enable code signing with a certificate authority
- WinSparkle can verify signatures
- Recommended: Authenticode signing for .msi or .exe
- For MSI: Use WiX toolset with certificate
- For EXE: Use signtool.exe with code-signing certificate

### 8. Update Installation & Packaging

For both platforms, ensure installers are created:

- **macOS**: Use [cpack/dmg.cmake](cmake/) to create .dmg with automatic update capability
- **Windows**: Use NSIS or WiX to create .msi with update capability
- Both should maintain app in `/Applications` or `Program Files` with proper permissions

---

## Linux Considerations

Linux doesn't have a standard auto-updater like macOS/Windows. Options:

1. **Keep custom XML updater** - Direct users to download/update manually
2. **Flatpak/Snap** - Let platform handle updates
3. **AppImage + AppImageUpdate** - Self-updating AppImage containers

---

## Build Instructions

### macOS
```bash
# Install Sparkle via Homebrew
brew install sparkle

# Verify Sparkle location
ls /usr/local/Frameworks/Sparkle.framework

# Build with Sparkle support (framework will be auto-bundled via CMake)
cmake -B build \
  -DELEMENT_BUILD_PLUGINS=ON \
  -DELEMENT_BUILD_TESTS=OFF
cmake --build build

# Verify framework was bundled
ls -la build/element_app_artefacts/Release/Element.app/Contents/Frameworks/Sparkle.framework

# Verify code signing
codesign -v --deep build/element_app_artefacts/Release/Element.app
```

### Windows
```bash
# Install WinSparkle via vcpkg
vcpkg install winsparkle

# Build with WinSparkle support (DLL will be auto-copied via CMake)
cmake -B build ^
  -DELEMENT_BUILD_PLUGINS=ON ^
  -DELEMENT_BUILD_TESTS=OFF
cmake --build build

# Verify DLL was copied
dir "build\element_app_artefacts\Release\WinSparkle.dll"
```

### Troubleshooting Framework/DLL Issues

**macOS - Framework not found at runtime:**
```bash
# Check if framework is in app bundle
otool -L Element.app/Contents/MacOS/Element | grep Sparkle

# Expected output:
# @rpath/Sparkle.framework/Versions/A/Sparkle (compatibility version 2.0.0)

# If missing, manually copy:
mkdir -p Element.app/Contents/Frameworks
cp -r /usr/local/Frameworks/Sparkle.framework Element.app/Contents/Frameworks/
codesign --deep -s - Element.app
```

**Windows - WinSparkle.dll not found:**
```batch
REM Check if DLL exists
dir Element.exe WinSparkle.dll

REM If missing, copy manually to same directory as executable
copy "C:\path\to\WinSparkle.dll" ".\WinSparkle.dll"
```

---

## Migration Path

1. **Phase 1**: Add framework dependencies and create wrapper classes
2. **Phase 2**: Update GuiService to use new wrappers on macOS/Windows
3. **Phase 3**: Update build configuration to enable code signing
4. **Phase 4**: Create appcast feeds and test updates
5. **Phase 5**: Maintain backward compatibility with Linux using custom updater

---

## Files to Create/Modify

### New Files
- `cmake/FindSparkle.cmake` - CMake finder for Sparkle
- `cmake/FindWinSparkle.cmake` - CMake finder for WinSparkle
- `src/ui/sparkle_updater.hpp` - Sparkle wrapper header
- `src/ui/sparkle_updater.mm` - Sparkle wrapper implementation
- `src/ui/winsparkle_updater.hpp` - WinSparkle wrapper header
- `src/ui/winsparkle_updater.cpp` - WinSparkle wrapper implementation

### Modified Files
- `CMakeLists.txt` - Add framework linking
- `src/services/guiservice.cpp` - Use native updaters
- `include/element/ui/updater.hpp` - Optional: deprecate or keep for Linux
- `src/ui/updater.cpp` - Optional: keep for Linux fallback

---

## Dependencies & Licensing

- **Sparkle**: MIT License (Mac) - compatible with GPL-3.0
- **WinSparkle**: MIT License (Windows) - compatible with GPL-3.0
- Both are stable, widely-used projects with good community support

---

## References

- [Sparkle Documentation](https://sparkle-project.org/)
- [WinSparkle Documentation](https://github.com/vslavik/winsparkle)
- [Element Current Update System](src/ui/updater.cpp)
- [Element Settings Management](src/settings.cpp)
