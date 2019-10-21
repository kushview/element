#!/usr/bin/env python
# -*- coding: utf-8 -*-

import fnmatch, os, shutil, sys, platform, subprocess
from waflib import TaskGen, Task, Build, Options, Utils, Errors
from waflib.TaskGen import taskgen_method, feature, after_method, before_method
from waflib.Tools import c_osx

import element

au_base_code = '''
    AUBase.cpp
    AUBuffer.cpp
    AUCarbonViewBase.cpp
    AUCarbonViewControl.cpp
    AUCarbonViewDispatch.cpp
    AUDispatch.cpp
    AUEffectBase.cpp
    AUInputElement.cpp
    AUMIDIBase.cpp
    AUMIDIEffectBase.cpp
    AUOutputBase.cpp
    AUOutputElement.cpp
    AUScopeElement.cpp
    CAAudioChannelLayout.cpp
    CAAUParameter.cpp
    CAMutex.cpp
    CarbonEventHandler.cpp
    CAStreamBasicDescription.cpp
    CAVectorUnit.cpp
    ComponentBase.cpp
    MusicDeviceBase.cpp
'''.split()

def find_core_audio_file (ctx, root_path, file):
    for root, dirnames, filenames in os.walk (root_path):
        for filename in fnmatch.filter (filenames, file):
            return os.path.join (root, filename)
    return ""

def options(opt):
    opt.add_option ('--core-audio-root', dest='core_audio_root', type='string', \
        help='Path To Core Audio SDK', default='/Developer/Extras/CoreAudio')
    opt.add_option ('--au-libdir', dest='au_libdir', type='string', \
        help='Path To Core Audio SDK', default='/Library/Audio/Plug-Ins/Components')

def configure(conf):
    outdir = conf.options.out
    if len (outdir) == 0:
        outdir = "build"

    # I have no idea why ".." needs prepened to this in order to find the files
    conf.env.CORE_AUDIO_ROOT = os.path.join ("..", os.path.relpath (conf.options.core_audio_root, outdir))
    conf.env.AUDIO_UNIT_LIBDIR = conf.options.au_libdir

    if not conf.env.REZ:
        conf.find_program ("Rez")

@feature ('audiounit')
@before_method ('process_source')
def append_audio_unit_source (self):
    if not getattr(self, "includes", False):
        self.includes = []
    self.includes += [os.path.join (self.env.CORE_AUDIO_ROOT, "AudioUnits/AUPublic")]
    self.includes += [os.path.join (self.env.CORE_AUDIO_ROOT, "AudioUnits/AUPublic/AUBase")]
    self.includes += [os.path.join (self.env.CORE_AUDIO_ROOT, "AudioUnits/AUPublic/AUCarbonViewBase")]
    self.includes += [os.path.join (self.env.CORE_AUDIO_ROOT, "AudioUnits/AUPublic/AUInstrumentBase")]
    self.includes += [os.path.join (self.env.CORE_AUDIO_ROOT, "AudioUnits/AUPublic/AUViewBase")]
    self.includes += [os.path.join (self.env.CORE_AUDIO_ROOT, "AudioUnits/AUPublic/OtherBases")]
    self.includes += [os.path.join (self.env.CORE_AUDIO_ROOT, "AudioUnits/AUPublic/Utility")]
    self.includes += [os.path.join (self.env.CORE_AUDIO_ROOT, "PublicUtility")]

    for f in au_base_code:
        found = find_core_audio_file (self, self.env.CORE_AUDIO_ROOT, f)
        if len (found) > 0:
            self.source += [found]

@feature ('audiounit')
@after_method ('apply_link')
def process_carbon_resource (self):
    resource = getattr (self, "au_resource", False)
    if resource:
        out  = self.link_task.outputs[0]
        name = c_osx.bundle_name_for_output (out,  getattr (self, 'mac_extension', '.component'))
        dir = out.parent.find_or_declare(name)
        dir.mkdir()

        outnode = dir.find_or_declare (['Contents', 'Resources', out.name + ".rsrc"])
        self.aurez_task = self.create_task ('aurez', resource, outnode)

        resdir = getattr (self, 'install_path', self.env.AUDIO_UNIT_LIBDIR) + '/%s/Contents/Resources' % name
        self.bld.install_files (resdir, outnode)

class aurez (Task.Task):
    color = 'RED'
    def run (self):
        cmd = [self.env.REZ, '-o', self.outputs[0].abspath(),
               '-arch', 'x86_64', '-useDF', '-script', 'Roman', '-d', 'x86_64_YES',
               '-d', 'ELEMENT_PLUGIN=1', '-d', 'SystemSevenOrLater=1',
               '-I', '/System/Library/Frameworks/CoreServices.framework/Frameworks/CarbonCore.framework/Versions/A/Headers',
               ]

        for i in self.env.INCPATHS:
            cmd.append ("-I")
            cmd.append (i)

        cmd.append (self.inputs[0].srcpath())
        subprocess.call (cmd)
'''
For Reference: an example shell script that works...

#!/bin/bash
/Applications/Xcode-4.3.app/Contents/Developer/usr/bin/Rez \
-o juce_AU_Resources.rsrc \
-arch x86_64 \
-useDF \
-script Roman \
-define ELEMENT_PLUGIN=1 \
-d "ppc_$ppc" \
-d "i386_$i386" \
-d "ppc64_$ppc64" \
-d x86_64_YES \
-d SystemSevenOrLater=1 \
-I /System/Library/Frameworks/CoreServices.framework/Frameworks/CarbonCore.framework/Versions/A/Headers \
-I /Applications/Xcode-4.3.app/Contents/Developer/Extras/CoreAudio/AudioUnits/AUPublic/AUBase \
-I /Applications/Xcode-4.3.app/Contents/Developer/Extras/CoreAudio/AudioUnits/AUPublic/AUCarbonViewBase \
-I /Applications/Xcode-4.3.app/Contents/Developer/Extras/CoreAudio/AudioUnits/AUPublic/AUBase \
-I element/juce \
-I libs/juce/modules/juce_audio_plugin_client \
-I libs/juce/modules/juce_audio_plugin_client/RTAS \
-I libs/juce/modules/juce_audio_plugin_client/utility \
-I /Applications/Xcode-4.3.app/Contents/Developer/Extras/CoreAudio/AudioUnits/AUPublic/OtherBases \
-I /Applications/Xcode-4.3.app/Contents/Developer/Extras/CoreAudio/AudioUnits/AUPublic/AUViewBase \
-I /Applications/Xcode-4.3.app/Contents/Developer/Extras/CoreAudio/PublicUtility \
-I /Applications/Xcode-4.3.app/Contents/Developer/Extras/CoreAudio/AudioUnits/AUPublic/Utility \
-I /Applications/Xcode-4.3.app/Contents/Developer/Extras/CoreAudio/PublicUtility \
-I /Applications/Xcode-4.3.app/Contents/Developer/Extras/CoreAudio/AudioUnits/AUPublic/Utility \
-I /Applications/Xcode-4.3.app/Contents/Developer/Extras/CoreAudio/AudioUnits/AUPublic/AUBase \
-isysroot /Applications/Xcode-4.3.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk \
libs/juce/modules/juce_audio_plugin_client/AU/juce_AU_Resources.r
'''
