import math
import pxr
import carb
import weakref
import omni.ext
import omni.ui as ui
from omni.kit.menu.utils import add_menu_items, remove_menu_items, MenuItemDescription

from semu.xr.openxr import _openxr


class Extension(omni.ext.IExt):
    def on_startup(self, ext_id):
        # get extension settings
        self._settings = carb.settings.get_settings()
        self._disable_openxr = self._settings.get("/exts/semu.xr.openxr/disable_openxr")

        self._window = None
        self._menu_items = [MenuItemDescription(name="OpenXR UI", onclick_fn=lambda a=weakref.proxy(self): a._menu_callback())]
        add_menu_items(self._menu_items, "Add-ons")

        self._xr = None
        self._ready = False

        self._timeline = omni.timeline.get_timeline_interface()
        self._physx_subs = omni.physx.get_physx_interface().subscribe_physics_step_events(self._on_simulation_step)

    def on_shutdown(self):
        self._physx_subs = None
        remove_menu_items(self._menu_items, "Add-ons")
        self._window = None

    def _get_reference_space(self):
        reference_space = [_openxr.XR_REFERENCE_SPACE_TYPE_VIEW, _openxr.XR_REFERENCE_SPACE_TYPE_LOCAL, _openxr.XR_REFERENCE_SPACE_TYPE_STAGE]
        return reference_space[self._xr_settings_reference_space.model.get_item_value_model().as_int]

    def _get_origin_pose(self):
        space_origin_position = [self._xr_settings_space_origin_position.model.get_item_value_model(i).as_int for i in self._xr_settings_space_origin_position.model.get_item_children()]
        space_origin_rotation = [self._xr_settings_space_origin_rotation.model.get_item_value_model(i).as_int for i in self._xr_settings_space_origin_rotation.model.get_item_children()]
        return {"position": pxr.Gf.Vec3d(*space_origin_position), "rotation": pxr.Gf.Vec3d(*space_origin_rotation)}

    def _get_frame_transformations(self):
        transform_fit = self._xr_settings_transform_fit.model.get_value_as_bool()
        transform_flip = [None, 0, 1, (0,1)]
        transform_flip = transform_flip[self._xr_settings_transform_flip.model.get_item_value_model().as_int]
        return {"fit": transform_fit, "flip": transform_flip}

    def _get_stereo_rectification(self):
        return [self._xr_settings_stereo_rectification.model.get_item_value_model(i).as_float * math.pi / 180.0 for i in self._xr_settings_stereo_rectification.model.get_item_children()]

    def _menu_callback(self):
        self._build_ui()

    def _on_start_openxr(self):
        # get parameters from ui
        graphics = [_openxr.XR_KHR_OPENGL_ENABLE_EXTENSION_NAME]
        graphics = graphics[self._xr_settings_graphics_api.model.get_item_value_model().as_int]

        form_factor = [_openxr.XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY, _openxr.XR_FORM_FACTOR_HANDHELD_DISPLAY]
        form_factor = form_factor[self._xr_settings_form_factor.model.get_item_value_model().as_int]

        blend_mode = [_openxr.XR_ENVIRONMENT_BLEND_MODE_OPAQUE, _openxr.XR_ENVIRONMENT_BLEND_MODE_ADDITIVE, _openxr.XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND]
        blend_mode = blend_mode[self._xr_settings_blend_mode.model.get_item_value_model().as_int]

        view_configuration_type = [_openxr.XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO, _openxr.XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO]
        view_configuration_type = view_configuration_type[self._xr_settings_view_configuration_type.model.get_item_value_model().as_int]

        # disable static parameters ui
        self._xr_settings_graphics_api.enabled = False
        self._xr_settings_form_factor.enabled = False
        self._xr_settings_blend_mode.enabled = False
        self._xr_settings_view_configuration_type.enabled = False

        if self._xr is None:
            self._xr = _openxr.acquire_openxr_interface(disable_openxr=self._disable_openxr)
            if not self._xr.init(graphics=graphics, use_ctypes=False):
                print("[ERROR] OpenXR.init with graphics: {}".format(graphics))

            # setup OpenXR application using default and ui parameters
            if self._xr.create_instance():
                if self._xr.get_system(form_factor=form_factor, blend_mode=blend_mode, view_configuration_type=view_configuration_type):
                    # create session and define interaction profiles
                    if self._xr.create_session():    
                        # setup cameras and viewports and prepare rendering using the internal callback
                        if view_configuration_type == _openxr.XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO:
                            self._xr.setup_mono_view()
                        elif view_configuration_type == _openxr.XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO:
                            self._xr.setup_stereo_view()
                        # enable/disable buttons
                        self._ui_start_xr.enabled = False
                        self._ui_stop_xr.enabled = True
                        # play 
                        self._timeline.play()
                        self._ready = True
                        return
                    else:
                        print("[ERROR] OpenXR.create_session")
                else:
                    print("[ERROR] OpenXR.get_system with form_factor: {}, blend_mode: {}, view_configuration_type: {}".format(form_factor, blend_mode, view_configuration_type))
            else:
                print("[ERROR] OpenXR.create_instance")

            self._on_stop_openxr()
            
    def _on_stop_openxr(self):
        self._ready = False

        _openxr.release_openxr_interface(self._xr)
        self._xr = None

        # enable static parameters ui
        self._xr_settings_graphics_api.enabled = True
        self._xr_settings_form_factor.enabled = True
        self._xr_settings_blend_mode.enabled = True
        self._xr_settings_view_configuration_type.enabled = True

        # enable/disable buttons
        self._ui_start_xr.enabled = True
        self._ui_stop_xr.enabled = False

    def _on_simulation_step(self, step):
        if self._ready and self._xr is not None:
            # origin
            self._xr.set_reference_system_pose(**self._get_origin_pose())
            # transformation and rectification
            self._xr.set_stereo_rectification(*self._get_stereo_rectification())
            self._xr.set_frame_transformations(**self._get_frame_transformations())
            # action and rendering loop
            if not self._xr.poll_events():
                self._on_stop_openxr()
                return
            if self._xr.is_session_running():
                if not self._xr.poll_actions():
                    self._on_stop_openxr()
                    return
                if not self._xr.render_views(self._get_reference_space()):
                    self._on_stop_openxr()
                    return

    def _build_ui(self):
        if not self._window:
            self._window = ui.Window(title="OpenXR UI", width=300, height=375, visible=True, dockPreference=ui.DockPreference.LEFT_BOTTOM)
            with self._window.frame:
                with ui.VStack():
                    ui.Spacer(height=5)
                    with ui.HStack(height=0):
                        ui.Label("Graphics API:", width=85, tooltip="OpenXR graphics API supported by the runtime")
                        self._xr_settings_graphics_api = ui.ComboBox(0, "OpenGL")
                    
                    ui.Spacer(height=5)
                    with ui.HStack(height=0):
                        ui.Label("Form factor:", width=80, tooltip="XrFormFactor enum. HEAD_MOUNTED_DISPLAY: the tracked display is attached to the user's head. HANDHELD_DISPLAY: the tracked display is held in the user's hand, independent from the user's head")
                        self._xr_settings_form_factor = ui.ComboBox(0, "Head Mounted Display", "Handheld Display")
                    ui.Spacer(height=5)
                    with ui.HStack(height=0):
                        ui.Label("Blend mode:", width=80, tooltip="XrEnvironmentBlendMode enum. OPAQUE: display the composition layers with no view of the physical world behind them. ADDITIVE: additively blend the composition layers with the real world behind the display. ALPHA BLEND: alpha-blend the composition layers with the real world behind the display")
                        self._xr_settings_blend_mode = ui.ComboBox(0, "Opaque", "Additive", "Alpha blend")
                    ui.Spacer(height=5)
                    with ui.HStack(height=0):
                        ui.Label("View configuration type:", width=145, tooltip="XrViewConfigurationType enum. MONO: one primary display (e.g. an AR phone's screen). STEREO: two primary displays, which map to a left-eye and right-eye view")
                        self._xr_settings_view_configuration_type = ui.ComboBox(1, "Mono", "Stereo")

                    ui.Spacer(height=5)
                    ui.Separator(height=1, width=0)
                    ui.Spacer(height=5)

                    with ui.HStack(height=0):
                        ui.Label("Space origin:", width=85)
                    ui.Spacer(height=5)
                    with ui.HStack(height=0):
                        ui.Label("  |-- Position (in centimeters):", width=165, tooltip="Cartesian position (in centimeters) used as reference origin")
                        self._xr_settings_space_origin_position = ui.MultiIntDragField(0, 0, 0, step=1)
                    ui.Spacer(height=5)
                    with ui.HStack(height=0):
                        ui.Label("  |-- Rotation (XYZ):", width=110, tooltip="Rotation (in degress) on each axis used as reference origin")
                        self._xr_settings_space_origin_rotation = ui.MultiIntDragField(0, 0, 0, min=-180, max=180)

                    ui.Spacer(height=5)
                    with ui.HStack(height=0):
                        style = {"Tooltip": {"width": 50, "word-wrap": "break-word"}}
                        ui.Label("Reference space (views):", width=145, tooltip="XrReferenceSpaceType enum. VIEW: track the view origin for the primary viewer. LOCAL: establish a world-locked origin. STAGE: runtime-defined space that can be walked around on", style=style)
                        self._xr_settings_reference_space = ui.ComboBox(1, "View", "Local", "Stage")

                    ui.Spacer(height=5)
                    with ui.HStack(height=0):
                        ui.Label("Stereo rectification (x,y,z):", width=150, tooltip="Angle (in degrees) on each rotation axis for stereoscopic rectification")
                        self._xr_settings_stereo_rectification = ui.MultiFloatDragField(0.0, 0.0, 0.0, min=-10, max=10, step=0.1)
                    
                    ui.Spacer(height=5)
                    with ui.HStack(height=0):
                        ui.Label("Frame transformations:")
                    ui.Spacer(height=5)
                    with ui.HStack(height=0):
                        ui.Label("  |-- Fit:", width=45, tooltip="Adjust each rendered image to the recommended resolution of the display device by cropping and scaling the image from its center")
                        self._xr_settings_transform_fit = ui.CheckBox()
                    ui.Spacer(height=5)
                    with ui.HStack(height=0):
                        ui.Label("  |-- Flip:", width=45, tooltip="Flip each image with respect to its view")
                        self._xr_settings_transform_flip = ui.ComboBox(0, "None", "Vertical", "Horizontal", "Both")

                    ui.Spacer(height=5)
                    ui.Separator(height=1, width=0)
                    ui.Spacer(height=5)
                    with ui.HStack(height=0):
                        self._ui_start_xr = ui.Button("Start OpenXR", height=0, clicked_fn=self._on_start_openxr)
                        self._ui_stop_xr = ui.Button("Stop OpenXR", height=0, clicked_fn=self._on_stop_openxr)
                    
            self._ui_stop_xr.enabled = False
                    