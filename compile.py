import os
from distutils.core import setup
from distutils.extension import Extension

from Cython.Distutils import build_ext

isaac_sim_path = os.getcwd()[:os.getcwd().find("/exts/omni/add_on/openxr")]

ext_modules = [
    Extension("_openxr",
              ["omni/add_on/openxr/openxr.py"],
              library_dirs=[os.path.join(isaac_sim_path, "kit", "python", "include")]),
]

for e in ext_modules:
    e.cython_directives = {'language_level': "3"}

setup(
    name = 'omni.add_on.openxr',
    cmdclass = {'build_ext': build_ext},
    ext_modules = ext_modules
)
