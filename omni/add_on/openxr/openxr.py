import os
import sys
import ctypes
import numpy as np


def acquire_openxr_interface(use_ctypes: bool = False, graphics: str = "OpenGL"):
    return OpenXR(use_ctypes=use_ctypes, graphics=graphics)

def release_openxr_interface(xr):
    print("TODO: _openxr.release_openxr_interface")
    pass


XrStructureType = ctypes.c_int

class XrQuaternionf(ctypes.Structure):
    _fields_ = [('x', ctypes.c_float), ('y', ctypes.c_float), ('z', ctypes.c_float), ('w', ctypes.c_float)]

class XrVector3f(ctypes.Structure):
    _fields_ = [('x', ctypes.c_float), ('y', ctypes.c_float), ('z', ctypes.c_float)]

class XrPosef(ctypes.Structure):
    _fields_ = [('orientation', XrQuaternionf), ('position', XrVector3f)]

class XrFovf(ctypes.Structure):
    _fields_ = _fields_ = [('angleLeft', ctypes.c_float), ('angleRight', ctypes.c_float), ('angleUp', ctypes.c_float), ('angleDown', ctypes.c_float)]

class XrView(ctypes.Structure):
    _fields_ = [('type', XrStructureType), ('next', ctypes.c_void_p), ('pose', XrPosef), ('fov', XrFovf)]

class XrViewConfigurationView(ctypes.Structure):
    _fields_ = [('type', XrStructureType), ('next', ctypes.c_void_p), 
                ('recommendedImageRectWidth', ctypes.c_uint32), ('maxImageRectWidth', ctypes.c_uint32), 
                ('recommendedImageRectHeight', ctypes.c_uint32), ('maxImageRectHeight', ctypes.c_uint32), 
                ('recommendedSwapchainSampleCount', ctypes.c_uint32), ('maxSwapchainSampleCount', ctypes.c_uint32)]


class OpenXR:
    def __init__(self, use_ctypes=False, graphics="OpenGL") -> None:
        self._lib = None
        self._app = None

        self._use_ctypes = use_ctypes
        
        self._frame_left = None
        self._frame_right = None
        self._callback_render_views = None

        # constants
        self.XR_KHR_OPENGL_ENABLE_EXTENSION_NAME = "XR_KHR_opengl_enable"
        self.XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME = "XR_KHR_opengl_es_enable"
        self.XR_KHR_VULKAN_ENABLE_EXTENSION_NAME = "XR_KHR_vulkan_enable"
        self.XR_KHR_D3D11_ENABLE_EXTENSION_NAME = "XR_KHR_D3D11_enable"
        self.XR_KHR_D3D12_ENABLE_EXTENSION_NAME = "XR_KHR_D3D12_enable"

        self.XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY = 1
        self.XR_FORM_FACTOR_HANDHELD_DISPLAY = 2

        self.XR_ENVIRONMENT_BLEND_MODE_OPAQUE = 1
        self.XR_ENVIRONMENT_BLEND_MODE_ADDITIVE = 2
        self.XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND = 3

        self.XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO = 1
        self.XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO = 2

        # graphics API
        if graphics in ["OpenGL", self.XR_KHR_OPENGL_ENABLE_EXTENSION_NAME]:
            self._graphics = self.XR_KHR_OPENGL_ENABLE_EXTENSION_NAME
        elif graphics in ["OpenGLES", self.XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME]:
            self._graphics = self.XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME
            raise NotImplementedError("OpenGLES graphics API is not implemented yet")
        elif graphics in ["Vulkan", self.XR_KHR_VULKAN_ENABLE_EXTENSION_NAME]:
            self._graphics = self.XR_KHR_VULKAN_ENABLE_EXTENSION_NAME
            raise NotImplementedError("Vulkan graphics API is not implemented yet")
        elif graphics in ["D3D11", self.XR_KHR_D3D11_ENABLE_EXTENSION_NAME]:
            self._graphics = self.XR_KHR_D3D11_ENABLE_EXTENSION_NAME
            raise NotImplementedError("D3D11 graphics API is not implemented yet")
        elif graphics in ["D3D12", self.XR_KHR_D3D12_ENABLE_EXTENSION_NAME]:
            self._graphics = self.XR_KHR_D3D12_ENABLE_EXTENSION_NAME
            raise NotImplementedError("D3D12 graphics API is not implemented yet")
        else:
            raise ValueError("Invalid graphics API ({}). Valid graphics APIs are OpenGL, OpenGLES, Vulkan, D3D11, D3D12".format(graphics))
        
    def init(self) -> bool:
        """
        Init OpenXR by loading the compiled library
        """
        # TODO: catch exceptions
        if __name__ == "__main__":
            extension_path = os.getcwd()[:os.getcwd().find("/omni/add_on/openxr")]
        else:
            extension_path = __file__[:__file__.find("/omni/add_on/openxr")]
        # ctypes
        if self._use_ctypes:
            ctypes.PyDLL(os.path.join(extension_path, "bin", "libGL.so"), mode = ctypes.RTLD_GLOBAL)
            ctypes.PyDLL(os.path.join(extension_path, "bin", "libSDL2.so"), mode = ctypes.RTLD_GLOBAL)
            ctypes.PyDLL(os.path.join(extension_path, "bin", "libopenxr_loader.so"), mode = ctypes.RTLD_GLOBAL)
            
            self._lib = ctypes.PyDLL(os.path.join(extension_path, "bin", "xrlib_c.so"), mode = ctypes.RTLD_GLOBAL)
            self._app = self._lib.openXrApplication()
            print("OpenXR.init", self._lib)
        # pybind11
        else:
            sys.setdlopenflags(os.RTLD_GLOBAL | os.RTLD_LAZY)
            sys.path.append(os.path.join(extension_path, "bin"))
            # change cwd
            tmp_dir= os.getcwd()
            os.chdir(extension_path)
            #import library
            import xrlib_p
            #restore cwd
            os.chdir(tmp_dir)

            self._lib = xrlib_p
            self._app = xrlib_p.OpenXrApplication()
            print("OpenXR.init", xrlib_p)
        return True

    def is_session_running(self) -> bool:
        if self._use_ctypes:
            return bool(self._lib.isSessionRunning(self._app))
        else:
            return self._app.isSessionRunning()

    def create_instance(self, application_name: str = "Omniverse (VR)", 
                              engine_name: str = "OpenXR Engine", 
                              api_layers: list = [], 
                              extensions: list = []) -> bool:
        extensions.append(self._graphics)
        
        if self._use_ctypes:
            # format API layes
            requested_api_layers = (ctypes.c_char_p * len(api_layers))()
            requested_api_layers[:] = [layer.encode('utf-8') for layer in api_layers]

            # format extensions
            requested_extensions = (ctypes.c_char_p * len(extensions))()
            requested_extensions[:] = [extension.encode('utf-8') for extension in extensions]

            return bool(self._lib.createInstance(self._app, 
                                                ctypes.create_string_buffer(application_name.encode('utf-8')),
                                                ctypes.create_string_buffer(engine_name.encode('utf-8')),
                                                requested_api_layers,
                                                len(api_layers),
                                                requested_extensions,
                                                len(extensions)))
        else:
            return self._app.createInstance(application_name, engine_name, api_layers, extensions)

    def get_system(self, form_factor: int = 1, blend_mode: int = 1, view_configuration_type: int = 2) -> bool:
        # check form_factor
        if not form_factor in [self.XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY, self.XR_FORM_FACTOR_HANDHELD_DISPLAY]:
            raise ValueError("Invalid form factor ({}). Valid form factors are XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY ({}), XR_FORM_FACTOR_HANDHELD_DISPLAY ({})" \
                             .format(form_factor, self.XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY, self.XR_FORM_FACTOR_HANDHELD_DISPLAY))
        # check blend_mode
        if not blend_mode in [self.XR_ENVIRONMENT_BLEND_MODE_OPAQUE, self.XR_ENVIRONMENT_BLEND_MODE_ADDITIVE, self.XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND]:
            raise ValueError("Invalid blend mode ({}). Valid blend modes are XR_ENVIRONMENT_BLEND_MODE_OPAQUE ({}), XR_ENVIRONMENT_BLEND_MODE_ADDITIVE ({}), XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND ({})" \
                             .format(blend_mode, self.XR_ENVIRONMENT_BLEND_MODE_OPAQUE, self.XR_ENVIRONMENT_BLEND_MODE_ADDITIVE, self.XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND))
        # check view_configuration_type
        if not view_configuration_type in [self.XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO, self.XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO]:
            raise ValueError("Invalid view configuration type ({}). Valid view configuration types are XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO ({}), XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO ({})" \
                             .format(view_configuration_type, self.XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO, self.XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO))

        if self._use_ctypes:
            return bool(self._lib.getSystem(self._app, form_factor, blend_mode, view_configuration_type))
        else:
            return self._app.getSystem(form_factor, blend_mode, view_configuration_type)

    def create_action_set(self) -> bool:
        if self._use_ctypes:
            return bool(self._lib.createActionSet(self._app))
        else:
            return self._app.createActionSet()

    def create_session(self) -> bool:
        if self._use_ctypes:
            return bool(self._lib.createSession(self._app))
        else:
            return self._app.createSession()

    def poll_events(self) -> bool:
        if self._use_ctypes:
            exit_loop = ctypes.c_bool(False)
            result = bool(self._lib.pollEvents(self._app, ctypes.byref(exit_loop)))
            return result and not exit_loop.value
        else:
            result = self._app.pollEvents()
            return result[0] and not result[1]

    def poll_actions(self) -> bool:
        if self._use_ctypes:
            return bool(self._lib.pollActions(self._app))
        else:
            return self._app.pollActions()

    def render_views(self) -> bool:
        if self._use_ctypes:
            return bool(self._lib.renderViews(self._app))
        else:
            return self._app.renderViews()

    def subscribe_render_event(self, callback=None):
        def _internal_callback(num_views, views, configuration_views):
            pass
        
        if self._use_ctypes:
            if callback is None:
                callback = _internal_callback
            self._callback_render_event = ctypes.CFUNCTYPE(None, ctypes.c_int, ctypes.POINTER(XrView), ctypes.POINTER(XrViewConfigurationView))(callback)
            self._lib.setRenderCallback(self._app, self._callback_render_event)
        else:
            self._callback_render_event = callback
            self._app.setRenderCallbackFunction(self._callback_render_event)

    def set_frames(self, configuration_views: list, left: np.ndarray, right: np.ndarray = None) -> bool:
        if self._use_ctypes:
            self._frame_left = self._crop(configuration_views[0], left)
            if right is None:
                return bool(self._lib.setFrames(self._app, 
                                                self._frame_left.shape[1], self._frame_left.shape[0], self._frame_left.ctypes.data_as(ctypes.c_void_p),
                                                0, 0, None))
            else:
                self._frame_right = self._crop(configuration_views[1], right)
                return bool(self._lib.setFrames(self._app, 
                                                self._frame_left.shape[1], self._frame_left.shape[0], self._frame_left.ctypes.data_as(ctypes.c_void_p),
                                                self._frame_right.shape[1], self._frame_right.shape[0], self._frame_right.ctypes.data_as(ctypes.c_void_p)))
        else:
            self._frame_left = self._crop(configuration_views[0], left)
            if right is None:
                return self._app.setFrames(self._frame_left, np.array(None))
            else:
                self._frame_right = self._crop(configuration_views[1], right)
                return self._app.setFrames(self._frame_left, self._frame_right)

    def _crop(self, configuration_view: XrViewConfigurationView, frame: np.ndarray) -> np.ndarray:
        current_ratio = frame.shape[1] / frame.shape[0]
        if self._use_ctypes:
            recommended_ratio = configuration_view.recommendedImageRectWidth / configuration_view.recommendedImageRectHeight
            recommended_size = (configuration_view.recommendedImageRectWidth, configuration_view.recommendedImageRectHeight)
        else:
            recommended_ratio = configuration_view["recommendedImageRectWidth"] / configuration_view["recommendedImageRectHeight"]
            recommended_size = (configuration_view["recommendedImageRectWidth"], configuration_view["recommendedImageRectHeight"])
        if abs(current_ratio - recommended_ratio) > 0.05:
            if current_ratio > recommended_ratio:
                m = int(abs(recommended_ratio * frame.shape[0] - frame.shape[1]) / 2)
                return cv2.resize(frame[:, m:-m], recommended_size)
            else:
                m = int(abs(frame.shape[1] / recommended_ratio - frame.shape[0]) / 2)
                return cv2.resize(frame[m:-m], recommended_size)
        return frame


if __name__ == "__main__":
    import cv2
    import time
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('--ctypes', default=False, action="store_true", help='use ctypes instead of pybind11')
    args = parser.parse_args()

    _xr = acquire_openxr_interface(args.ctypes)
    _xr.init()

    ready = False
    end = False

    if _xr.create_instance():
        if _xr.get_system():
            if _xr.create_action_set():
                if _xr.create_session():
                    ready = True
                else:
                    print("[ERROR]:", "createSession")
            else:
                print("[ERROR]:", "createActionSet")
        else:
            print("[ERROR]:", "getSystem")
    else:
        print("[ERROR]:", "createInstance")

    if ready:
        cap = cv2.VideoCapture("/home/argus/Videos/xr/xr/sample.mp4")

        def callback_render(num_views, views, configuration_views):
            global end
            ret, frame = cap.read()
            if ret:
                if num_views == 2:
                    frame1 = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                    _xr.set_frames(configuration_views, frame, frame1)

                    # show frame
                    k = 0.25
                    frame = cv2.resize(np.hstack((frame, frame1)), (int(2*k*frame.shape[1]), int(k*frame.shape[0])))
                    cv2.imshow('frame', frame)
                    if cv2.waitKey(1) & 0xFF == ord('q'):
                        exit()
            else:
                end = True

        _xr.subscribe_render_event(callback_render)

        while(cap.isOpened() or not end):
            if _xr.poll_events():
                if _xr.is_session_running():
                    if not _xr.poll_actions():
                        print("[ERROR]:", "pollActions")
                        break
                    # _xr._init_callbacks()
                    if not _xr.render_views():
                        print("[ERROR]:", "renderViews")
                        break
                else:
                    time.sleep(0.1)
            else:
                break

    print("END")