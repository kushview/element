#!/usr/bin/env python

def options (opt):
    opt.add_option ('--host', default='', type='string', dest="host", \
                    help="Use a cross compiler [ Default: none ]")

def configure (conf):
    '''cross compiling a a pre-load routine, see setup_compiler
       this block happens after setup_compiler'''
    conf.env.HOST = conf.options.host

def setup_compiler (conf):
    '''setup the (cross) compiler. This function should be called
       in waf "configure" but before other tools are loaded'''
    if not len (conf.env.HOST) > 0:
        return

    using_gcc = True # this needs to be detected and/or an option
    if using_gcc:
        conf.env.AR      = ['%s-ar' % conf.env.HOST]
        conf.env.CC      = ['%s-gcc' % conf.env.HOST]
        conf.env.CXX     = ['%s-g++' % conf.env.HOST]
        conf.env.STRIP   = ['%s-strip' % conf.env.HOST]

def host_is_mingw32 (ctx):
    return 'mingw32' in ctx.env.HOST or 'w64' in ctx.env.HOST

def host_is_windows (ctx):
    return host_is_mingw32 (ctx)
