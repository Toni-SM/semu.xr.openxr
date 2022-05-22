#!/bin/bash

extension_dir=../../exts/semu.xr.openxr
extension_tree=semu/xr/openxr

# delete old files
rm -r $extension_dir

mkdir -p $extension_dir/$extension_tree
mkdir -p $extension_dir/$extension_tree/scripts
mkdir -p $extension_dir/$extension_tree/tests

cp -r config $extension_dir
cp -r data $extension_dir
cp -r docs $extension_dir

# scripts folder
cp $extension_tree/scripts/extension.py $extension_dir/$extension_tree/scripts

# tests folder
cp $extension_tree/tests/__init__.py $extension_dir/$extension_tree/tests
cp $extension_tree/tests/test_openxr.py $extension_dir/$extension_tree/tests

# single files
cp $extension_tree/__init__.py $extension_dir/$extension_tree/
cp $extension_tree/*.so $extension_dir/$extension_tree/
