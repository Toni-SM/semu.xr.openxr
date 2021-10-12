#!/bin/bash

export LIBRARY_PATH=$(pwd)/../../kit/python/include

# delete old data
rm -r build
rm _openxr*.so
rm omni/add_on/openxr/*.so
rm omni/add_on/openxr/*.c

# compile code
# ../../kit/python/bin/python3 compile.py build_ext --inplace
python compile.py build_ext --inplace

# # move compiled file
mv _openxr.cpython-37m-x86_64-linux-gnu.so omni/add_on/openxr/

# delete temporal data
rm -r build
rm omni/add_on/openxr/*.c
