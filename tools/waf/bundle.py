import os,shutil,platform
from waflib import Task ,Utils
from waflib.Configure import conf
from waflib.TaskGen import taskgen_method, feature, after_method, before_method, process_source

@conf
def bundle (self, *k, **kw):
    kw['features'] = 'bundle'
    return self(*k, **kw)

@taskgen_method
def bundle_node (self):
    return self.path.find_or_declare (self.target)

@taskgen_method
def bundle_target (self, leaf):
    return self.bundle_node().find_or_declare (leaf)

@taskgen_method
def bundle_name (self):
    return self.bundle_node().name

@feature ('bundle')
@before_method ('apply_link')
def create_task_bundle (self):
    bundle = self.bundle_node()
    if not bundle: return
    self.bundle_task = self.create_task ('bundle', [], bundle)

class bundle (Task.Task):
    color='PINK'
    def run (self):
        self.outputs[0].mkdir()
