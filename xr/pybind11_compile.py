import os
from distutils.core import setup

from pybind11.setup_helpers import Pybind11Extension, build_ext


ext_modules = [
    Pybind11Extension("xrlib_p",
                      ["pybind11_wrapper.cpp"],
                      include_dirs=[os.path.join(os.getcwd(), "thirdparty", "openxr-1.0.19", "include"),
                                    os.path.join(os.getcwd(), "thirdparty", "opengl", "include")],
                      library_dirs=[os.path.join(os.getcwd(), "thirdparty", "openxr-1.0.19", "lib"),
                                    os.path.join(os.getcwd(), "thirdparty", "opengl", "lib"),
                                    # change according to Isaac Sim's python version
                                    os.path.join(os.getcwd(), "..", "..", "..", "kit", "python", "lib")],
                      libraries=["openxr_loader", "GL", "SDL2"],
                      extra_link_args=["-Wl,-rpath=./bin"],
                      undef_macros=["CTYPES", "APPLICATION"]),
]

setup(
    name = 'openxr-lib',
    cmdclass = {'build_ext': build_ext},
    ext_modules = ext_modules
)
