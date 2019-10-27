/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

#define JucePlugin_Name                   "Element FX"
#define JucePlugin_Desc                   "Modular Instrument and Effects Rack"
#define JucePlugin_ManufacturerCode       0x4b736856 // 'KshV'
#define JucePlugin_PluginCode             0x456c4658 // 'ElFX'

#define JucePlugin_IsSynth                0
#define JucePlugin_WantsMidiInput         1
#define JucePlugin_ProducesMidiOutput     1
#define JucePlugin_IsMidiEffect           0

#define JucePlugin_VSTUniqueID            JucePlugin_PluginCode
#define JucePlugin_VSTCategory            kPlugCategEffect

#define JucePlugin_AUMainType             kAudioUnitType_MusicEffect
#define JucePlugin_AUSubType              JucePlugin_PluginCode
#define JucePlugin_AUExportPrefix         ElementFXAU
#define JucePlugin_AUExportPrefixQuoted   "ElementFXAU"

#define JucePlugin_CFBundleIdentifier     net.kushview.plugins.ElementFX

#define JucePlugin_AAXIdentifier          net.kushview.ElementFX
#define JucePlugin_AAXManufacturerCode    JucePlugin_ManufacturerCode
#define JucePlugin_AAXProductId           JucePlugin_PluginCode
#define JucePlugin_AAXCategory            AAX_ePlugInCategory_None
#define JucePlugin_AAXDisableBypass       0
#define JucePlugin_AAXDisableMultiMono    0

#define JucePlugin_IAAType                0x61757269 // 'auri'
#define JucePlugin_IAASubType             JucePlugin_PluginCode
#define JucePlugin_IAAName                "Kushview: Element FX"
