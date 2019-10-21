#!/usr/bin/env python

def options (opt):
    opt.add_option ('--cross', default='', type='string', dest="cross", \
                    help="Use a cross compiler [ Default: none ]")

def configure (conf):
    '''cross compiling a a pre-load routine, see setup_compiler
       this block happens after setup_compiler'''
    cross_compiling = conf.env.CROSS_COMPILING = len(conf.options.cross) > 0
    conf.env.CROSS = conf.options.cross

    if cross_compiling:
        pass

def setup_compiler(conf):
    '''setup the (cross) compiler. This function should be called
       in waf "configure" but before other tools are loaded'''
    if not len(conf.options.cross) > 0:
        return

    using_gcc = True # this needs to be detected and/or an option

    if using_gcc:
        conf.env.AR     = '%s-ar' % conf.options.cross
        conf.env.CC     = '%s-gcc' % conf.options.cross
        conf.env.CXX    = '%s-g++' % conf.options.cross
        conf.env.STRIP  = '%s-strip' % conf.options.cross

def is_mingw (ctx):
    return 'mingw' in ctx.env.CROSS or 'w64' in ctx.env.CROSS

def is_windows (ctx):
    return is_mingw (ctx)
