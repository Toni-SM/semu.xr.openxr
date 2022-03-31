## Building form source

### Linux

Install the following packages or dependencies. Change the `OV_APP` variable to the name of the Omniverse app you want to build for

```bash
cd src/omni.add_on.openxr/xr
sudo apt install libx11-dev
```

```bash
~/.local/share/ov/pkg/OV_APP/kit/python/bin/python3 -m pip install pybind11
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
