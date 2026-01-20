// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/ui.hpp>
#include <element/ui/content.hpp>
#include <element/session.hpp>
#include "presetmanager.hpp"
#include <element/context.hpp>

#include <element/datapath.hpp>
#include "services/presetservice.hpp"

using juce::String;

namespace element {

struct PresetService::Impl
{
    Impl() {}
    ~Impl() {}

    void refresh()
    {
    }
};

PresetService::PresetService()
{
    impl.reset (new Impl());
}

PresetService::~PresetService()
{
    impl.reset (nullptr);
}

void PresetService::activate()
{
}

void PresetService::deactivate()
{
}

void PresetService::refresh()
{
    context().presets().refresh();
}

void PresetService::add (const Node& node, const String& presetName)
{
    const DataPath path;

    if (! node.savePresetTo (path, presetName))
    {
        AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                          TRANS ("Preset"),
                                          TRANS ("Could not save preset"));
    }
    else
    {
        context().presets().refresh();
    }

    if (auto* ui = sibling<UI>())
        ui->stabilizeContent();
}

} // namespace element
