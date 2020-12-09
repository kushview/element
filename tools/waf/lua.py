import sys

def derive_env (ctx):
    env = ctx.env.derive()
    if sys.platform == 'windows':
        env.cshlib_PATTERN = env.cxxshlib_PATTERN = '%s.dll'
    else:
        env.cshlib_PATTERN = env.cxxshlib_PATTERN = '%s.so'
        env.macbundle_PATTERN = '%s.so'
    return env

def module (ctx):
    mod = ctx (
        features    = 'c cshlib',
        env         = derive_env (ctx),
        use         = [],
        cflags      = [],
        linkflags   = [],
    )

    if 'linux' in sys.platform:
        mod.env.append_unique ('CFLAGS', [ '-fPIC' ])
        mod.env.append_unique ('CXXFLAGS', [ '-fPIC' ])
        mod.linkflags.append ('-fPIC')
        mod.linkflags.append ('-fvisibility=hidden')
        mod.cflags.append('-fvisibility=hidden')

    elif 'windows' in sys.platform:
        pass

    elif 'darwin' in sys.platform:
        mod.env.append_unique ('CFLAGS', [ '-fPIC' ])
        mod.env.append_unique ('CXXFLAGS', [ '-fPIC' ])
        mod.linkflags.append ('-fPIC')
        mod.linkflags.append ('-fvisibility=hidden')
        mod.cflags.append ('-fvisibility=hidden')
        mod.mac_bundle = True

    return mod
