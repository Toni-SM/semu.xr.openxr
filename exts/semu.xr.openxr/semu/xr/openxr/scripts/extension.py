import gc
import carb
import omni.ext
try:
    import cv2
except:
    omni.kit.pipapi.install("opencv-python")
try:
    from .. import _openxr as _openxr
except:
    print(">>>> [DEVELOPMENT] import openxr")
    from .. import openxr as _openxr

__all__ = ["Extension", "_openxr"]


class Extension(omni.ext.IExt):
    def on_startup(self, ext_id):
        # get extension settings
        self._settings = carb.settings.get_settings()
        disable_openxr = self._settings.get("/exts/semu.xr.openxr/disable_openxr")

        self._xr = _openxr.acquire_openxr_interface(disable_openxr=disable_openxr)

    def on_shutdown(self):
        _openxr.release_openxr_interface(self._xr)
        gc.collect()
