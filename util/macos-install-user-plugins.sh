
# Audio Units
if [ -d "./plugins/instrument/au/KV-Element.component/" ]; then
    rsync -var --delete "./plugins/instrument/au/KV-Element.component/" \
        "$HOME/Library/Audio/Plug-Ins/Components/KV-Element.component/"
fi

if [ -d "./plugins/effect/au/KV-Element-FX.component/" ]; then
    rsync -var --delete "./plugins/effect/au/KV-Element-FX.component/" \
        "$HOME/Library/Audio/Plug-Ins/Components/KV-Element-FX.component/"
fi

if [ -d "./plugins/midieffect/au/KV-Element-MFX.component/" ]; then
    rsync -var --delete "./plugins/midieffect/au/KV-Element-MFX.component/" \
        "$HOME/Library/Audio/Plug-Ins/Components/KV-Element-MFX.component/"
fi

# VST2
if [ -d "./plugins/instrument/vst2/KV-Element.vst/" ]; then
    rsync -var --delete "./plugins/instrument/vst2/KV-Element.vst/" \
        "$HOME/Library/Audio/Plug-Ins/VST/KV-Element.vst/"
fi

if [ -d "./plugins/effect/vst2/KV-Element-FX.vst/" ]; then
    rsync -var --delete "./plugins/effect/vst2/KV-Element-FX.vst/" \
        "$HOME/Library/Audio/Plug-Ins/VST/KV-Element-FX.vst/"
fi


# VST3
if [ -d "./plugins/instrument/vst3/KV-Element.vst3/" ]; then
    rsync -var --delete "./plugins/instrument/vst3/KV-Element.vst3/" \
        "$HOME/Library/Audio/Plug-Ins/VST3/KV-Element.vst3/"
fi

if [ -d "./plugins/effect/vst3/KV-Element-FX.vst3/" ]; then
    rsync -var --delete "./plugins/effect/vst3/KV-Element-FX.vst3/" \
        "$HOME/Library/Audio/Plug-Ins/VST3/KV-Element-FX.vst3/"
fi
