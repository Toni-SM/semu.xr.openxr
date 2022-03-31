#!/bin/bash

# delete old files
. clean_compiled_files.bash

# compile code
~/.local/share/ov/pkg/code-2022.1.0/kit/python/bin/python3 pybind11_ext.py build_ext --inplace

# copy compiled file
cp xrlib_p* ../bin/xrlib_p.so
