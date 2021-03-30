from waflib.Configure import conf
from subprocess import call, Popen, PIPE
import os

def call_git (ctx, args):
    if not os.path.exists('.git'):
        return (False, 'Not a git repository')
    cmd = [] + ctx.env.GIT
    if not isinstance (cmd, list): return False
    if not isinstance (args, list): args = []
    cmd += args
    P = Popen (cmd, stdout=PIPE)
    (r, err) = P.communicate()
    P.wait()
    return (r, err)

def is_repo():
    return os.path.exists ('.git')

@conf
def find_git (self, **kw):
    kw['var'] = 'GIT'
    return self.find_program ('git', **kw)

@conf
def git_hash (self, what='HEAD'):
    if not is_repo(): return ''
    cmd = 'rev-parse %s' % what
    (r, e) = call_git (self, cmd.strip().split())
    return r.strip()

@conf
def git_short_hash (ctx):
    if not is_repo(): return ''
    githash = 'NA'
    (githash, err) = call_git (ctx, 'rev-parse --short HEAD'.split())
    return githash.strip()

@conf
def git_n_changes (self):
    if not is_repo(): return 0
    (out, err) = call_git (self, 'status --porcelain --untracked-files=no'.split())
    if isinstance (out, str):
        return len(out.splitlines())
    return 0

@conf 
def git_is_dirty (self):
    if not is_repo(): return False
    return self.git_n_changes() > 0

@conf
def git_last_tag (self):
    if not is_repo(): return ''
    (tag, e) = call_git (self, 'describe --abbrev=0'.split())
    return tag.strip()

@conf
def git_update_env (conf, e=None):
    if not is_repo(): return
    if not conf.env.GIT: return
    if not e: e = conf.env
    e.GIT_TAG = None
    tag = conf.git_last_tag()
    if tag and conf.git_hash() == conf.git_hash (tag):
        if conf.git_n_changes() == 0:
            e.GIT_TAG = tag
    shorthash = conf.git_short_hash()
    if conf.git_n_changes() > 0:
        shorthash += '-dirty'
    e.GIT_COMMIT = shorthash
    lasthash = conf.git_hash()
    e.GIT_HASH = lasthash

def configure (conf):
    conf.find_git (mandatory=False)
    conf.git_update_env (conf.env)
