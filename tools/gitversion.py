import os, string
from subprocess import Popen, PIPE

# Generates build/include/GitVersion.h
# must be run from the top level source dir

template_file_data = \
'''// Generated file
#pragma once
#define EL_GIT_VERSION "XXXX"
'''

if os.path.exists('.git'):
    process = Popen(["git", "rev-parse", "--short", "HEAD"], stdout=PIPE)
    (githash, err) = process.communicate()
    githash = githash.strip()
    exit_code = process.wait()

    process = Popen(["bash", "tools/nchanges.sh"], stdout=PIPE)
    (nchanges, err) = process.communicate()
    process.wait()
    nchanges = string.atoi(nchanges.strip())

    out = githash
    if nchanges > 0: out = out + '-dirty'
    if not os.path.exists("build/include"):
        os.mkdir("build/include")
    hf = open("build/include/GitVersion.h", "w")
    hf.write (template_file_data.replace ("XXXX", out.strip()))
    hf.close()
