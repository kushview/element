import json,os
from waflib.Configure import conf
from waflib.TaskGen import taskgen_method

CONFIG_KEYS = 'AR CC CXX'.split()

def options (self):
    self.add_option ('--depends', default='', dest='depends', type='string', 
                     help='Where dependency tools and libraries are located')

def configure (self):
    d = self.env.DEPENDSDIR = '%s'.strip() % self.options.depends
    if not os.path.exists (d):
        return
    try:
        configfile = open (os.path.join (d, 'config.json'))
    except:
        self.fatal ("depends.py: could not read config.json")
    config = json.load (configfile)
    
    self.env.HOST = os.path.basename (d)

    os.environ['PKG_CONFIG_PATH'] = '%s/lib/pkgconfig' % d
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
