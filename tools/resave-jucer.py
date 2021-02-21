from subprocess import call
import sys

PROJUCER_PROJECTS = '''
    tools/jucer/Standalone/Element.jucer
    tools/jucer/Standalone/Element.jucer
    tools/jucer/Standalone/Element.jucer
    tools/jucer/Standalone/Element.jucer'''.split()

def projucer_exe():
    if 'win32' in sys.platform or 'win64' in sys.platform:
        return "C:\\SDKs\\JUCE\\Projucer.exe"
    elif 'darwin' in sys.platform:
        return "/Applications/Projucer.app/Contents/MacOS/Projucer"
    else:
        return 'Projucer'

for project in PROJUCER_PROJECTS:
    call ([projucer_exe(), '--resave', project])
