from waflib.Configure import conf
import os, stat
import element

def inplace (infile, **keys):
    """
    process a single template in place
    """
    if not os.path.exists (infile):
        raise Exception ("Template does not exist: %s" % infile)
    if not infile.endswith (".in"):
        raise Exception ("Template is not an *.in file")

    f = open (infile, 'r')
    txt = f.read()
    f.close()
    
    for k, v in keys.items():
        txt = txt.replace ("@%s@" % k, v)
    
    outfile = infile.replace ('.in', '')
    f = open (outfile, 'w')
    f.write (txt)
    f.close()

    return outfile

@conf
def template (self, *k, **kw):
    kw['features'] = 'subst'
    kw['target'] = []
    T = self(*k, **kw)
    for f in T.source:
        T.target.append (f.relpath().replace ('.in', ''))
    return T
