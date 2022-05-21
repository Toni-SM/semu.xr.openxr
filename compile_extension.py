import os
import sys
from distutils.core import setup
from distutils.extension import Extension

from Cython.Distutils import build_ext

# OV python (kit\python\include)
if sys.platform == 'win32':
    python_library_dir = os.path.join(os.path.dirname(sys.executable), "include")
elif sys.platform == 'linux':
    python_library_dir = os.path.join(os.path.dirname(sys.executable), "..", "include")

if not os.path.exists(python_library_dir):
    raise Exception("OV Python library directory not found: {}".format(python_library_dir))

ext_modules = [
    Extension("_openxr",
              [os.path.join("semu", "xr", "openxr", "openxr.py")],
              library_dirs=[python_library_dir]),
]

for ext in ext_modules:
    ext.cython_directives = {'language_level': "3"}

setup(
    name = 'semu.xr.openxr',
    cmdclass = {'build_ext': build_ext},
    ext_modules = ext_modules
)
