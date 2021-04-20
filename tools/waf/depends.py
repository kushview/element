import os

def options (self):
    self.add_option ('--depends', default='', dest='depends', type='string', help='Where dependency tools and libraries are located')

def configure (self):
    self.env.DEPENDSDIR = self.options.depends
    if os.path.exists (self.env.DEPENDSDIR):
        self.env.append_unique ('CPPFLAGS', ['-I%s/include' % self.env.DEPENDSDIR])
        self.env.append_unique ('LINKFLAGS', ['-L%s/lib' % self.env.DEPENDSDIR])
