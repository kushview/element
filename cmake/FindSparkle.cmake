# SPDX-FileCopyrightText: 2026 Kushview, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

set(SPARKLE_VERSION "2.8.1" CACHE STRING "Sparkle version to download")

set(SPARKLE_FEED_URL "http://localhost:8000/appcast.xml"
    CACHE STRING "The Sparkle feed url to use")
set(SPARKLE_PUBLIC_KEY "+3pPKr274O3KL4lTWaEs/ukVk3Ayvnc7BkdS45zOnE4="
    CACHE STRING "The Sparkle eDSA public key to use")

FetchContent_Declare(
    sparkle
    URL https://github.com/Sparkle-project/Sparkle/releases/download/${SPARKLE_VERSION}/Sparkle-${SPARKLE_VERSION}.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(sparkle)
message(STATUS "Searching for Sparkle in: ${sparkle_SOURCE_DIR}")
find_library(SPARKLE_FRAMEWORK Sparkle HINTS ${sparkle_SOURCE_DIR})

if(SPARKLE_FRAMEWORK)
    message(STATUS "Found Sparkle: ${SPARKLE_FRAMEWORK}")
else()
    message(FATAL_ERROR "Sparkle framework not found")
endif()

string(APPEND ELEMENT_APP_PLIST_TO_MERGE 
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>
    <!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">
    <plist version=\"1.0\">
    <dict>
")
string(APPEND ELEMENT_APP_PLIST_TO_MERGE "<key>SUFeedURL</key>")
string(APPEND ELEMENT_APP_PLIST_TO_MERGE "<string>${SPARKLE_FEED_URL}</string>")
string(APPEND ELEMENT_APP_PLIST_TO_MERGE "<key>SUPublicEDKey</key>")
string(APPEND ELEMENT_APP_PLIST_TO_MERGE "<string>${SPARKLE_PUBLIC_KEY}</string>")
string(APPEND ELEMENT_APP_PLIST_TO_MERGE "<key>SUEnableAutomaticChecks</key>")
string(APPEND ELEMENT_APP_PLIST_TO_MERGE "<false/>")
string(APPEND ELEMENT_APP_PLIST_TO_MERGE "<key>SUScheduledCheckInterval</key>")
string(APPEND ELEMENT_APP_PLIST_TO_MERGE "<integer>0</integer>")
string(APPEND ELEMENT_APP_PLIST_TO_MERGE "</dict></plist>")
