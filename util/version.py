#!/usr/bin/env python
# encoding: utf-8
# Michael Fisher, 2022

from optparse import OptionParser
from subprocess import call, Popen, PIPE
import os

VERSION="1.0.0"
CURRENT_REVISION="origin/develop"
LAST_VERSION="853bdc30cb6e9839e0585037f35e42f0b02bc080"

def options():
    parser = OptionParser()
    
    parser.add_option ("--current-version", type="string", dest="current_version", 
                        default=VERSION, help="The current version string to display")
    parser.add_option ("--current-revision", type="string", dest="current_revision", 
                        default=CURRENT_REVISION, help="The ending revision to count commits to.")
    parser.add_option ("--last-version", type="string", dest="last_version", 
                        default=LAST_VERSION, help="The last version, hash, tag, etc in git to count commits from")
    parser.add_option ("--before", type="string", dest="before", default='', help="Prefix string")
    parser.add_option ("--after", type="string", dest="after", default='', help="Postfix string")
    
    parser.add_option ("--cwd", type="string", dest="cwd", default='', help="path to git repository")

    parser.add_option ("--hash", action="store_true", dest="hash", default=False)
    parser.add_option ("--short-hash", action="store_true", dest="short_hash", default=False)

    parser.add_option ("--build", action="store_true", dest="build", default=False)
    parser.add_option ("--build-style", type='string', dest='build_style', default='dashed')
    
    parser.add_option ("--ignore-dirty", action="store_true", dest="ignore_dirty", default=False,
                       help="Ignore dirty flag")

    (opts, _) = parser.parse_args()
    return opts

def call_git (ctx, args):
    if not os.path.exists('.git'):
        return (False, 'Not a git repository')
    cmd = ['git'] if ctx == None else [] + ctx.env.GIT
    if not isinstance (cmd, list): return False
    if not isinstance (args, list): args = []
    devnull = open (os.devnull, 'w')
    cmd += args
    P = Popen (cmd, stdout=PIPE, stderr=devnull)
    (r, err) = P.communicate()
    P.wait()
    return (r, err)

def exists():
    '''Returns true if the current directory is a git repository'''
    return os.path.exists ('.git')

def ncommits (revision):
    args = 'rev-list %s..%s' % (revision, CURRENT_REVISION)
    (n, err) = call_git (None, args.split())
    if None == err:
        return len (n.split())
    return 0

def build_number (revision):
    bn = ncommits (revision) - 1
    if bn < 0: bn = 0
    return bn

def nchanges():
    if not exists(): return 0
    (out, err) = call_git (None, 'status --porcelain --untracked-files=no'.split())
    if isinstance (out, str):
        return len(out.splitlines())

    return 0

def is_dirty():
    return nchanges() > 0

def get_hash (what='HEAD'):
    if not exists(): return ''
    args = 'rev-parse %s' % what
    (r, e) = call_git (None, args.strip().split())
    return r.strip().decode('utf-8')

def get_hash_short (what='HEAD'):
    if not exists(): return ''
    args = 'rev-parse --short %s' % what
    (r, e) = call_git (None, args.strip().split())
    return r.strip().decode('utf-8')

def version():
    here = os.getcwd()
    opts = options()
    if len(opts.cwd) > 0:
        os.chdir (opts.cwd)

    if opts.short_hash:
        print (get_hash_short())
        return 0
    
    if opts.hash:
        print (get_hash())
        return 0
    
    show_dirty = is_dirty() and not opts.ignore_dirty

    vers = opts.current_version
    if exists():
        if opts.build:
            if opts.build_style == 'dotted':
                vers += '.%s' % build_number (opts.last_version)
            elif opts.build_style == 'dashed':
                vers += '-%s' % build_number (opts.last_version)
            elif opts.build_style == 'revision':
                vers += '_r%s' % build_number (opts.last_version)
            elif opts.build_style == 'onlybuild':
                vers = '%s' % build_number (opts.last_version)
        if show_dirty:
            vers += "-dirty"

    os.chdir (here)
    print ('%s%s%s' % (opts.before, vers, opts.after))
    return 0

if __name__ == '__main__':
    exit (version())
