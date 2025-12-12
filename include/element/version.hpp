// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>

#include <element/version.h>

namespace element {

using VersionSegments = std::vector<std::string>;

/** Representation of a version number */
class Version {
public:
    inline explicit Version (const std::string& version = "")
    {
        _hex = toHex (version);
        _build = build (version);
    }

    inline ~Version() = default;

    /** Returns the version string with git hash appended */
    inline static std::string withGitHash()
    {
        std::string result (ELEMENT_VERSION_STRING);
        if (strlen (ELEMENT_GIT_SHORT_HASH) > 0)
            result += std::string ("-") + std::string (ELEMENT_GIT_SHORT_HASH);
        return result;
    }

    /** Parse to segments */
    inline static VersionSegments segments (const std::string& version,
                                            const std::string& delim = ",.-")
    {
        VersionSegments s;

        boost::split (s, version, boost::is_any_of (delim));
        for (auto& seg : s) {
            boost::trim (seg);
        }

        return s;
    }

    /** Parses the versin string to a hex integer. */
    inline static int toHex (const std::string& version)
    {
        const auto segs (segments (version));
        int value = 0;
        if (segs.size() >= 3) {
            value = (std::stoi (segs[0]) << 24)
                    + (std::stoi (segs[1]) << 16)
                    + (std::stoi (segs[2]) << 8);
        }

        if (segs.size() >= 4)
            value += std::stoi (segs[3]);

        return value;
    }

    /** Parses the build number from a version string. i.e. the 4th version 
        segment.
     */
    inline static int build (const std::string& version)
    {
        const auto segs (segments (version));
        if (segs.size() >= 4)
            return std::stoi (segs[3]);
        return 0;
    }

    /** Returns the version as a hex integer. */
    inline int asHex() const noexcept { return _hex; }

    /** Returns the build number. */
    inline int build() const noexcept { return _build; }

    /** @internal */
    inline Version (const Version& o) { operator= (o); }
    /** @internal */
    inline Version& operator= (const Version& o)
    {
        this->_build = o._build;
        this->_hex = o._hex;
        return *this;
    }

    inline bool operator== (const Version& o) const noexcept { return _hex == o._hex; }
    inline bool operator!= (const Version& o) const noexcept { return _hex != o._hex; }
    inline bool operator> (const Version& o) const noexcept { return _hex > o._hex; }
    inline bool operator< (const Version& o) const noexcept { return _hex < o._hex; }
    inline bool operator>= (const Version& o) const noexcept { return _hex >= o._hex; }
    inline bool operator<= (const Version& o) const noexcept { return _hex <= o._hex; }

private:
    int _hex = 0x00;
    int _build = 0x00;
};

} // namespace element
