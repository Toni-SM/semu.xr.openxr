## Building from source

### Linux

```bash
cd src/semu.xr.openxr
bash compile_extension.bash
```

## Removing old compiled files

Get a fresh clone of the repository and follow the next steps

```bash
# remove compiled files _openxr.cpython-37m-x86_64-linux-gnu.so
git filter-repo --invert-paths --path exts/semu.xr.openxr/semu/xr/openxr/_openxr.cpython-37m-x86_64-linux-gnu.so

# add origin
git remote add origin git@github.com:Toni-SM/semu.xr.openxr.git

# push changes
git push origin --force --all
git push origin --force --tags
```

## Packaging the extension

```bash
cd src/semu.xr.openxr
bash package_extension.bash
```