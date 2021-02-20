from subprocess import call
import sys

MSVC_PROJECTS = [
    'tools\jucer\Standalone\Builds\VisualStudio2019\Element.sln',
    'tools\jucer\Element\Builds\VisualStudio2019\Element.sln',
    'tools\jucer\ElementFX\Builds\VisualStudio2019\Element FX.sln'
]

XCODE_PROJECTS = [
    'tools/jucer/Element/Builds/MacOSX/Element.xcodeproj'
    'tools/jucer/ElementFX/Builds/MacOSX/Element FX.xcodeproj'
    'tools/jucer/ElementMFX/Builds/MacOSX/Element MFX.xcodeproj'
    'tools/jucer/Standalone/Builds/MacOSX/Element.xcodeproj'
]

if 'win' in sys.platform:
    for project in MSVC_PROJECTS:
        call (['msbuild', '/t:Clean', '/p:Configuration=Release', '/p:Platform=x64', project])
elif 'darwin' in sys.platform:
    for project in XCODE_PROJECTS:
        raise RuntimeError ("Implement Xcode cleaning")
