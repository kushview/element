// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/juce/core.hpp>

#include <element/version.h>
#include <element/ui/updater.hpp>
#include <element/juce/core.hpp>
#include <element/juce/events.hpp>
#include <element/datapath.hpp>
#include <element/version.hpp>

using namespace juce;

namespace element {

#if ! ELEMENT_UPDATER
std::unique_ptr<Updater> Updater::create()
{
    std::unique_ptr<Updater> u (new Updater());
    return u;
}
#endif

Updater::Updater() {}
Updater::~Updater() {}

void Updater::check (bool background) { juce::ignoreUnused (background); }
std::string Updater::feedUrl() const noexcept { return {}; }
void Updater::setFeedUrl (const std::string& url) { juce::ignoreUnused (url); }

} // namespace element
