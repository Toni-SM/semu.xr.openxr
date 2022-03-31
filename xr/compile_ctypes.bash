#!/bin/bash

# delete old files
. clean_compiled_files.bash

# set variables
export OPENXR_DIR=$(pwd)"/thirdparty/openxr"
export OPENGL_DIR=$(pwd)"/thirdparty/opengl"
export SDL_DIR=$(pwd)"/thirdparty/sdl2"

export DFLAGS="-DAPPLICATION -DCTYPES -DXR_USE_PLATFORM_XLIB -DXR_USE_GRAPHICS_API_OPENGL"
export CFLAGS="-std=c++17 -pthread -O2 -fpermissive -Wwrite-strings"
export INCFLAGS="-I$OPENGL_DIR/include -I$OPENXR_DIR/include -I$SDL_DIR"
export LIBFLAGS="-L$OPENGL_DIR/lib -L$OPENXR_DIR/lib -L$SDL_DIR/lib"
export LDFLAGS="-lopenxr_loader -lGL -lSDL2 -lX11"      # -lvulkan -lSDL2_image -ldl

# generate executable
# g++ $DFLAGS $CFLAGS $INCFLAGS -o xrapp $CPPFILES xr_opengl.cpp xr.cpp $LIBFLAGS $LDFLAGS
# generate object file
g++ $DFLAGS $CFLAGS $INCFLAGS -fPIC -c -o xrlib.o xr.cpp $LIBFLAGS $LDFLAGS
# generate shared library
g++ -shared -Wl,-soname,xrlib_c.so -o xrlib_c.so xrlib.o

# copy compiled file
cp xrlib_c* ../bin

# delete temporal data
rm -r build
rm xrlib*
