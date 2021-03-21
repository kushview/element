import os,shutil,platform
from waflib import Task ,Utils
from waflib.TaskGen import taskgen_method, feature, after_method, before_method, process_source

@feature ('bundle')
@before_method ('process_source')
def create_task_bundle (self):
    if not getattr (self, 'bundle', False):
        return
    B = getattr (self, 'bundle')
    dir = self.path.find_or_declare (B)
    self.create_task ('bundle',[], dir)

@feature ('cshlib', 'cstlib', 'bundle')
@after_method ('apply_link')
def list_includes (self):
    have_bundle = isinstance (getattr (self, 'bundle', False), str)
    if have_bundle and self.link_task:
        print "hello"

class bundle (Task.Task):
    color='PINK'
    def run (self):
        self.outputs[0].mkdir()
