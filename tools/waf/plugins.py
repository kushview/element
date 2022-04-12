#!/usr/bin/env python

import os

def install_path (ctx, name):
    return os.path.join (ctx.env.LIBDIR, 'element/%s.element' % name)

def derive_env (ctx):
    env = ctx.env.derive()
    
    if ctx.host_is_windows():
        env.cshlib_PATTERN = env.cxxshlib_PATTERN = '%s.dll'
    elif ctx.host_is_linux():
        env.cshlib_PATTERN = env.cxxshlib_PATTERN = '%s.so'
    elif ctx.host_is_mac():
        env.cshlib_PATTERN = env.cxxshlib_PATTERN = '%s.dylib'

    for f in 'CFLAGS CXXFLAGS LINKFLAGS'.split():
        env.append_unique (f, [ '-fvisibility=hidden', '-fPIC' ])

    return env
