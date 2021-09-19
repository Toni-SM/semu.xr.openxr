import gc
import omni.ext
try:
    from .. import _openxr as _openxr
except:
    print(">>>> [DEVELOPMENT] import openxr")
    from .. import openxr as _openxr

__all__ = ["Extension", "_openxr"]


class Extension(omni.ext.IExt):
    def on_startup(self, ext_id):
        self._xr = _openxr.acquire_openxr_interface()

    def on_shutdown(self):
        _openxr.release_openxr_interface(self._xr)
        gc.collect()
