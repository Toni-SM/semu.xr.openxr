## OpenXR compact binding to create extended reality applications on NVIDIA Omniverse

> This extension provides a compact python binding (on top of the open standard [OpenXR](https://www.khronos.org/openxr/) for augmented reality (AR) and virtual reality (VR)) to create extended reality applications taking advantage of NVIDIA Omniverse rendering capabilities. In addition to updating views (e.g., head-mounted display), it enables subscription to any input event (e.g., controller buttons and triggers) and execution of output actions (e.g., haptic vibration) through a simple and efficient API for accessing conformant devices such as HTC Vive, Oculus and others...

<br>

### Table of Contents

- [Add the extension to a NVIDIA Omniverse app and enable it](#extension)
- [Diagrams](#diagrams)
- [Sample code](#sample)
- [GUI launcher](#gui)
- [Extension API](#api)
  - [Acquiring extension interface](#api-interface)
  - [API](#api-functions)
    - [```init```](#method-init)
    - [```is_session_running```](#method-is_session_running)
    - [```create_instance```](#method-create_instance)
    - [```get_system```](#method-get_system)
    - [```create_session```](#method-create_session)
    - [```poll_events```](#method-poll_events)
    - [```poll_actions```](#method-poll_actions)
    - [```render_views```](#method-render_views)
    - [```subscribe_action_event```](#method-subscribe_action_event)
    - [```apply_haptic_feedback```](#method-apply_haptic_feedback)
    - [```stop_haptic_feedback```](#method-stop_haptic_feedback)
    - [```setup_mono_view```](#method-setup_mono_view)
    - [```setup_stereo_view```](#method-setup_stereo_view)
    - [```get_recommended_resolutions```](#method-get_recommended_resolutions)
    - [```set_reference_system_pose```](#method-set_reference_system_pose)
    - [```set_stereo_rectification```](#method-set_stereo_rectification)
    - [```set_frame_transformations```](#method-set_frame_transformations)
    - [```teleport_prim```](#method-teleport_prim)
    - [```subscribe_render_event```](#method-subscribe_render_event)
    - [```set_frames```](#method-set_frames)
  - [Available enumerations](#api-enumerations)
  - [Available constants](#api-constants)

<br>

<a name="extension"></a>
### Add the extension to a NVIDIA Omniverse app and enable it

1. Add the the extension by following the steps described in [Extension Search Paths](https://docs.omniverse.nvidia.com/py/kit/docs/guide/extensions.html#extension-search-paths) or simply download and unzip the latest [release](https://github.com/Toni-SM/omni.add_on.openxr/releases) in one of the extension folders such as ```omniverse-apps/exts```

2. Enable the extension by following the steps described in [Extension Enabling/Disabling](https://docs.omniverse.nvidia.com/py/kit/docs/guide/extensions.html#extension-enabling-disabling)

3. Import the extension into any python code and use it...

    ```python
    from omni.add_on.openxr import _openxr
    ```

<br>

<a name="diagrams"></a>
### Diagrams

High-level overview of extension usage, including the order of function calls, callbacks and the action and rendering loop

<p align="center">
  <img src="https://user-images.githubusercontent.com/22400377/137474691-cdc1aeee-2c34-40ef-82f2-f2aa13587c45.png" width="55%">
</p>

Typical OpenXR application showing the grouping of the standard functions under the compact binding provided by the extension (adapted from [openxr-10-reference-guide.pdf](https://www.khronos.org/registry/OpenXR/specs/1.0/refguide/openxr-10-reference-guide.pdf))

![openxr-application](https://user-images.githubusercontent.com/22400377/136704215-5507bbee-666a-42da-b692-cbf8c08a749b.png)

<br>

<a name="sample"></a>
### Sample code

The following sample code shows a typical workflow that configures and renders on a stereo headset the view generated in an Omniverse application. It configures and subscribes two input actions to the left controller to 1) mirror on a simulated sphere the pose of the controller and 2) change the dimensions of the sphere based on the position of the trigger. In addition, an output action, a haptic vibration, is configured and executed when the controller trigger reaches its maximum position

A short video, after the code, shows a test of the OpenXR application from the Script Editor using an HTC Vive Pro

```python
import omni
from omni.add_on.openxr import _openxr

# create a sphere to mirror the controller's pose
sphere_prim = omni.usd.get_context().get_stage().DefinePrim("/sphere", "Sphere")

# acquire interface
xr = _openxr.acquire_openxr_interface()

# setup OpenXR application using default parameters
xr.init()
xr.create_instance()
xr.get_system()

# action callback
def on_action_event(path, value):
    # process controller's trigger
    if path == "/user/hand/left/input/trigger/value":
      # modify the sphere's radius (from 1 o 10 centimeters) according to the controller's trigger position
      sphere_prim.GetAttribute("radius").Set(value * 9 + 1)
      # apply haptic vibration when the controller's trigger is fully depressed
      if value == 1:
        xr.apply_haptic_feedback("/user/hand/left/output/haptic", {"duration": _openxr.XR_MIN_HAPTIC_DURATION})
    # mirror the controller's pose on the sphere (cartesian position and rotation as quaternion)
    elif path == "/user/hand/left/input/grip/pose":
        xr.teleport_prim(sphere_prim, value[0], value[1])

# subscribe controller actions (haptic actions don't require callbacks) 
xr.subscribe_action_event("/user/hand/left/input/grip/pose", callback=on_action_event, reference_space=_openxr.XR_REFERENCE_SPACE_TYPE_STAGE)
xr.subscribe_action_event("/user/hand/left/input/trigger/value", callback=on_action_event)
xr.subscribe_action_event("/user/hand/left/output/haptic")

# create session and define interaction profiles
xr.create_session()

# setup cameras and viewports and prepare rendering using the internal callback
xr.setup_stereo_view()
xr.set_frame_transformations(flip=0)
xr.set_stereo_rectification(y=0.05)

# execute action and rendering loop on each simulation step
def on_simulation_step(step):
    if xr.poll_events() and xr.is_session_running():
        xr.poll_actions()
        xr.render_views(_openxr.XR_REFERENCE_SPACE_TYPE_STAGE)

physx_subs = omni.physx.get_physx_interface().subscribe_physics_step_events(on_simulation_step)
```

[Watch the sample video](https://user-images.githubusercontent.com/22400377/136706132-cc96dc22-235d-454d-a145-a65f6a35c9f2.mp4)

<br>

<a name="gui"></a>
### GUI launcher

The extension also provides a graphical user interface that helps to launch a partially configurable OpenXR application form a window. This interface is located in the *Add-ons > OpenXR UI* menu

The first four options (Graphics API, Form factor, Blend mode, View configuration type) cannot be modified once the OpenXR application is running. They are used to create and configure the OpenXR instance, system and session

The other options (under the central separator) can be modified while the application is running. They help to modify the pose of the reference system, or to perform transformations on the images to be rendered, for example.

<p align="center">
  <img src="https://user-images.githubusercontent.com/22400377/137474713-b148ded7-0ece-4d8c-aa30-79244920cf64.png" width="65%">
</p>

<br>

<a name="api"></a>
### Extension API

<a name="api-interface"></a>
#### Acquiring extension interface

* Acquire OpenXR interface

  ```python
  _openxr.acquire_openxr_interface() -> omni::add_on::openxr::OpenXR
  ```

* Release OpenXR interface

  ```python
  _openxr.release_openxr_interface(xr: omni::add_on::openxr::OpenXR) -> None
  ```

<a name="api-functions"></a>
#### API

The following functions are provided on the OpenXR interface:

<a name="method-init"></a>
- Init OpenXR application by loading the necessary libraries

  ```python
  init(graphics: str = "OpenGL", use_ctypes: bool = False) -> bool
  ```

  Parameters: 
  - graphics: ```str```

    OpenXR graphics API supported by the runtime (OpenGL, OpenGLES, Vulkan, D3D11, D3D12). **Note:** At the moment only OpenGL is available
  
  - use_ctypes: ```bool```, optional
  
    If true, use ctypes as C/C++ interface instead of pybind11 (default)

  Returns:
  - ```bool```

    ```True``` if initialization was successful, otherwise ```False```

<a name="method-is_session_running"></a>
- Get OpenXR session's running status

  ```python
  is_session_running() -> bool
  ```

  Returns:
  - ```bool```
    
    Return ```True``` if the OpenXR session is running, ```False``` otherwise

<a name="method-create_instance"></a>
- Create an OpenXR instance to allow communication with an OpenXR runtime

  ```python
  create_instance(application_name: str = "Omniverse (XR)", engine_name: str = "", api_layers: list = [], extensions: list = []) -> bool
  ```

  Parameters:
  - application_name: ```str```, optional
    
    Name of the OpenXR application (default: *Omniverse (XR)*)

  - engine_name: ```str```, optional
    
    Name of the engine (if any) used to create the application (empty by default)

  - api_layers: ```list``` of ```str```, optional
    
    [API layers](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#api-layers) to be inserted between the OpenXR application and the runtime implementation

  - extensions: ```list``` of ```str```, optional
    
    [Extensions](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#extensions) to be loaded. **Note:** The graphics API selected during initialization (init) is automatically included in the extensions to be loaded. At the moment only the graphic extensions are configured

  Returns:
  - ```bool```
    
    ```True``` if the instance has been created successfully, otherwise ```False```

<a name="method-get_system"></a>
- Obtain the system represented by a collection of related devices at runtime

  ```python
  get_system(form_factor: int = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY, blend_mode: int = XR_ENVIRONMENT_BLEND_MODE_OPAQUE, view_configuration_type: int = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) -> bool
  ```

  Parameters:
  - form_factor: {```XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY```, ```XR_FORM_FACTOR_HANDHELD_DISPLAY```}, optional
    
    Desired [form factor](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#form_factor_description) from ```XrFormFactor``` enum (default: ```XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY```)
    
  - blend_mode: {```XR_ENVIRONMENT_BLEND_MODE_OPAQUE```, ```XR_ENVIRONMENT_BLEND_MODE_ADDITIVE```, ```XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND```}, optional
    
    Desired environment [blend mode](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#environment_blend_mode) from ```XrEnvironmentBlendMode``` enum (default: ```XR_ENVIRONMENT_BLEND_MODE_OPAQUE```)
    
  - view_configuration_type: {```XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO```, ```XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO```}, optional
    
    Primary [view configuration](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#view_configurations) type from ```XrViewConfigurationType``` enum (default: ```XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO```)

  Returns:
  - ```bool```
    
    ```True``` if the system has been obtained successfully, otherwise ```False```

<a name="method-create_session"></a>
- Create an OpenXR session that represents an application's intention to display XR content

  ```python
  create_session() -> bool
  ```
        
  Returns:
  - ```bool```
    
    ```True``` if the session has been created successfully, otherwise ```False```

<a name="method-poll_events"></a>
- [Event polling](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#event-polling) and processing

  ```python
  poll_events() -> bool
  ```

  Returns:
  - ```bool```
    
    ```False``` if the running session needs to end (due to the user closing or switching the application, etc.), otherwise ```False```

<a name="method-poll_actions"></a>
- [Action](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#_action_overview) polling

  ```python
  poll_actions() -> bool
  ```

  Returns:
  - ```bool```
    
    ```True``` if there is no error during polling, otherwise ```False```

<a name="method-render_views"></a>
- Present rendered images to the user's views according to the selected reference space

  ```python
  render_views(reference_space: int = XR_REFERENCE_SPACE_TYPE_LOCAL) -> bool
  ```
  
  Parameters:
  - reference_space: {```XR_REFERENCE_SPACE_TYPE_VIEW```, ```XR_REFERENCE_SPACE_TYPE_LOCAL```, ```XR_REFERENCE_SPACE_TYPE_STAGE```}, optional
    
    Desired [reference space](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#reference-spaces) type from ```XrReferenceSpaceType``` enum used to render the images (default: ```XR_REFERENCE_SPACE_TYPE_LOCAL```)

  Returns:
  - ```bool```
    
    ```True``` if there is no error during rendering, otherwise ```False```

<a name="method-subscribe_action_event"></a>
- Create an action given a path and subscribe a callback function to the update event of this action

  ```python
  subscribe_action_event(path: str, callback: Union[Callable[[str, object], None], None] = None, action_type: Union[int, None] = None, reference_space: Union[int, None] = XR_REFERENCE_SPACE_TYPE_LOCAL) -> bool
  ```

  If ```action_type``` is ```None``` the action type will be automatically defined by parsing the last segment of the path according to the following policy:

  | Action type (```XrActionType```) | Last segment of the path |
  |----------------------------------|--------------------------|
  | ```XR_ACTION_TYPE_BOOLEAN_INPUT``` | */click*, */touch* |
  | ```XR_ACTION_TYPE_FLOAT_INPUT``` | */value*, */force* |
  | ```XR_ACTION_TYPE_VECTOR2F_INPUT``` | */x*, */y* |
  | ```XR_ACTION_TYPE_POSE_INPUT``` | */pose* |
  | ```XR_ACTION_TYPE_VIBRATION_OUTPUT``` | */haptic*, */haptic_left*, */haptic_right*, */haptic_left_trigger*, */haptic_right_trigger* |

  The callback function (a callable object) should have only the following 2 parameters:
  - path: ```str```
    
    The complete path (user path and subpath) of the action that invokes the callback
    
   - value: ```bool```, ```float```, ```tuple(float, float)```, ```tuple(pxr.Gf.Vec3d, pxr.Gf.Quatd)```
     
     The current state of the action according to its type
     
     | Action type (```XrActionType```) | python type |
     |----------------------------------|-------------|
     | ```XR_ACTION_TYPE_BOOLEAN_INPUT``` | ```bool``` |
     | ```XR_ACTION_TYPE_FLOAT_INPUT``` | ```float``` |
     | ```XR_ACTION_TYPE_VECTOR2F_INPUT``` (x, y) | ```tuple(float, float)``` |
     | ```XR_ACTION_TYPE_POSE_INPUT``` (position (in centimeters), rotation as quaternion) | ```tuple(pxr.Gf.Vec3d, pxr.Gf.Quatd)``` |

  ```XR_ACTION_TYPE_VIBRATION_OUTPUT``` actions will not invoke their callback function. In this case the callback must be None
     
  ```XR_ACTION_TYPE_POSE_INPUT``` also specifies, through the definition of the reference_space parameter, the reference space used to retrieve the pose

  The collection of available paths corresponds to the following [interaction profiles](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#semantic-path-interaction-profiles):
    - [Khronos Simple Controller](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#_khronos_simple_controller_profile)
    - [Google Daydream Controller](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#_google_daydream_controller_profile)
    - [HTC Vive Controller](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#_htc_vive_controller_profile)
    - [HTC Vive Pro](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#_htc_vive_pro_profile)
    - [Microsoft Mixed Reality Motion Controller](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#_microsoft_mixed_reality_motion_controller_profile)
    - [Microsoft Xbox Controller](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#_microsoft_xbox_controller_profile)
    - [Oculus Go Controller](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#_oculus_go_controller_profile)
    - [Oculus Touch Controller](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#_oculus_touch_controller_profile)
    - [Valve Index Controller](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#_valve_index_controller_profile)
  
  Parameters:
  - path: ```str```
    
    Complete [path](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#semantic-path-reserved) (user path and subpath) referring to the action
    
  - callback: callable object (2 parameters) or ```None``` for ```XR_ACTION_TYPE_VIBRATION_OUTPUT```
    
    Callback invoked when the state of the action changes
  
  - action_type: {```XR_ACTION_TYPE_BOOLEAN_INPUT```, ```XR_ACTION_TYPE_FLOAT_INPUT```, ```XR_ACTION_TYPE_VECTOR2F_INPUT```, ```XR_ACTION_TYPE_POSE_INPUT```, ```XR_ACTION_TYPE_VIBRATION_OUTPUT```} or ```None```, optional
    
    Action [type](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrActionType) from ```XrActionType``` enum (default: ```None```)
  
  - reference_space: {```XR_REFERENCE_SPACE_TYPE_VIEW```, ```XR_REFERENCE_SPACE_TYPE_LOCAL```, ```XR_REFERENCE_SPACE_TYPE_STAGE```}, optional
    
    Desired [reference space](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#reference-spaces) type from ```XrReferenceSpaceType``` enum used to retrieve the pose (default: ```XR_REFERENCE_SPACE_TYPE_LOCAL```)

  Returns
  - ```bool```
    
    ```True``` if there is no error during action creation, otherwise ```False```

<a name="method-apply_haptic_feedback"></a>
- Apply a [haptic feedback](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#_output_actions_and_haptics) to a device defined by a path (user path and subpath)

  ```python
  apply_haptic_feedback(path: str, haptic_feedback: dict) -> bool
  ```

  Parameters:
  - path: ```str```
    
    Complete [path](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#semantic-path-reserved) (user path and subpath) referring to the action
    
  - haptic_feedback: ```dict```
    
    A python dictionary containing the field names and value of a ```XrHapticBaseHeader```-based structure. **Note:** At the moment the only haptics type supported is the unextended OpenXR [XrHapticVibration](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrHapticVibration)

  Returns:
  - ```bool```
    
    ```True``` if there is no error during the haptic feedback application, otherwise ```False```

<a name="method-stop_haptic_feedback"></a>
- Stop a [haptic feedback](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#_output_actions_and_haptics) applied to a device defined by a path (user path and subpath)

  ```python
  stop_haptic_feedback(path: str) -> bool
  ```

  Parameters:
  - path: ```str```
    
    Complete [path](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#semantic-path-reserved) (user path and subpath) referring to the action

  Returns:
  - ```bool```
    
    ```True``` if there is no error during the haptic feedback stop, otherwise ```False```

<a name="method-setup_mono_view"></a>
- Setup Omniverse viewport and camera for monoscopic rendering

  ```python
  setup_mono_view(camera: Union[str, pxr.Sdf.Path, pxr.Usd.Prim] = "/OpenXR/Cameras/camera", camera_properties: dict = {"focalLength": 10}) -> None
  ```

  This method obtains the viewport window for the given camera. If the viewport window does not exist, a new one is created and the camera is set as active. If the given camera does not exist, a new camera is created with the same path and set to the recommended resolution of the display device
        
  Parameters:
  - camera: ```str```, ```pxr.Sdf.Path``` or ```pxr.Usd.Prim```, optional
    
    Omniverse camera prim or path (default: */OpenXR/Cameras/camera*)
  
  - camera_properties: ```dict```
    
    Dictionary containing the [camera properties](https://docs.omniverse.nvidia.com/app_create/prod_materials-and-rendering/cameras.html#camera-properties) supported by the Omniverse kit to be set (default: ```{"focalLength": 10}```)

<a name="method-setup_stereo_view"></a>
- Setup Omniverse viewports and cameras for stereoscopic rendering

  ```python
  setup_stereo_view(left_camera: Union[str, pxr.Sdf.Path, pxr.Usd.Prim] = "/OpenXR/Cameras/left_camera", right_camera: Union[str, pxr.Sdf.Path, pxr.Usd.Prim, None] = "/OpenXR/Cameras/right_camera", camera_properties: dict = {"focalLength": 10}) -> None
  ```

  This method obtains the viewport window for each camera. If the viewport window does not exist, a new one is created and the camera is set as active. If the given cameras do not exist, new cameras are created with the same path and set to the recommended resolution of the display device
        
  Parameters:
  - left_camera: ```str```, ```pxr.Sdf.Path``` or ```pxr.Usd.Prim```, optional
    
    Omniverse left camera prim or path (default: */OpenXR/Cameras/left_camera*)
    
  - right_camera: ```str```, ```pxr.Sdf.Path``` or ```pxr.Usd.Prim```, optional
    
    Omniverse right camera prim or path (default: */OpenXR/Cameras/right_camera*)
    
  - camera_properties: ```dict```
    
    Dictionary containing the [camera properties](https://docs.omniverse.nvidia.com/app_create/prod_materials-and-rendering/cameras.html#camera-properties) supported by the Omniverse kit to be set (default: ```{"focalLength": 10}```)

<a name="method-get_recommended_resolutions"></a>
- Get the recommended resolution of the display device

  ```python
  get_recommended_resolutions() -> tuple
  ```
  
  Returns:
  - ```tuple```
    
    Tuple containing the recommended resolutions (width, height) of each device view. If the tuple length is 2, index 0 represents the left eye and index 1 represents the right eye

<a name="method-set_reference_system_pose"></a>
- Set the pose of the origin of the reference system

  ```python
  set_reference_system_pose(position: Union[pxr.Gf.Vec3d, None] = None, rotation: Union[pxr.Gf.Vec3d, None] = None) -> None
  ```
  
  Parameters:
  - position: ```pxr.Gf.Vec3d``` or ```None```, optional

    Cartesian position (in centimeters) (default: ```None```)
  
  - rotation: ```pxr.Gf.Vec3d``` or ```None```, optional
    
    Rotation (in degress) on each axis (default: ```None```)

<a name="method-set_stereo_rectification"></a>
- Set the angle (in radians) of the rotation axes for stereoscopic view rectification

  ```python
  set_stereo_rectification(x: float = 0, y: float = 0, z: float = 0) -> None
  ```

  Parameters:
  - x: ```float```, optional
    
    Angle (in radians) of the X-axis (default: 0)
  
  - y: ```float```, optional
    
    Angle (in radians) of the Y-axis (default: 0)
    
  - x: ```float```, optional
    
    Angle (in radians) of the Z-axis (default: 0)

<a name="method-set_frame_transformations"></a>
- Specify the transformations to be applied to the rendered images

  ```python
  set_frame_transformations(fit: bool = False, flip: Union[int, tuple, None] = None) -> None
  ```
  
  Parameters:
  - fit: ```bool```, optionl
    
    Adjust each rendered image to the recommended resolution of the display device by cropping and scaling the image from its center (default: ```False```). OpenCV ```resize``` method with ```INTER_LINEAR``` interpolation will be used to scale the image to the recommended resolution
  
  - flip: ```int```, ```tuple``` or ```None```, optionl
    
    Flip each image around vertical (0), horizontal (1), or both axes (0,1) (default: ```None```) 

<a name="method-teleport_prim"></a>
- Teleport the prim specified by the given transformation (position and rotation)

  ```python
  teleport_prim(prim: pxr.Usd.Prim, position: pxr.Gf.Vec3d, rotation: pxr.Gf.Quatd, reference_position: Union[pxr.Gf.Vec3d, None] = None, reference_rotation: Union[pxr.Gf.Vec3d, None] = None) -> None
  ```

  Parameters:
  - prim: ```pxr.Usd.Prim```
    
    Target prim
    
  - position: ```pxr.Gf.Vec3d```
    
    Cartesian position (in centimeters) used to transform the prim
    
  - rotation: ```pxr.Gf.Quatd```
    
    Rotation (as quaternion) used to transform the prim
    
  - reference_position: ```pxr.Gf.Vec3d``` or ```None```, optional
    
    Cartesian position (in centimeters) used as reference system (default: ```None```)
    
  - reference_rotation: ```pxr.Gf.Vec3d``` or ```None```, optional
    
    Rotation (in degress) on each axis used as reference system (default: ```None```)

<a name="method-subscribe_render_event"></a>
- Subscribe a callback function to the render event

  ```python
  subscribe_render_event(callback=None) -> None
  ```

  The callback function (a callable object) should have only the following 3 parameters:
  
  - num_views: ```int```
    
    The number of views to render: mono (1), stereo (2)
    
  - views: ```tuple``` of ```XrView``` structure
  
    A [XrView](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrView) structure contains the view pose and projection state necessary to render a image. The length of the tuple corresponds to the number of views (if the tuple length is 2, index 0 represents the left eye and index 1 represents the right eye)

  - configuration_views: ```tuple``` of ```XrViewConfigurationView``` structure
    
    A [XrViewConfigurationView](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrViewConfigurationView) structure specifies properties related to rendering of a view (e.g. the optimal width and height to be used when rendering the view). The length of the tuple corresponds to the number of views (if the tuple length is 2, index 0 represents the left eye and index 1 represents the right eye)

    The callback function must call the ```set_frames``` function to pass to the selected graphics API the image or images to be rendered

    If the callback is None, an internal callback will be used to render the views. This internal callback updates the pose of the cameras according to the specified reference system, gets the images from the previously configured viewports and invokes the ```set_frames``` function to render the views

  Parameters:
  - callback: callable object (3 parameters) or ```None```, optional
    
    Callback invoked on each render event (default: ```None```)

<a name="method-set_frames"></a>
- Pass to the selected graphics API the images to be rendered in the views

  ```python
  set_frames(configuration_views: list, left: numpy.ndarray, right: numpy.ndarray = None) -> bool
  ```
  
  In the case of stereoscopic devices, the parameters left and right represent the left eye and right eye respectively. To pass an image to the graphic API of monoscopic devices only the parameter left should be used (the parameter right must be ```None```)

  This function will apply to each image the transformations defined by the ```set_frame_transformations``` function if they were specified

  Parameters:
  - configuration_views: ```tuple``` of ```XrViewConfigurationView``` structure
    
    A [XrViewConfigurationView](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrViewConfigurationView) structure specifies properties related to rendering of a view (e.g. the optimal width and height to be used when rendering the view)
    
  - left: ```numpy.ndarray```
    
    RGB or RGBA image (```numpy.uint8```)  
  
  - right: ```numpy.ndarray``` or ```None```
    
    RGB or RGBA image (```numpy.uint8```)

  Returns:
  - ```bool```
    
    ```True``` if there is no error during the passing to the selected graphics API, otherwise ```False```

<a name="api-enumerations"></a>
#### Available enumerations

- Form factors supported by OpenXR runtimes ([```XrFormFactor```](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrFormFactor))

  - ```XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY``` = 1

  - ```XR_FORM_FACTOR_HANDHELD_DISPLAY``` = 2

- Environment blend mode ([```XrEnvironmentBlendMode```](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrEnvironmentBlendMode))

  - ```XR_ENVIRONMENT_BLEND_MODE_OPAQUE``` = 1

  - ```XR_ENVIRONMENT_BLEND_MODE_ADDITIVE``` = 2

  - ```XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND``` = 3

- Primary view configuration type ([```XrViewConfigurationType```](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrViewConfigurationType))

  - ```XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO``` = 1

  - ```XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO``` = 2

- Reference spaces ([```XrReferenceSpaceType```](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrReferenceSpaceType))

  - ```XR_REFERENCE_SPACE_TYPE_VIEW``` = 1

  - ```XR_REFERENCE_SPACE_TYPE_LOCAL``` = 2

  - ```XR_REFERENCE_SPACE_TYPE_STAGE``` = 3

- Action type ([```XrActionType```](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrActionType))

  - ```XR_ACTION_TYPE_BOOLEAN_INPUT``` = 1

  - ```XR_ACTION_TYPE_FLOAT_INPUT``` = 2

  - ```XR_ACTION_TYPE_VECTOR2F_INPUT``` = 3

  - ```XR_ACTION_TYPE_POSE_INPUT``` = 4

  - ```XR_ACTION_TYPE_VIBRATION_OUTPUT``` = 100

<a name="api-constants"></a>
#### Available constants

- Graphics API extension names

  - ```XR_KHR_OPENGL_ENABLE_EXTENSION_NAME``` = "XR_KHR_opengl_enable"

  - ```XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME``` = "XR_KHR_opengl_es_enable"

  - ```XR_KHR_VULKAN_ENABLE_EXTENSION_NAME``` = "XR_KHR_vulkan_enable"

  - ```XR_KHR_D3D11_ENABLE_EXTENSION_NAME``` = "XR_KHR_D3D11_enable"

  - ```XR_KHR_D3D12_ENABLE_EXTENSION_NAME``` = "XR_KHR_D3D12_enable"

- Useful constants for applying haptic feedback

  - ```XR_NO_DURATION``` = 0

  - ```XR_INFINITE_DURATION``` = 0x7fffffffffffffff

  - ```XR_MIN_HAPTIC_DURATION``` = -1

  - ```XR_FREQUENCY_UNSPECIFIED``` = 0
