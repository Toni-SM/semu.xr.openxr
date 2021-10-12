#!/bin/bash

# delete old files
rm -r build
rm xrlib*

# compile code
python pybind11_compile.py build_ext --inplace

# copy compiled file
cp xrlib_p.cpython-37m-x86_64-linux-gnu.so ../bin/xrlib_p.so
