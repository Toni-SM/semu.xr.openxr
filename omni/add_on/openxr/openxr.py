from typing import Union, Callable

import os
import sys
import ctypes

import cv2
import numpy as np

if __name__ != "__main__":
    import pxr
    import omni
    from pxr import UsdGeom, Gf, Usd
    from omni.syntheticdata import sensors
else:
    class pxr:
        class Gf:
            Vec3d = None
            Quatd = lambda w,x,y,z: None
        class Usd:
            Prim = None
        class UsdGeom:
            pass
        class Sdf:
            Path = None
    Gf = pxr.Gf


# constants
XR_KHR_OPENGL_ENABLE_EXTENSION_NAME = "XR_KHR_opengl_enable"
XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME = "XR_KHR_opengl_es_enable"
XR_KHR_VULKAN_ENABLE_EXTENSION_NAME = "XR_KHR_vulkan_enable"
XR_KHR_D3D11_ENABLE_EXTENSION_NAME = "XR_KHR_D3D11_enable"
XR_KHR_D3D12_ENABLE_EXTENSION_NAME = "XR_KHR_D3D12_enable"

XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY = 1
XR_FORM_FACTOR_HANDHELD_DISPLAY = 2

XR_ENVIRONMENT_BLEND_MODE_OPAQUE = 1
XR_ENVIRONMENT_BLEND_MODE_ADDITIVE = 2
XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND = 3

XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO = 1
XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO = 2

XR_REFERENCE_SPACE_TYPE_VIEW = 1
XR_REFERENCE_SPACE_TYPE_LOCAL = 2
XR_REFERENCE_SPACE_TYPE_STAGE = 3

XR_ACTION_TYPE_BOOLEAN_INPUT = 1
XR_ACTION_TYPE_FLOAT_INPUT = 2
XR_ACTION_TYPE_VECTOR2F_INPUT = 3
XR_ACTION_TYPE_POSE_INPUT = 4
XR_ACTION_TYPE_VIBRATION_OUTPUT = 100

XR_NO_DURATION = 0
XR_INFINITE_DURATION = 2**32
XR_MIN_HAPTIC_DURATION = -1
XR_FREQUENCY_UNSPECIFIED = 0


def acquire_openxr_interface():
    return OpenXR()

def release_openxr_interface(xr):
    print("TODO: _openxr.release_openxr_interface")
    pass




# structures (ctypes)
XrActionType = ctypes.c_int
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

class ActionState(ctypes.Structure):
    _fields_ = [('type', XrActionType),
                ('path', ctypes.c_char_p),
                ('isActive', ctypes.c_bool),
                ('stateBool', ctypes.c_bool), 
                ('stateFloat', ctypes.c_float), 
                ('stateVectorX', ctypes.c_float), 
                ('stateVectorY', ctypes.c_float)]

class ActionPoseState(ctypes.Structure):
    _fields_ = [('type', XrActionType),
                ('path', ctypes.c_char_p),
                ('isActive', ctypes.c_bool),
                ('pose', XrPosef)]




class OpenXR:
    def __init__(self) -> None:
        self._lib = None
        self._app = None

        self._graphics = None
        self._use_ctypes = False
        
        # views
        self._prim_left = None
        self._prim_right = None
        self._frame_left = None
        self._frame_right = None
        self._viewport_window_left = None
        self._viewport_window_right = None
        self._rectification_quat_left = Gf.Quatd(1, 0, 0, 0)
        self._rectification_quat_right = Gf.Quatd(1, 0, 0, 0)

        self._viewport_interface = None

        self._transform_fit = None
        self._transform_flip = None

        # callbacks
        self._callback_action_events = {}
        self._callback_action_pose_events = {}
        self._callback_render_event = None

    def init(self, graphics: str = "OpenGL", use_ctypes: bool = False) -> bool:
        """
        Init OpenXR application by loading the related libraries

        Parameters
        ----------
        graphics: str
            OpenXR graphics API supported by the runtime (OpenGL, OpenGLES, Vulkan, D3D11, D3D12).
            Note: At the moment only OpenGL is available
        use_ctypes: bool, optional
            If true, use ctypes as C/C++ interface instead of pybind11 (default)

        Returns
        -------
        bool
            True if initialization was successful, otherwise False
        """
        # TODO: what about no graphic API (only controllers for example)?
        self._use_ctypes = use_ctypes
        # graphics API
        if graphics in ["OpenGL", XR_KHR_OPENGL_ENABLE_EXTENSION_NAME]:
            self._graphics = XR_KHR_OPENGL_ENABLE_EXTENSION_NAME
        elif graphics in ["OpenGLES", XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME]:
            self._graphics = XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME
            raise NotImplementedError("OpenGLES graphics API is not implemented yet")
        elif graphics in ["Vulkan", XR_KHR_VULKAN_ENABLE_EXTENSION_NAME]:
            self._graphics = XR_KHR_VULKAN_ENABLE_EXTENSION_NAME
            raise NotImplementedError("Vulkan graphics API is not implemented yet")
        elif graphics in ["D3D11", XR_KHR_D3D11_ENABLE_EXTENSION_NAME]:
            self._graphics = XR_KHR_D3D11_ENABLE_EXTENSION_NAME
            raise NotImplementedError("D3D11 graphics API is not implemented yet")
        elif graphics in ["D3D12", XR_KHR_D3D12_ENABLE_EXTENSION_NAME]:
            self._graphics = XR_KHR_D3D12_ENABLE_EXTENSION_NAME
            raise NotImplementedError("D3D12 graphics API is not implemented yet")
        else:
            raise ValueError("Invalid graphics API ({}). Valid graphics APIs are OpenGL, OpenGLES, Vulkan, D3D11, D3D12".format(graphics))

        # libraries path
        if __name__ == "__main__":
            extension_path = os.getcwd()[:os.getcwd().find("/omni/add_on/openxr")]
        else:
            extension_path = __file__[:__file__.find("/omni/add_on/openxr")]
        
        try:
            # ctypes
            if self._use_ctypes:
                ctypes.PyDLL(os.path.join(extension_path, "bin", "libGL.so"), mode = ctypes.RTLD_GLOBAL)
                ctypes.PyDLL(os.path.join(extension_path, "bin", "libSDL2.so"), mode = ctypes.RTLD_GLOBAL)
                ctypes.PyDLL(os.path.join(extension_path, "bin", "libopenxr_loader.so"), mode = ctypes.RTLD_GLOBAL)
                
                self._lib = ctypes.PyDLL(os.path.join(extension_path, "bin", "xrlib_c.so"), mode = ctypes.RTLD_GLOBAL)
                self._app = self._lib.openXrApplication()
                print("[INFO] OpenXR initialized using ctypes interface")
            
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
                print("[INFO] OpenXR initialized using pybind11 interface")
        except Exception as e:
            print("[ERROR] OpenXR initialization:", e)
            return False
        
        try:
            self._viewport_interface = omni.kit.viewport.get_viewport_interface()
        except Exception as e:
            print("[WARNING] omni.kit.viewport.get_viewport_interface:", e)
        return True

    def is_session_running(self) -> bool:
        """
        OpenXR session's running status

        Returns
        -------
        bool
            Return True if the OpenXR session is running, False otherwise
        """
        if self._use_ctypes:
            return bool(self._lib.isSessionRunning(self._app))
        else:
            return self._app.isSessionRunning()

    def create_instance(self, application_name: str = "Omniverse (VR)", engine_name: str = "", api_layers: list = [], extensions: list = []) -> bool:
        """
        Create an OpenXR instance to allow communication with an OpenXR runtime

        OpenXR internal function calls:
        - xrEnumerateApiLayerProperties
        - xrEnumerateInstanceExtensionProperties
        - xrCreateInstance 

        Parameters
        ----------
        application_name: str, optional
            Name of the OpenXR application (default: 'Omniverse (VR)')
        engine_name: str, optional
            Name of the engine (if any) used to create the application (empty by default)
        api_layers: list of str, optional
            [API layers](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#api-layers) to be inserted between the OpenXR application and the runtime implementation.
        extensions: list of str, optional
            [Extensions](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#extensions) to be loaded.
            Note: At the moment only the graphic extensions are configured.
            Note: The graphics API selected during initialization (init) is automatically included in the extensions to be loaded.
        
        Returns
        -------
        bool
            True if the instance has been created successfully, otherwise False
        """
        if self._graphics not in extensions:
            extensions += [self._graphics]
        
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
        """
        Obtain the system represented by a collection of related devices at runtime

        OpenXR internal function calls:
        - xrGetSystem
        - xrGetInstanceProperties
        - xrGetSystemProperties
        - xrEnumerateViewConfigurations
        - xrGetViewConfigurationProperties
        - xrEnumerateViewConfigurationViews
        - xrEnumerateEnvironmentBlendModes
        - xrCreateActionSet (actionSetName: 'actionset', localizedActionSetName: 'localized_actionset')
        
        Parameters
        ----------
        form_factor: {XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY, XR_FORM_FACTOR_HANDHELD_DISPLAY}, optional
            Desired [form factor](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#form_factor_description) from XrFormFactor enum (default: XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY)
        blend_mode: {XR_ENVIRONMENT_BLEND_MODE_OPAQUE, XR_ENVIRONMENT_BLEND_MODE_ADDITIVE, XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND}, optional
            Desired environment [blend mode](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#environment_blend_mode) from XrEnvironmentBlendMode enum (default: XR_ENVIRONMENT_BLEND_MODE_OPAQUE)
        view_configuration_type: {XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO}, optional
            Primary [view configuration] (https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#view_configurations) type from XrViewConfigurationType enum (default: XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO)

        Returns
        -------
        bool
            True if the system has been obtained successfully, otherwise False
        """
        # check form_factor
        if not form_factor in [XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY, XR_FORM_FACTOR_HANDHELD_DISPLAY]:
            raise ValueError("Invalid form factor ({}). Valid form factors are XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY ({}), XR_FORM_FACTOR_HANDHELD_DISPLAY ({})" \
                             .format(form_factor, XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY, XR_FORM_FACTOR_HANDHELD_DISPLAY))
        # check blend_mode
        if not blend_mode in [XR_ENVIRONMENT_BLEND_MODE_OPAQUE, XR_ENVIRONMENT_BLEND_MODE_ADDITIVE, XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND]:
            raise ValueError("Invalid blend mode ({}). Valid blend modes are XR_ENVIRONMENT_BLEND_MODE_OPAQUE ({}), XR_ENVIRONMENT_BLEND_MODE_ADDITIVE ({}), XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND ({})" \
                             .format(blend_mode, XR_ENVIRONMENT_BLEND_MODE_OPAQUE, XR_ENVIRONMENT_BLEND_MODE_ADDITIVE, XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND))
        # check view_configuration_type
        if not view_configuration_type in [XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO]:
            raise ValueError("Invalid view configuration type ({}). Valid view configuration types are XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO ({}), XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO ({})" \
                             .format(view_configuration_type, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO))

        if self._use_ctypes:
            return bool(self._lib.getSystem(self._app, form_factor, blend_mode, view_configuration_type))
        else:
            return self._app.getSystem(form_factor, blend_mode, view_configuration_type)

    def create_session(self) -> bool:
        """
        Create an OpenXR session that represents an application's intention to display XR content

        OpenXR internal function calls:
        - xrCreateSession
        - xrEnumerateReferenceSpaces
        - xrCreateReferenceSpace
        - xrGetReferenceSpaceBoundsRect
        - xrSuggestInteractionProfileBindings
        - xrAttachSessionActionSets
        - xrCreateActionSpace
        - xrEnumerateSwapchainFormats
        - xrCreateSwapchain
        - xrEnumerateSwapchainImages
        
        Returns
        -------
        bool
            True if the session has been created successfully, otherwise False
        """
        if self._use_ctypes:
            return bool(self._lib.createSession(self._app))
        else:
            return self._app.createSession()

    def poll_events(self) -> bool:
        """
        [Event polling](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#event-polling) and processing

        OpenXR internal function calls:
        - xrPollEvent
        - xrBeginSession
        - xrEndSession

        Returns
        -------
        bool
            False if the running session needs to end (due to the user closing or switching the application, etc.), otherwise False
        """
        if self._use_ctypes:
            exit_loop = ctypes.c_bool(False)
            result = bool(self._lib.pollEvents(self._app, ctypes.byref(exit_loop)))
            return result and not exit_loop.value
        else:
            result = self._app.pollEvents()
            return result[0] and not result[1]

    def poll_actions(self) -> bool:
        """
        [Action](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#_action_overview) polling

        OpenXR internal function calls:
        - xrSyncActions
        - xrGetActionStateBoolean
        - xrGetActionStateFloat
        - xrGetActionStateVector2f
        - xrGetActionStatePose

        Returns
        -------
        bool
            True if there is no error during polling, otherwise False
        """
        if self._use_ctypes:
            requested_action_states = (ActionState * len(self._callback_action_events.keys()))()
            result = bool(self._lib.pollActions(self._app, requested_action_states, len(requested_action_states)))

            for state in requested_action_states:
                value = None
                if not state.type:
                    break
                if state.type == XR_ACTION_TYPE_BOOLEAN_INPUT:
                    value = state.stateBool
                elif state.type == XR_ACTION_TYPE_FLOAT_INPUT:
                    value = state.stateFloat
                elif state.type == XR_ACTION_TYPE_VECTOR2F_INPUT:
                    value = (state.stateVectorX, state.stateVectorY)
                elif state.type == XR_ACTION_TYPE_POSE_INPUT:
                    continue
                elif state.type == XR_ACTION_TYPE_VIBRATION_OUTPUT:
                    continue
                self._callback_action_events[state.path.decode("utf-8")](state.path.decode("utf-8"), value)
            return result
        
        else:
            result = self._app.pollActions()

            for state in result[1]:
                value = None
                if state["type"] == XR_ACTION_TYPE_BOOLEAN_INPUT:
                    value = state["stateBool"]
                elif state["type"] == XR_ACTION_TYPE_FLOAT_INPUT:
                    value = state["stateFloat"]
                elif state["type"] == XR_ACTION_TYPE_VECTOR2F_INPUT:
                    value = (state["stateVectorX"], state["stateVectorY"])
                elif state["type"] == XR_ACTION_TYPE_POSE_INPUT:
                    continue
                elif state["type"] == XR_ACTION_TYPE_VIBRATION_OUTPUT:
                    continue
                self._callback_action_events[state["path"]](state["path"], value)
            return result[0]

    def render_views(self, reference_space: int = 2) -> bool:
        """
        Present rendered images to the user's views according to the selected reference space
        
        OpenXR internal function calls:
        - xrWaitFrame
        - xrBeginFrame
        - xrLocateSpace
        - xrLocateViews
        - xrAcquireSwapchainImage
        - xrWaitSwapchainImage
        - xrReleaseSwapchainImage
        - xrEndFrame

        Parameters
        ----------
        reference_space: {XR_REFERENCE_SPACE_TYPE_VIEW, XR_REFERENCE_SPACE_TYPE_LOCAL, XR_REFERENCE_SPACE_TYPE_STAGE}, optional
            Desired [reference space](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#reference-spaces) type from XrReferenceSpaceType enum used to render the images (default: XR_REFERENCE_SPACE_TYPE_LOCAL)

        Returns
        -------
        bool
            True if there is no error during rendering, otherwise False
        """
        if self._callback_render_event is None:
            print("[WARNING] No callback has been established for rendering events. Internal callback will be used")
            self.subscribe_render_event()

        if self._use_ctypes:
            requested_action_pose_states = (ActionPoseState * len(self._callback_action_pose_events.keys()))()
            result =  bool(self._lib.renderViews(self._app, reference_space, requested_action_pose_states, len(requested_action_pose_states)))

            for state in requested_action_pose_states:
                value = None
                if state.type == XR_ACTION_TYPE_POSE_INPUT and state.isActive:
                    value = ((state.pose.position.x, state.pose.position.y, state.pose.position.z),
                             (state.pose.orientation.x, state.pose.orientation.y, state.pose.orientation.z, state.pose.orientation.w))
                    self._callback_action_events[state.path.decode("utf-8")](state.path.decode("utf-8"), value)
            return result

        else:
            result = self._app.renderViews(reference_space)

            for state in result[1]:
                value = None
                if state["type"] == XR_ACTION_TYPE_POSE_INPUT and state["isActive"]:
                    value = ((state["pose"]["position"]["x"], state["pose"]["position"]["y"], state["pose"]["position"]["z"]),
                             (state["pose"]["orientation"]["x"], state["pose"]["orientation"]["y"], state["pose"]["orientation"]["z"], state["pose"]["orientation"]["w"]))
                    self._callback_action_events[state["path"]](state["path"], value)
            return result[0]
    
    # action utilities

    def subscribe_action_event(self, path: str, callback: Union[Callable[[str, object], None], None] = None, action_type: Union[int, None] = None, reference_space: Union[int, None] = 2) -> bool:
        """
        Create an action given a path and subscribe a callback function to the update event of this action

        If action_type is None the action type will be automatically defined by parsing the last segment of the path according to the following policy:
        - XR_ACTION_TYPE_BOOLEAN_INPUT: /click, /touch
        - XR_ACTION_TYPE_FLOAT_INPUT: /value, /force
        - XR_ACTION_TYPE_VECTOR2F_INPUT: /x, /y
        - XR_ACTION_TYPE_POSE_INPUT: /pose
        - XR_ACTION_TYPE_VIBRATION_OUTPUT: /haptic, /haptic_left, /haptic_right, /haptic_left_trigger, /haptic_right_trigger

        The callback function (a callable object) should have only the following 2 parameters:
        - path: str
           The complete path (user path and subpath) of the action that invokes the callback
        - value: bool, float, tuple(float, float), tuple(tuple(float, float, float), tuple(float, float, float, float))
           The current state of the action according to its type
           - XR_ACTION_TYPE_BOOLEAN_INPUT: bool
           - XR_ACTION_TYPE_FLOAT_INPUT: float 
           - XR_ACTION_TYPE_VECTOR2F_INPUT (x, y): tuple(float, float)
           - XR_ACTION_TYPE_POSE_INPUT (cartesian position (x,y,z), rotation as quaternion (x,y,z,w)): tuple(tuple(float, float, float), tuple(float, float, float, float))

        XR_ACTION_TYPE_VIBRATION_OUTPUT actions will not invoke their callback function. In this case the callback must be None
        XR_ACTION_TYPE_POSE_INPUT also specifies, through the definition of the reference_space parameter, the reference space used to retrieve the pose

        OpenXR internal function calls:
        - xrCreateAction
        
        Parameters
        ----------
        path: str
            Complete [path](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#semantic-path-reserved) (user path and subpath) referring to the action
        callback: callable object (2 parameters) or None for XR_ACTION_TYPE_VIBRATION_OUTPUT
            Callback invoked when the state of the action changes
        action_type: {XR_ACTION_TYPE_BOOLEAN_INPUT, XR_ACTION_TYPE_FLOAT_INPUT, XR_ACTION_TYPE_VECTOR2F_INPUT, XR_ACTION_TYPE_POSE_INPUT, XR_ACTION_TYPE_VIBRATION_OUTPUT} or None, optional
            Action [type](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrActionType) from XrActionType enum (default: None)
        reference_space: {XR_REFERENCE_SPACE_TYPE_VIEW, XR_REFERENCE_SPACE_TYPE_LOCAL, XR_REFERENCE_SPACE_TYPE_STAGE}, optional
            Desired [reference space](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#reference-spaces) type from XrReferenceSpaceType enum used to retrieve the pose (default: XR_REFERENCE_SPACE_TYPE_LOCAL)

        Returns
        -------
        bool
            True if there is no error during action creation, otherwise False
        """
        if action_type is None:
            if path.split("/")[-1] in ["click", "touch"]:
                action_type = XR_ACTION_TYPE_BOOLEAN_INPUT
            elif path.split("/")[-1] in ["value", "force"]:
                action_type = XR_ACTION_TYPE_FLOAT_INPUT
            elif path.split("/")[-1] in ["x", "y"]:
                action_type = XR_ACTION_TYPE_VECTOR2F_INPUT
            elif path.split("/")[-1] in ["pose"]:
                action_type = XR_ACTION_TYPE_POSE_INPUT
            elif path.split("/")[-1] in ["haptic", "haptic_left", "haptic_right", "haptic_left_trigger", "haptic_right_trigger"]:
                action_type = XR_ACTION_TYPE_VIBRATION_OUTPUT
            else:
                raise ValueError("The action type cannot be retrieved from the path {}".format(path))
        
        if callback is None and action_type != XR_ACTION_TYPE_VIBRATION_OUTPUT:
            raise ValueError("The callback was not defined")
        self._callback_action_events[path] = callback
        if action_type == XR_ACTION_TYPE_POSE_INPUT:
            self._callback_action_pose_events[path] = callback
        
        if self._use_ctypes:
            return bool(self._lib.addAction(self._app, ctypes.create_string_buffer(path.encode('utf-8')), action_type, reference_space))
        else:
            return self._app.addAction(path, action_type, reference_space)

    def apply_haptic_feedback(self, path: str, haptic_feedback: dict = {}) -> bool:
        """
        Apply a [haptic feedback](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#_output_actions_and_haptics) to a device defined by a path (user path and subpath)

        OpenXR internal function calls:
        - xrApplyHapticFeedback

        Parameters
        ----------
        path: str
            Complete [path](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#semantic-path-reserved) (user path and subpath) referring to the action
        haptic_feedback: dict
            A python dictionary containing the field names and value of a XrHapticBaseHeader-based structure.
            Note: At the moment the only haptics type supported is the unextended OpenXR [XrHapticVibration](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrHapticVibration)

        Returns
        -------
        bool
            True if there is no error during the haptic feedback application, otherwise False
        """
        amplitude = haptic_feedback.get("amplitude", 0.5)
        duration = haptic_feedback.get("duration", XR_MIN_HAPTIC_DURATION)
        frequency = haptic_feedback.get("frequency", XR_FREQUENCY_UNSPECIFIED)
        if self._use_ctypes:
            amplitude = ctypes.c_float(amplitude)
            duration = ctypes.c_int64(duration)
            frequency = ctypes.c_float(frequency)
            return bool(self._lib.applyHapticFeedback(self._app, ctypes.create_string_buffer(path.encode('utf-8')), amplitude, duration, frequency))
        else:
            return self._app.applyHapticFeedback(path, amplitude, duration, frequency)

    def stop_haptic_feedback(self, path: str) -> bool:
        """
        Stop a [haptic feedback](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#_output_actions_and_haptics) applied to a device defined by a path (user path and subpath)

        OpenXR internal function calls:
        - xrStopHapticFeedback

        Parameters
        ----------
        path: str
            Complete [path](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#semantic-path-reserved) (user path and subpath) referring to the action

        Returns
        -------
        bool
            True if there is no error during the haptic feedback stop, otherwise False
        """
        if self._use_ctypes:
            return bool(self._lib.stopHapticFeedback(self._app, ctypes.create_string_buffer(path.encode('utf-8'))))
        else:
            return self._app.stopHapticFeedback(path)

    # view utilities

    def setup_mono_view(self, camera: Union[str, pxr.Sdf.Path, pxr.Usd.Prim] = "/OpenXR/Cameras/camera") -> None:
        """
        Setup Omniverse viewport and camera for monoscopic rendering

        This method obtains the viewport window for the given camera. If the viewport window does not exist, a new one is created and the camera is set as active. If the given camera does not exist, a new camera is created with the same path and set to the recommended resolution of the display device
        
        Parameters
        ----------
        camera: str, pxr.Sdf.Path or pxr.Usd.Prim, optional
            Omniverse camera prim or path (default: '/OpenXR/Cameras/camera')
        """
        self.setup_stereo_view(camera, None)

    def setup_stereo_view(self, left_camera: Union[str, pxr.Sdf.Path, pxr.Usd.Prim] = "/OpenXR/Cameras/left_camera", right_camera: Union[str, pxr.Sdf.Path, pxr.Usd.Prim, None] = "/OpenXR/Cameras/right_camera") -> None:
        """
        Setup Omniverse viewports and cameras for stereoscopic rendering

        This method obtains the viewport window for each camera. If the viewport window does not exist, a new one is created and the camera is set as active. If the given cameras do not exist, new cameras are created with the same path and set to the recommended resolution of the display device
        
        Parameters
        ----------
        left_camera: str, pxr.Sdf.Path or pxr.Usd.Prim, optional
            Omniverse left camera prim or path (default: '/OpenXR/Cameras/left_camera')
        right_camera: str, pxr.Sdf.Path or pxr.Usd.Prim, optional
            Omniverse right camera prim or path (default: '/OpenXR/Cameras/right_camera')
        """
        def get_or_create_vieport_window(camera, teleport=True, window_size=(400, 300), resolution=(1280, 720)):
            window = None
            camera = str(camera.GetPath() if type(camera) is Usd.Prim else camera)
            # get viewport window
            for interface in self._viewport_interface.get_instance_list():
                w = self._viewport_interface.get_viewport_window(interface)
                if camera == w.get_active_camera():
                    window = w
                    # check visibility
                    if not w.is_visible():
                        w.set_visible(True)
                    break
            # create viewport window if not exist
            if window is None:
                window = self._viewport_interface.get_viewport_window(self._viewport_interface.create_instance())
                window.set_window_size(*window_size)
                window.set_active_camera(camera)
                window.set_texture_resolution(*resolution)
                if teleport:
                    window.set_camera_position(camera, 1.0, 1.0, 1.0, True)
                    window.set_camera_target(camera, 0.0, 0.0, 0.0, True)
            return window
        
        stage = omni.usd.get_context().get_stage()

        # left camera
        teleport_camera = False
        self._prim_left = None
        if type(left_camera) is Usd.Prim:
            self._prim_left = left_camera
        elif stage.GetPrimAtPath(left_camera).IsValid():
            self._prim_left = stage.GetPrimAtPath(left_camera)
        else:
            teleport_camera = True
            self._prim_left = stage.DefinePrim(omni.usd.get_stage_next_free_path(stage, left_camera, False), "Camera")
        self._viewport_window_left = get_or_create_vieport_window(self._prim_left, teleport=teleport_camera)

        # right camera
        teleport_camera = False
        self._prim_right = None
        if right_camera is not None:
            if type(right_camera) is Usd.Prim:
                self._prim_right = right_camera
            elif stage.GetPrimAtPath(right_camera).IsValid():
                self._prim_right = stage.GetPrimAtPath(right_camera)
            else:
                teleport_camera = True
                self._prim_right = stage.DefinePrim(omni.usd.get_stage_next_free_path(stage, right_camera, False), "Camera")
            self._viewport_window_right = get_or_create_vieport_window(self._prim_right, teleport=teleport_camera)

        # set recommended resolution
        resolutions = self.get_recommended_resolutions()
        if len(resolutions) and self._viewport_window_left is not None:
            self._viewport_window_left.set_texture_resolution(*resolutions[0])
        if len(resolutions) == 2 and self._viewport_window_right is not None:
            self._viewport_window_right.set_texture_resolution(*resolutions[1])

    def get_recommended_resolutions(self) -> tuple:
        """
        Get the recommended resolution of the display device
        
        Returns
        -------
        tuple
            Tuple containing the recommended resolutions (width, height) of each device view.
            If the tuple length is 2, index 0 represents the left eye and index 1 represents the right eye
        """
        if self._use_ctypes:
            num_views = self._lib.getViewConfigurationViewsSize(self._app)
            views = (XrViewConfigurationView * num_views)()
            if self._lib.getViewConfigurationViews(self._app, views, num_views):
                return [(view.recommendedImageRectWidth, view.recommendedImageRectHeight) for view in views]
            else:
                return []
        else:
            return tuple([(view["recommendedImageRectWidth"], view["recommendedImageRectHeight"]) for view in self._app.getViewConfigurationViews()])

    def set_stereo_rectification(self, x: float = 0, y: float = 0, z: float = 0) -> None:
        """
        Set the angle (in radians) of the rotation axes for stereoscopic view rectification

        Parameters
        ----------
        x: float, optional
            Angle (in radians) of the X-axis (default: 0)
        y: float, optional
            Angle (in radians) of the Y-axis (default: 0)
        x: float, optional
            Angle (in radians) of the Z-axis (default: 0)
        """
        self._rectification_quat_left = pxr.Gf.Quatd(1, 0, 0, 0)
        self._rectification_quat_right = pxr.Gf.Quatd(1, 0, 0, 0)
        if x:   # w,x,y,z = cos(a/2), sin(a/2), 0, 0
            self._rectification_quat_left *= pxr.Gf.Quatd(np.cos(x/2), np.sin(x/2), 0, 0)
            self._rectification_quat_right *= pxr.Gf.Quatd(np.cos(-x/2), np.sin(-x/2), 0, 0)
        if y:   # w,x,y,z = cos(a/2), 0, sin(a/2), 0
            self._rectification_quat_left *= pxr.Gf.Quatd(np.cos(y/2), 0, np.sin(y/2), 0)
            self._rectification_quat_right *= pxr.Gf.Quatd(np.cos(-y/2), 0, np.sin(-y/2), 0)
        if z:   # w,x,y,z = cos(a/2), 0, 0, sin(a/2)
            self._rectification_quat_left *= pxr.Gf.Quatd(np.cos(z/2), 0, 0, np.sin(z/2))
            self._rectification_quat_right *= pxr.Gf.Quatd(np.cos(-z/2), 0, 0, np.sin(-z/2))

    def set_frame_transformations(self, fit: bool = False, flip: Union[int, tuple, None] = None) -> None:
        """
        Specify the transformations to be applied to the rendered images

        Parameters
        ----------
        fit: bool, optionl
            Adjust each rendered image to the recommended resolution of the display device by cropping and scaling the image from its center (default: False)
            OpenCV.resize method with INTER_LINEAR interpolation will be used to scale the image to the recommended resolution
        flip: int, tuple or None, optionl
            Flips each image around vertical (0), horizontal (1), or both axes (0,1) (default: None) 
        """
        self._transform_fit = fit
        self._transform_flip = flip

    def teleport_prim(self, prim: pxr.Usd.Prim, position: pxr.Gf.Vec3d, rotation: pxr.Gf.Quatd) -> None:
        """
        Teleport the prim specified by the given transformation (position and rotation)

        Parameters
        ----------
        prim: pxr.Usd.Prim
            Target prim
        position: pxr.Gf.Vec3d
            Cartesian position (in centimeters) used to transform the prim
        rotation: pxr.Gf.Quatd
            Rotation (as quaternion) used to transform the prim
        """
        # TODO: specify the base position and translation when camera is configured (setup_mono/stereo_view)
        properties = prim.GetPropertyNames()
        translated, rotated = False, False
        # translate
        if "xformOp:translate" in properties or "xformOp:translation" in properties:
            prim.GetAttribute("xformOp:translate").Set(position)
            translated = True
        # rotate
        if "xformOp:rotate" in properties:
            prim.GetAttribute("xformOp:rotate").Set(Gf.Vec3d(90, 0, 0))
        elif "xformOp:rotateXYZ" in properties:
            prim.GetAttribute("xformOp:rotateXYZ").Set(Gf.Vec3d(90, 0, 0))
        
        mat = Gf.Matrix4d()
        mat.SetIdentity()
        if not translated:
            mat.SetTranslateOnly(position)
        if not rotated:
            mat.SetRotateOnly(Gf.Rotation(rotation))
        if "xformOp:transform" in properties:
            prim.GetAttribute("xformOp:transform").Set(mat)
        else:
            print("Create")
            UsdGeom.Xformable(prim).AddXformOp(UsdGeom.XformOp.TypeTransform, UsdGeom.XformOp.PrecisionDouble, "").Set(mat)

    def subscribe_render_event(self, callback=None):
        def _internal_callback(num_views, views, configuration_views):
            # XR_REFERENCE_SPACE_TYPE_VIEW:  +Y up, +X to the right, and -Z forward
            # XR_REFERENCE_SPACE_TYPE_LOCAL: +Y up, +X to the right, and -Z forward
            # XR_REFERENCE_SPACE_TYPE_STAGE: +Y up, and the X and Z axes aligned with the rectangle edges
            # teleport left camera
            if self._use_ctypes:
                position = views[0].pose.position
                rotation = views[0].pose.orientation
                position = Gf.Vec3d(position.x, position.y, position.z)
                rotation = Gf.Quatd(rotation.w, rotation.x, rotation.y, rotation.z)
            else:
                position = views[0]["pose"]["position"]
                rotation = views[0]["pose"]["orientation"]
                position = Gf.Vec3d(position["x"] * 100, -position["z"] * 100, position["y"] * 100)
                rotation = Gf.Quatd(rotation["w"], rotation["x"], rotation["y"], rotation["z"])
            self.teleport_prim(self._prim_left, position, self._rectification_quat_left * rotation)            

            # teleport right camera
            if num_views == 2:
                if self._use_ctypes:
                    position = views[1].pose.position
                    rotation = views[1].pose.orientation
                    position = Gf.Vec3d(position.x, position.y, position.z)
                    rotation = Gf.Quatd(rotation.w, rotation.x, rotation.y, rotation.z)
                else:
                    position = views[1]["pose"]["position"]
                    rotation = views[1]["pose"]["orientation"]
                    position = Gf.Vec3d(position["x"] * 100, -position["z"] * 100, position["y"] * 100)
                    rotation = Gf.Quatd(rotation["w"], rotation["x"], rotation["y"], rotation["z"])
                self.teleport_prim(self._prim_right, position, self._rectification_quat_right * rotation)
            
            # set frames
            try:
                frame_left = sensors.get_rgb(self._viewport_window_left)
                frame_right = sensors.get_rgb(self._viewport_window_right) if num_views == 2 else None
                self.set_frames(configuration_views, frame_left, frame_right)
            except Exception as e:
                print("[ERROR]", str(e))
        
        if callback is None:
            callback = _internal_callback
        if self._use_ctypes:
            self._callback_render_event = ctypes.CFUNCTYPE(None, ctypes.c_int, ctypes.POINTER(XrView), ctypes.POINTER(XrViewConfigurationView))(callback)
            self._lib.setRenderCallback(self._app, self._callback_render_event)
        else:
            self._callback_render_event = callback
            self._app.setRenderCallback(self._callback_render_event)

    def set_frames(self, configuration_views: list, left: np.ndarray, right: np.ndarray = None) -> bool:
        use_rgba = True if left.shape[2] == 4 else False
        if self._use_ctypes:
            self._frame_left = self._transform(configuration_views[0], left)
            if right is None:
                return bool(self._lib.setFrames(self._app, 
                                                self._frame_left.shape[1], self._frame_left.shape[0], self._frame_left.ctypes.data_as(ctypes.c_void_p),
                                                0, 0, None, 
                                                use_rgba))
            else:
                self._frame_right = self._transform(configuration_views[1], right)
                return bool(self._lib.setFrames(self._app, 
                                                self._frame_left.shape[1], self._frame_left.shape[0], self._frame_left.ctypes.data_as(ctypes.c_void_p),
                                                self._frame_right.shape[1], self._frame_right.shape[0], self._frame_right.ctypes.data_as(ctypes.c_void_p),
                                                use_rgba))
        else:
            self._frame_left = self._transform(configuration_views[0], left)
            if right is None:
                return self._app.setFrames(self._frame_left, np.array(None), use_rgba)
            else:
                self._frame_right = self._transform(configuration_views[1], right)
                return self._app.setFrames(self._frame_left, self._frame_right, use_rgba)

    def _transform(self, configuration_view: XrViewConfigurationView, frame: np.ndarray) -> np.ndarray:
        transformed = False
        if self._transform_flip is not None:
            transformed = True
            frame = np.flip(frame, axis=self._transform_flip)
        if self._transform_fit:
            transformed = True
            current_ratio = frame.shape[1] / frame.shape[0]
            if self._use_ctypes:
                recommended_ratio = configuration_view.recommendedImageRectWidth / configuration_view.recommendedImageRectHeight
                recommended_size = (configuration_view.recommendedImageRectWidth, configuration_view.recommendedImageRectHeight)
            else:
                recommended_ratio = configuration_view["recommendedImageRectWidth"] / configuration_view["recommendedImageRectHeight"]
                recommended_size = (configuration_view["recommendedImageRectWidth"], configuration_view["recommendedImageRectHeight"])
            if current_ratio > recommended_ratio:
                m = int(abs(recommended_ratio * frame.shape[0] - frame.shape[1]) / 2)
                frame = cv2.resize(frame[:, m:-m], recommended_size, interpolation=cv2.INTER_LINEAR)
            else:
                m = int(abs(frame.shape[1] / recommended_ratio - frame.shape[0]) / 2)
                frame = cv2.resize(frame[m:-m, :], recommended_size, interpolation=cv2.INTER_LINEAR)
        return np.array(frame, copy=True) if transformed else frame




if __name__ == "__main__":
    import cv2
    import time
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('--ctypes', default=False, action="store_true", help='use ctypes instead of pybind11')
    args = parser.parse_args()

    _xr = acquire_openxr_interface()
    _xr.init(use_ctypes=args.ctypes)

    ready = False
    end = False

    def callback_action_pose(path, value):
        return
        print(path, value)

    def callback_action(path, value):
        if path in ["/user/hand/left/input/menu/click", "/user/hand/right/input/menu/click"]:
            # print(path, value)
            print(_xr.apply_haptic_feedback("/user/hand/left/output/haptic", {"duration": 1000000}))
            print(_xr.apply_haptic_feedback("/user/hand/right/output/haptic", {"duration": 1000000}))

    if _xr.create_instance():
        if _xr.get_system():
            _xr.subscribe_action_event("/user/head/input/volume_up/click", callback=callback_action)
            _xr.subscribe_action_event("/user/head/input/volume_down/click", callback=callback_action)
            _xr.subscribe_action_event("/user/head/input/mute_mic/click", callback=callback_action)
            _xr.subscribe_action_event("/user/hand/left/input/trigger/value", callback=callback_action)
            _xr.subscribe_action_event("/user/hand/right/input/trigger/value", callback=callback_action)
            _xr.subscribe_action_event("/user/hand/left/input/menu/click", callback=callback_action)
            _xr.subscribe_action_event("/user/hand/right/input/menu/click", callback=callback_action)

            _xr.subscribe_action_event("/user/hand/left/input/grip/pose", callback=callback_action_pose, reference_space=_xr.XR_REFERENCE_SPACE_TYPE_LOCAL)
            _xr.subscribe_action_event("/user/hand/right/input/grip/pose", callback=callback_action_pose, reference_space=_xr.XR_REFERENCE_SPACE_TYPE_LOCAL)

            _xr.subscribe_action_event("/user/hand/left/output/haptic", callback=callback_action)
            _xr.subscribe_action_event("/user/hand/right/output/haptic", callback=callback_action)
            if _xr.create_session():
                ready = True
            else:
                print("[ERROR]:", "createSession")
        else:
            print("[ERROR]:", "getSystem")
    else:
        print("[ERROR]:", "createInstance")

    if ready:
        cap = cv2.VideoCapture("/home/argus/Videos/xr/xr/sample.mp4")

        def callback_render(num_views, views, configuration_views):
            pass
            # global end
            # ret, frame = cap.read()
            # if ret:
            #     if num_views == 2:
            #         frame1 = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            #         _xr.set_frames(configuration_views, frame, frame1)

            #         # show frame
            #         k = 0.25
            #         frame = cv2.resize(np.hstack((frame, frame1)), (int(2*k*frame.shape[1]), int(k*frame.shape[0])))
            #         cv2.imshow('frame', frame)
            #         if cv2.waitKey(1) & 0xFF == ord('q'):
            #             exit()
            # else:
            #     end = True

        _xr.subscribe_render_event(callback_render)

        # while(cap.isOpened() or not end):
        for i in range(10000000):
            if _xr.poll_events():
                if _xr.is_session_running():
                    if not _xr.poll_actions():
                        print("[ERROR]:", "pollActions")
                        break
                    if not _xr.render_views():
                        print("[ERROR]:", "renderViews")
                        break
                else:
                    print("wait for is_session_running()")
                    time.sleep(0.1)
            else:
                break

    print("END")