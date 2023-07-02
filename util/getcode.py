#!/usr/bin/env python
# encoding: utf-8
# Michael Fisher, 2023

from optparse import OptionParser

def options():
    parser = OptionParser()
    parser.add_option ("--version", type="string", dest="version")
    parser.add_option ("--code", type="string", dest="code")
    parser.add_option ("--integer", action="store_true", default=False, dest="integer")
    (opts, _) = parser.parse_args()
    return opts

opts = options()

if (opts.version != None and len(opts.version) > 0):
    version = opts.version.split (".")
    value = ((int(version[0]) << 16) +
             (int(version[1]) << 8) +
             int(version[2]))
    if opts.integer: print (value)
    else: print("0x%x" % value)
elif (opts.code != None and len(opts.code) > 0):
    code = opts.code
    value = ((ord(code[0]) << 24) + (ord(code[1]) << 16) + (ord(code[2]) << 8)  + (ord(code[3])))
    if opts.integer: print (value)
    else: print("0x%x" % value)
else:
    exit(1)

exit (0)
