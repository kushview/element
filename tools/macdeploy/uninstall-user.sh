#!/bin/sh

apps="$HOME/Applications"
rm -rf "$apps/Element.app"

plugins="$HOME/Library/Audio/Plug-Ins"
rm -rf "$plugins/Components/KV_Element.component"
rm -rf "$plugins/VST/KV_Element.vst"
rm -rf "$plugins/VST3/KV_Element.vst3"
rm -rf "$plugins/AAX_unsigned/KV_Element.aaxplugin"

rm -rf "$plugins/Components/KV_ElementFX.component"
rm -rf "$plugins/VST/KV_ElementFX.vst"
rm -rf "$plugins/VST3/KV_ElementFX.vst3"
rm -rf "$plugins/AAX_unsigned/KV_ElementFX.aaxplugin"

rm -rf "$plugins/Components/KV_ElementMFX.component"
