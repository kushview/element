from subprocess import call, Popen, PIPE
import os

def short_hash (ctx):
    githash = ''
    if os.path.exists('.git'):
        process = Popen(["git", "rev-parse", "--short", "HEAD"], stdout=PIPE)
        (githash, err) = process.communicate()
        githash = githash.strip()
        process.wait()
        githash = str(githash)
    return githash

def nchanges (ctx):
    if not ctx.env.HAVE_GIT:
        return 0
    proc = Popen ('git status --porcelain --untracked-files=no'.split(), stdout=PIPE)
    (out, err) = proc.communicate()
    result = len (out.splitlines())
    proc.wait()
    return result
