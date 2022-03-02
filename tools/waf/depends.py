import json,os
from waflib.Configure import conf
from waflib.TaskGen import taskgen_method

CONFIG_KEYS = 'AR CC CXX'.split()

def options (self):
    self.add_option ('--depends', default='', dest='depends', type='string', 
                     help='Where dependency tools and libraries are located')
    self.add_option ('--depends-allow-system', default=False, dest='depends_allow_system', action='store_true',
                     help='Allow usage of system packages along with those in the depends path.')

def configure (self):
    d = self.env.DEPENDSDIR = '%s'.strip() % self.options.depends
    self.env.HAVE_DEPENDS = False
    allow_system = self.env.DEPENDS_ALLOW_SYSTEM = self.options.depends_allow_system
    if not os.path.exists (d):
        self.fatal ("depends directory does not exist\n  - %s" % d)
    try:
        configfile = open (os.path.join (d, 'share', 'config.json'))
    except:
        self.fatal ("depends.py: could not read config.json")

    config = json.load (configfile)
    self.env.HAVE_DEPENDS = True
    self.env.HOST = os.path.basename (d)

    os.environ['PKG_CONFIG_PATH'] = '%s/lib/pkgconfig' % d
    if not allow_system:
        os.environ['PKG_CONFIG_LIBDIR'] = os.environ['PKG_CONFIG_PATH']

    for k in config:
        if not k in CONFIG_KEYS or len (self.env[k]) > 0:
            continue
        if k == 'AR' or k == 'CC' or k == 'CXX':
            self.env[k] = config[k].split()
        else:
            self.fatal ('depends.py: %s config key not handled' % k)
    
    self.env.append_unique ('CPPFLAGS_DEPENDS',  ['-I%s/include' % d])
    self.env.append_unique ('LINKFLAGS_DEPENDS', ['-L%s/lib' % d])

@conf
@taskgen_method
def host_path (self):
    return '%s' % self.env.DEPENDSDIR

def copydlls (self):
    import subprocess
    import shutil
    d = self.env.DEPENDSDIR
    targetdir = 'build/lib'
    if not os.path.exists (targetdir):
        os.mkdir (targetdir)
    # # Compiled
    # for dll in [ 'build/lib/element-0.dll' ]: #, 'build/lib/element-juce-0.dll' ]:
    #     path = dll
    #     if os.path.exists (path):
    #         print ("copy: %s" % path)
    #         shutil.copy2 (path, 'build/bin')
    #     else:
    #         self.fatal ("could not copy DLL: %s" % dll)
    
    # 3rd party dll's
    for dll in [ 'lib/serd-0.dll', 'lib/sord-0.dll', 'lib/sratom-0.dll', 
                 'lib/lilv-0.dll', 'lib/suil-0.dll' ]:
        path = os.path.join (d, dll)
        if os.path.exists (path):
            print ("copy: %s" % os.path.basename (path))
            shutil.copy2 (path, targetdir)
        else:
            self.fatal ("could not copy DLL: %s" % dll)
    
    # GCC dll's
    dlls = [ 'libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll' ]
    for dll in dlls:
        program = ''
        for prog in self.env.CC:
            if 'gcc' in prog:
                program = prog
                break
        cmd = [ program, '-print-file-name=%s' % dll ]
        path = subprocess.check_output(cmd)
        path = path.strip()

        if os.path.exists (path):
            print ("copy: %s" % os.path.basename (path))
            shutil.copy2 (path, targetdir)
        else:
            self.fatal ("could not copy DLL: %s" % path)

# TODO: figure out how to make this work with out-of-tree / absolute
# resource paths:
#
# from waflib.TaskGen import extension
# from waflib import Task

# @extension('.dll')
# def add_coypdlls(self, node):
# 	tsk = self.create_task('copydlls', node) #, node.change_ext('.luac'))
# 	inst_to = getattr(self, 'install_path', self.env.BINDIR and '${BINDIR}' or None)
# 	if inst_to:
# 		self.add_install_files(install_to=inst_to, install_from=tsk.outputs)
# 	return tsk

# class copydlls(Task.Task):
# 	run_str = 'cp -f ${TGT} ${SRC}'
# 	color   = 'YELLOW'
