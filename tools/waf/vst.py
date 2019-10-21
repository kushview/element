#!/usr/bin/env python
# -*- coding: utf-8 -*-

from waflib.Configure import conf

def options (opt):
    opt.add_option ('--vstsdk', dest='vstsdk', type='string', help='Path to VST 2.4 sdk', default='/opt/element/sdks/vstsdk2.4')

@conf
def check_vst (self, required=False):
    # run and gun on this one
    self.env.append_unique ("CFLAGS", ["-I"+self.options.vstsdk])
    self.env.append_unique ("CXXFLAGS", ["-I"+self.options.vstsdk])
    self.check (header_name="public.sdk/source/vst2.x/audioeffect.h", mandatory=required)
    self.check (header_name="public.sdk/source/vst2.x/audioeffectx.h", mandatory=required)
    self.env.INCLUDES_VST = self.options.vstsdk
