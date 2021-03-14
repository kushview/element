import os, stat
import element

def process (infile, **keys):
    # process a single template
    if not os.path.exists (infile):
        raise Exception ("Template does not exist: %s" % infile)
    if not infile.endswith (".in"):
        raise Exception ("Timplate is not an *.in file")

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

def process_info_plist():
    plist = process ('data/Info.plist.in',
        PACKAGE_VERSION     = element.VERSION)

def process_linuxdeploy_sh():
    # generate linuxdeploy.sh
    script = process ('tools/linuxdeploy.sh.in',
        PACKAGE_VERSION     = element.VERSION)

    st = os.stat (script)
    os.chmod (script, st.st_mode | stat.S_IEXEC)

def generate():
    # generate all templates
    process_linuxdeploy_sh()
    process_info_plist()
