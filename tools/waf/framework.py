#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os, shutil, sys, platform

from subprocess import call, Popen, PIPE

from waflib import TaskGen, Task, Build, Options, Utils, Errors
from waflib.TaskGen import taskgen_method, feature, after_method, before_method
from waflib.Tools import c_osx

framework_info = '''<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist SYSTEM "file://localhost/System/Library/DTDs/PropertyList.dtd">
<plist version="0.9">
<dict>
    <key>CFBundleName</key>
    <string>%s</string>
    <key>CFBundleIdentifier</key>
    <string>%s</string>
    <key>CFBundleVersion</key>
    <string>0.0.1</string>
    <key>CFBundleShortVersionString</key>
    <string>0.0.1</string>
    <key>CFBundleSignature</key>
    <string>????</string>
    <key>CFBundlePackageType</key>
    <string>FMWK</string>
    <key>NSHumanReadableCopyright</key>
    <string>%s</string>
    <key>CFBundleGetInfoString</key>
    <string>Created by Element</string>
</dict>
</plist>
'''

def options(opts):
    opts.add_option ('--frameworkdir', default='/Library/Frameworks', dest='frameworkdir', \
        type='string', help="Specify the Framework Installation Path [Default: /Library/Frameworks]")

def configure (conf):
    conf.env.FRAMEWORKDIR = conf.options.frameworkdir
    if not conf.env.FRAMEWORKDIR: conf.env.FRAMEWORKDIR = '/Library/Frameworks'

@taskgen_method
def framework_dir (self):
    return getattr (self, 'install_path', self.env.FRAMEWORKDIR)

@taskgen_method
def create_framework_dirs (self, name, out, version):
    bld = self.bld
    dir = out.parent.find_or_declare (name)
    dir.mkdir()

    subdir = dir.find_or_declare(['Versions', version])
    subdir.mkdir()

    return dir

@feature ('cshlib', 'cxxshlib')
@before_method ('apply_link', 'propagate_uselib_vars')
def prep_framework_lib (self):
    if getattr (self, 'mac_framework', False):
        framework_version = getattr (self, 'framework_version', "A")

        # Remove any file extenstions on the library
        self.env['cshlib_PATTERN'] = self.env['cxxshlib_PATTERN'] = "%s"

        # Fixup the install name based on install_path
        out = os.path.basename (self.target)
        name = out + ".framework"

        install_name = '%s/%s/Versions/%s/%s' % (self.env.FRAMEWORKDIR, name, framework_version, out)
        self.env.append_unique ("LINKFLAGS", ['-install_name', install_name])

        use = self.use = self.to_list (getattr (self, 'use', []))
        if not 'MACFRAMEWORK' in use:
            use.append ('MACFRAMEWORK')

@feature('cshlib', 'cxxshlib')
@after_method('apply_link')
def create_task_mac_framework (self):
    if getattr(self, 'mac_framework', False):
        framework_version = getattr (self, 'framework_version', "A")
        out  = self.link_task.outputs[0]
        name = c_osx.bundle_name_for_output (out, ".framework")
        dir  = self.create_framework_dirs (name, out, framework_version)

        n1 = dir.find_or_declare (['Versions', framework_version, out.name])
        self.framework_task = self.create_task ('macpkg', self.link_task.outputs, n1)

        framework_base = getattr (self, 'install_path', self.framework_dir()) + '/%s' % name

        version_dir = framework_base + '/Versions/%s' % framework_version
        self.bld.install_files (version_dir, n1, chmod=Utils.O755)
        self.bld.symlink_as (framework_base + '/Versions/Current', framework_version)
        self.bld.symlink_as (framework_base + '/%s' % n1.name, "Versions/Current/%s" % n1.name)

        if getattr (self.bld, 'is_install', None):
            # disable the normal binary installation
            self.install_task.hasrun = Task.SKIP_ME

@feature('cshlib', 'cxxshlib')
@after_method('create_task_mac_framework')
def create_task_local_framework_symlinks (self):
    if getattr (self, 'mac_framework', False):

        framework_version = getattr (self, 'framework_version', "A")
        out = self.link_task.outputs[0]
        name = c_osx.bundle_name_for_output (out, ".framework")
        dir = self.create_framework_dirs (name, out, framework_version)

        ''' Symlinks created here really should have a corresponding bld.symlink_as
            during installation '''

        join = os.path.join
        # create a link from framework binary to framework root path
        linktarget = dir.find_or_declare(out.name).abspath()
        if os.path.lexists (linktarget):
            os.unlink (linktarget)

        linksource = join (join ("Versions", framework_version), out.name)
        os.symlink (linksource, linktarget)


@feature ('cshlib', 'cxxshlib')
@after_method ('apply_link')
def create_task_fwkplist (self):
    if getattr (self, 'mac_framework', False):
        out = self.link_task.outputs[0]
        version = getattr (self, "framework_version", 'A')
        name = c_osx.bundle_name_for_output (out, ".framework")

        dir = self.create_framework_dirs (name, out, version)
        tgt = dir.find_or_declare (['Versions', version, 'Resources', 'Info.plist'])
        self.plisttask = plisttask = self.create_task ('macplist', [], tgt)

        if getattr (self, 'mac_plist', False):
            node = self.path.find_resource (self.mac_plist)
            if node:
                plisttask.inputs.append (node)
            else:
                plisttask.code = self.mac_plist
        else:
            bundle_id = getattr (self, 'mac_bundle_identifier', 'com.example.%s' % out.name)
            bundle_cpy = getattr (self, 'mac_bundle_copyright', 'Copyright 2013 %s' % out.name)
            plisttask.code = framework_info % (out.name, bundle_id, bundle_cpy)

        default_framework_dir = self.framework_dir()
        inst_to = default_framework_dir + '/%s/Versions/%s/Resources' % (name, version)
        self.bld.install_files (inst_to, tgt)
