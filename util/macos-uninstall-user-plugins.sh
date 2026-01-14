#!/bin/sh
# SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
# SPDX-License-Identifier: GPL-3.0-or-later

rootdir="$HOME/Library/Audio/Plug-Ins"
find "$rootdir" -name "KV-Element*" -exec rm -rf {} \; > /dev/null
