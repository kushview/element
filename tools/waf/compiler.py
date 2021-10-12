#!/usr/bin/env python

from waflib.Configure import conf
from waflib.TaskGen import taskgen_method

def options (opt):
    opt.add_option ('--host', default='', type='string', dest="host", \
                    help="Use a cross compiler [ Default: none ]")

def configure (conf):
    '''cross compiling a a pre-load routine, see setup_compiler
       this block happens after setup_compiler'''
    conf.env.HOST = conf.options.host

@conf
@taskgen_method
def host_is_mingw32 (self):
    return 'mingw32' in self.env.HOST or 'w64' in self.env.HOST

@conf
@taskgen_method
def host_is_windows (self):
    return self.host_is_mingw32()
