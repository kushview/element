#!/usr/bin/env python

import os
from subprocess import Popen, PIPE
from waflib import Utils
from waflib.Configure import conf
from waflib.TaskGen import taskgen_method

def options (opt):
    opt.add_option ('--host', default='', type='string', dest="host", \
                    help="Use a cross compiler [ Default: none ]")

def guess_host():
    scpt = os.path.join (os.getcwd(), 'config.guess')
    cmd = ['sh', scpt]
    if not isinstance (cmd, list): return ''
    P = Popen (cmd, stdout=PIPE)
    (r, _) = P.communicate()
    P.wait()
    return r.strip()

def configure (conf):
    '''cross compiling a a pre-load routine, see setup_compiler
       this block happens after setup_compiler'''
    conf.env.HOST = conf.options.host
    if isinstance (conf.env.HOST, str) or len (conf.env.HOST) <= 0:
        conf.env.HOST = guess_host()
    conf.env.BUILD_PLATFORM = Utils.unversioned_sys_platform()
    conf.env.HOST_PLATFORM  = conf.env.BUILD_PLATFORM

@conf
@taskgen_method
def host_is_mingw32 (self):
    return 'mingw32' in self.env.HOST or 'w64' in self.env.HOST

@conf
@taskgen_method
def host_is_windows (self):
    return self.host_is_mingw32()

@conf
def host_is_mac (self):
    return 'apple-darwin' in self.env.HOST
