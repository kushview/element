#!/usr/bin/env python
# encoding: utf-8
# Michael Fisher, 2023

from optparse import OptionParser
import subprocess
from subprocess import call, Popen, PIPE
import os
import shutil

from sys import platform

def options():
    parser = OptionParser()
    
    parser.add_option ("--binary", type="string", dest="binary")
    parser.add_option ("--no-strip", action='store_true', dest='no_strip', default=False)
    parser.add_option ("--output", type="string", dest="output")
    parser.add_option ("--type", type="string", dest="type", default="macapp")
    parser.add_option ("--plist", type='string', dest='plist')
    parser.add_option ("--resource", type='string', dest='resource')
    parser.add_option ("--bindir", type='string', dest='bindir')
    parser.add_option ("--signtool-sha1", type="string", default='', dest='signtool_sha1')
    (opts, _) = parser.parse_args()
    return opts

opts = options()

if len (opts.output) <= 0:
    print ("no output specified")
    exit (1)

should_strip = not opts.no_strip

basename = os.path.basename
join = os.path.join

if os.path.exists (opts.output):
    if os.path.isdir(opts.output):
        shutil.rmtree(opts.output, ignore_errors=True)
    else:
        os.remove(opts.output)

if opts.type == 'macapp' or opts.type == 'bundle':
    # generic mac app or other bundle with "Contents/MacOS"
    # this is also used for vst3 and vst2 on OSX because of the plist
    if len (opts.plist) <= 0:
        print ("macapp and bundle require --plist")
        exit (1)

    contents = join (opts.output, 'Contents')
    for dir in 'MacOS Resources'.split():
        if not os.path.exists (join (contents, dir)):
            os.makedirs (join (contents, dir))
    
    if os.path.exists (opts.binary):
        tgt = join (contents, 'MacOS', basename (opts.binary))
        shutil.copy (opts.binary, tgt)
        if should_strip:
            if platform == "linux" or platform == "linux2":
                pass
            elif platform == "darwin":
                call (['/usr/bin/strip', '-x', '-arch', 'all', tgt],
                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    if os.path.exists (opts.plist):
        shutil.copyfile (opts.plist, join (contents, 'Info.plist'))
    
    if opts.resource != None and len(opts.resource) > 0 and os.path.exists (opts.resource):
        shutil.copy (opts.resource, join (contents, 'Resources', basename (opts.resource)))

    with open(join (contents, 'PkgInfo'), 'w') as f:
        if opts.type == 'macapp':
            f.write ("APPL????")
        else:
            f.write ("BNDL????")
        f.close()

    call (['touch', opts.output])

elif opts.type == 'vst3':
    if opts.bindir == None or len(opts.bindir) <= 0:
        print ("Need --bindir for VST3")
        exit (1)
    
    contents = join (opts.output, 'Contents')
    for dir in [ opts.bindir, 'Resources' ]:
        if not os.path.exists (join (contents, dir)):
            os.makedirs (join (contents, dir))
    if os.path.exists (opts.binary):
        if should_strip:
            if platform == "win32":
                pass

        binary_target = join (contents, opts.bindir, basename (opts.binary))
        shutil.copy (opts.binary, binary_target)
        if len(opts.signtool_sha1) > 0:
            call (['SignTool', 'sign', '/as', '/v', '/sha1', opts.signtool_sha1,
                   '/fd', 'SHA256', 
                   '/tr', 'http://sha256timestamp.ws.symantec.com/sha256/timestamp', 
                   binary_target ])

    if opts.resource != None and len(opts.resource) > 0 and os.path.exists (opts.resource):
        shutil.copy (opts.resource, join (contents, 'Resources', basename (opts.resource)))

else:
    print ("No valid bundle type specified")
    exit (2)

exit (0)