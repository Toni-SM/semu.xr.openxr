#!/bin/bash

# delete old files
. clean_compiled_files.bash
rm ../bin/xrlib_p*

# compile code (Python MUST be that used by Omniverse)
python pybind11_ext.py build_ext --inplace

# copy compiled file
cp xrlib_p* ../bin/xrlib_p.so
