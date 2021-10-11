import os

def options (self):
    self.add_option ('--depends', default='', dest='depends', type='string', 
                     help='Where dependency tools and libraries are located')

def configure (self):
    self.env.DEPENDSDIR = self.options.depends
    if os.path.exists (self.env.DEPENDSDIR):
        if not bool (self.env.HOST): 
            self.fatal ("depends.py: HOST not set.")
            return
        os.environ['PKG_CONFIG_PATH'] = '%s/%s/lib/pkgconfig' % (self.env.DEPENDSDIR, self.env.HOST)
        os.environ['PKG_CONFIG_LIBDIR'] = os.environ['PKG_CONFIG_PATH']
        self.env.append_unique ('CPPFLAGS_DEPENDS',  ['-I%s/%s/include' % (self.env.DEPENDSDIR, self.env.HOST)])
        self.env.append_unique ('LINKFLAGS_DEPENDS', ['-L%s/%s/lib' % (self.env.DEPENDSDIR, self.env.HOST)])

def host_path (ctx):
    if not bool (ctx.env.HOST):
        ctx.fatal ("depends.py: HOST not set.")
    return '%s/%s' % (ctx.env.DEPENDSDIR, ctx.env.HOST)

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
