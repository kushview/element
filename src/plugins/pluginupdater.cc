// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/ui/updater.hpp>

namespace element {

//=============================================================================
class PluginUpdater : public Updater
{
public:
    PluginUpdater() = default;
    ~PluginUpdater() = default;
};

std::unique_ptr<Updater> Updater::create()
{
    return std::make_unique<PluginUpdater>();
}

} // namespace element
