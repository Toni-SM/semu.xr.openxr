#!/bin/bash

# delete old files
. clean_compiled_files.bash

# compile code
python pybind11_ext.py build_ext --inplace

# copy compiled file
cp xrlib_p* ../bin/xrlib_p.so
