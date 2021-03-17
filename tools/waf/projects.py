import os, subprocess, sys

PROJUCER_PROJECTS = '''
    tools/jucer/ElementFX/ElementFX.jucer
    tools/jucer/ElementMFX/ElementMFX.jucer
    tools/jucer/Standalone/Element.jucer
    tools/jucer/Element/Element.jucer'''.split()

def projucer_fallback():
    if 'win32' in sys.platform or 'win64' in sys.platform:
        return ["C:\\SDKs\\JUCE\\Projucer.exe"]
    elif 'darwin' in sys.platform:
        return ["/Applications/Projucer.app/Contents/MacOS/Projucer"]
    else:
        return ['Projucer']

def projucer (ctx):
    if ctx and isinstance (ctx.env.PROJUCER, list):
        return ctx.env.PROJUCER
    return projucer_fallback()

def resave (ctx = None):
    # Resave all projects
    exe = projucer (ctx)
    exe.append ('--resave')
    call = subprocess.call
    devnull = open (os.devnull, 'w')

    print ("Resaving Projucer projects")
    for project in PROJUCER_PROJECTS:
        print (os.path.basename (project))
        subprocess.call (exe + [project],
            stdout=devnull, stderr=subprocess.STDOUT)
    call (['bash', 'tools/copybin.sh'],
        stdout=devnull, stderr=subprocess.STDOUT)
    devnull.close()
    print ("Done!")

# from waflib import Task, TaskGen
# from waflib.TaskGen import feature, after_method

# @TaskGen.extension ('.jucer')
# @feature ('projucer')
# @after_method ('process_source')
# def doit (self):
#     print (self.sources)

# def process (self, node):
#     tsk = self.create_task ('abcd')
#     print (tsk)
#     return tsk

# class abcd(Task.Task):
#     def run(self):
#         return 0

# def makeit():
#     ctx (
#         features='projucer',
#         sources=ctx.path.ant_glob ('tools/jucer/**/*.jucer'),
#         target='dumdum'
#     )
