## Building form source

### Linux

Install the following packages or dependencies

```bash
sudo apt install libx11-dev
```

Setup a python environment. Change the `OV_APP` variable to the name of the Omniverse app you want to build for

```bash
cd src/omni.add_on.openxr/xr
~/.local/share/ov/pkg/OV_APP/kit/python/bin/python3 -m venv env

# source the env
source env/bin/activate

# install required packages
python -m pip install --upgrade pip
python -m pip install pybind11
python -m pip install Cython
```

#### Build CTYPES-based library

```bash
cd src/omni.add_on.openxr/xr
bash compile_ctypes.bash
```

#### Build PYBIND11-based library

```bash
cd src/omni.add_on.openxr/xr
bash compile_pybind11.bash
```
