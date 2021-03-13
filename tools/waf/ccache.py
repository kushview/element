from waflib.Configure import conf

@conf 
def check_ccache (self):
    return self.find_program ('ccache', uselib_store='CCACHE', mandatory=False)

def options (self):
    self.add_option ('--enable-ccache', default=False, action='store_true', dest='use_ccache', \
        help="Use ccache if possible [ Default: disabled ]")

def configure (self):
    if not self.options.use_ccache:
        return
    self.check_ccache()
    if isinstance (self.env.CCACHE, list) and len (self.env.CCACHE) > 0:
        self.env.CC   = self.env.CCACHE + self.env.CC
        self.env.CXX  = self.env.CCACHE + self.env.CXX
