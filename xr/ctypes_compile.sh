#!/bin/bash

# delete old files
rm -r build
rm xrlib*
rm xrapp

# set variables
export OPENXR_DIR=$(pwd)"/thirdparty/openxr-1.0.19"
export VULKAN_DIR=$(pwd)"/thirdparty/vulkan-1.0.49.0"
export OPENGL_DIR=$(pwd)"/thirdparty/opengl"
export SDL_DIR=$(pwd)"/thirdparty/sdl2"

export DFLAGS="-DAPPLICATION -DCTYPES"
export CFLAGS="-std=c++17 -pthread -O2 -fpermissive -Wwrite-strings"
export INCFLAGS="-I$VULKAN_DIR/include -I$OPENGL_DIR/include -I$OPENXR_DIR/include -I$SDL_DIR"
export LIBFLAGS="-L$VULKAN_DIR/lib/linux64 -L$OPENGL_DIR/lib -L$OPENXR_DIR/lib -L$SDL_DIR/lib"
export LDFLAGS="-lopenxr_loader -lGL -lSDL2 -lX11"      # -lvulkan -lSDL2_image -ldl

# # executable
# g++ $DFLAGS $CFLAGS $INCFLAGS -o xrapp $CPPFILES xr.cpp $LIBFLAGS $LDFLAGS

# object file
g++ $DFLAGS $CFLAGS $INCFLAGS -fPIC -c -o xrlib.o xr.cpp $LIBFLAGS $LDFLAGS

# shared library
g++ -shared -Wl,-soname,xrlib_c.so -o xrlib_c.so xrlib.o


# copy compiled file
cp xrlib_c.so ../bin

# delete temporal files
rm xrlib*