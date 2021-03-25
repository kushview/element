import os, re
from waflib import Utils, Task, Errors
from waflib.TaskGen import extension, taskgen_method, feature
from waflib.Configure import conf

@conf
def libname_haxe(self, libname):
	return libname

@conf
def check_lib_haxe(self, libname, uselib_store=None):
	haxe_libs = [node.name for node in self.root.find_node('haxe_libraries').ant_glob()]
	changed = False
	self.start_msg('Checking for library %s' % libname)
	if libname + '.hxml' in haxe_libs:
		self.end_msg('yes')
	else:
		changed = True
		try:
			cmd = self.env.LIX + ['+lib', libname]
			res = self.cmd_and_log(cmd)
			if (res):
				raise Errors.WafError(res)
			else:
				self.end_msg('downloaded', color = 'YELLOW')
		except Errors.WafError as e:
			self.end_msg('no', color = 'RED')
			self.fatal('Getting %s has failed' % libname)

	postfix = uselib_store if uselib_store else libname.upper()
	self.env['LIB_' + postfix] += [self.libname_haxe(libname)]
	return changed

@conf
def check_libs_haxe(self, libnames, uselib_store=None):
	changed = False
	for libname in Utils.to_list(libnames):
		if self.check_lib_haxe(libname, uselib_store):
			changed = True
	return changed

@conf
def ensure_lix_pkg(self, *k, **kw):
	if kw.get('compiler') == 'hx':
		if isinstance(kw.get('libs'), list) and len(kw.get('libs')):
			changed = self.check_libs_haxe(kw.get('libs'), kw.get('uselib_store'))
			if changed:
				try:
					cmd = self.env.LIX + ['download']
					res = self.cmd_and_log(cmd)
					if (res):
						raise Errors.WafError(res)
				except Errors.WafError as e:
					self.fatal('lix download has failed')
		else:
			self.check_lib_haxe(kw.get('lib'), kw.get('uselib_store'))

@conf
def haxe(bld, *k, **kw):
	task_gen = bld(*k, **kw)

class haxe(Task.Task):
	vars = ['HAXE', 'HAXE_VERSION', 'HAXEFLAGS']
	ext_out = ['.hl', '.c', '.h']

	def run(self):
		cmd = self.env.HAXE + self.env.HAXEFLAGS
		return self.exec_command(cmd, stdout = open(os.devnull, 'w'))

@taskgen_method
def init_haxe_task(self, node):
	def addflags(flags):
		self.env.append_value('HAXEFLAGS', flags)

	if node.suffix() == '.hxml':
		addflags(self.path.abspath() + '/' + node.name)
	else:
		addflags(['-main', node.name])
	addflags(['-hl', self.path.get_bld().make_node(self.target).abspath()])
	addflags(['-cp', self.path.abspath()])
	addflags(['-D', 'resourcesPath=%s' % getattr(self, 'res', '')])
	if hasattr(self, 'use'):
		for dep in self.use:
			if self.env['LIB_' + dep]:
				for lib in self.env['LIB_' + dep]: addflags(['-lib', lib])

@extension('.hx', '.hxml')
def haxe_file(self, node):
	if len(self.source) > 1:
		self.bld.fatal('Use separate task generators for multiple files')

	try:
		haxetask = self.haxetask
	except AttributeError:
		haxetask = self.haxetask = self.create_task('haxe')
		self.init_haxe_task(node)

	haxetask.inputs.append(node)
	haxetask.outputs.append(self.path.get_bld().make_node(self.target))

@conf
def find_haxe(self, min_version):
	npx = self.env.NPX = self.find_program('npx')
	self.env.LIX = npx + ['lix']
	npx_haxe = self.env.HAXE = npx + ['haxe']
	try:
		output = self.cmd_and_log(npx_haxe + ['-version'])
	except Errors.WafError:
		haxe_version = None
	else:
		ver = re.search(r'\d+.\d+.\d+', output).group().split('.')
		haxe_version = tuple([int(x) for x in ver])

	self.msg('Checking for haxe version',
	         haxe_version, haxe_version and haxe_version >= min_version)
	if npx_haxe and haxe_version < min_version:
		self.fatal('haxe version %r is too old, need >= %r' % (haxe_version, min_version))

	self.env.HAXE_VERSION = haxe_version
	return npx_haxe

@conf
def check_haxe(self, min_version=(4,1,4)):
	if self.env.HAXE_MINVER:
		min_version = self.env.HAXE_MINVER
	find_haxe(self, min_version)

def configure(self):
	self.env.HAXEFLAGS = []
	self.check_haxe()
	self.add_os_flags('HAXEFLAGS', dup = False)
