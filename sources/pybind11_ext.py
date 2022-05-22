import os
import sys
from distutils.core import setup

from pybind11.setup_helpers import Pybind11Extension, build_ext

# OV python (kit\python\include)
if sys.platform == 'win32':
    python_library_dir = os.path.join(os.path.dirname(sys.executable), "include")
elif sys.platform == 'linux':
    python_library_dir = os.path.join(os.path.dirname(sys.executable), "..", "lib")

if not os.path.exists(python_library_dir):
    raise Exception("OV Python library directory not found: {}".format(python_library_dir))


ext_modules = [
    Pybind11Extension("xrlib_p",
                      ["pybind11_wrapper.cpp"],
                      include_dirs=[os.path.join(os.getcwd(), "thirdparty", "openxr", "include"),
                                    os.path.join(os.getcwd(), "thirdparty", "opengl", "include"),
                                    os.path.join(os.getcwd(), "thirdparty", "sdl2")],
                      library_dirs=[os.path.join(os.getcwd(), "thirdparty", "openxr", "lib"),
                                    os.path.join(os.getcwd(), "thirdparty", "opengl", "lib"),
                                    os.path.join(os.getcwd(), "thirdparty", "sdl2", "lib"),
                                    python_library_dir],
                      libraries=["openxr_loader", "GL", "SDL2"],
                      extra_link_args=["-Wl,-rpath=./bin"],
                      undef_macros=["CTYPES", "APPLICATION"]),
]

setup(
    name = 'openxr-lib',
    cmdclass = {'build_ext': build_ext},
    ext_modules = ext_modules
)
