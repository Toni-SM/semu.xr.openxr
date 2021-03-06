
#include <pybind11/pybind11.h>

#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "xr.cpp"

namespace py = pybind11;


namespace pybind11 { namespace detail {
    template <> struct type_caster<XrView>{
        public:
            PYBIND11_TYPE_CASTER(XrView, _("XrView"));

            // conversion from C++ to Python
            static handle cast(XrView src, return_value_policy /* policy */, handle /* parent */){
                
                PyObject * fov = PyDict_New();
                PyDict_SetItemString(fov, "angleLeft", PyFloat_FromDouble(src.fov.angleLeft));
                PyDict_SetItemString(fov, "angleRight", PyFloat_FromDouble(src.fov.angleRight));
                PyDict_SetItemString(fov, "angleUp", PyFloat_FromDouble(src.fov.angleUp));
                PyDict_SetItemString(fov, "angleDown", PyFloat_FromDouble(src.fov.angleDown));

                PyObject * position = PyDict_New();
                PyDict_SetItemString(position, "x", PyFloat_FromDouble(src.pose.position.x));
                PyDict_SetItemString(position, "y", PyFloat_FromDouble(src.pose.position.y));
                PyDict_SetItemString(position, "z", PyFloat_FromDouble(src.pose.position.z));

                PyObject * orientation = PyDict_New();
                PyDict_SetItemString(orientation, "x", PyFloat_FromDouble(src.pose.orientation.x));
                PyDict_SetItemString(orientation, "y", PyFloat_FromDouble(src.pose.orientation.y));
                PyDict_SetItemString(orientation, "z", PyFloat_FromDouble(src.pose.orientation.z));
                PyDict_SetItemString(orientation, "w", PyFloat_FromDouble(src.pose.orientation.w));

                PyObject * pose = PyDict_New();
                PyDict_SetItemString(pose, "position", position);
                PyDict_SetItemString(pose, "orientation", orientation);

                PyObject * view = PyDict_New();
                PyDict_SetItemString(view, "type", PyLong_FromLong(src.type));
                PyDict_SetItemString(view, "next", Py_None);
                PyDict_SetItemString(view, "pose", pose);
                PyDict_SetItemString(view, "fov", fov);

                return view;
            }
    };

    template <> struct type_caster<XrViewConfigurationView>{
        public:
            PYBIND11_TYPE_CASTER(XrViewConfigurationView, _("XrViewConfigurationView"));

            // conversion from C++ to Python
            static handle cast(XrViewConfigurationView src, return_value_policy /* policy */, handle /* parent */){
                
                PyObject * configurationView = PyDict_New();
                PyDict_SetItemString(configurationView, "type", PyLong_FromLong(src.type));
                PyDict_SetItemString(configurationView, "next", Py_None);
                PyDict_SetItemString(configurationView, "recommendedImageRectWidth", PyLong_FromLong(src.recommendedImageRectWidth));
                PyDict_SetItemString(configurationView, "maxImageRectWidth", PyLong_FromLong(src.maxImageRectWidth));
                PyDict_SetItemString(configurationView, "recommendedImageRectHeight", PyLong_FromLong(src.recommendedImageRectHeight));
                PyDict_SetItemString(configurationView, "maxImageRectHeight", PyLong_FromLong(src.maxImageRectHeight));
                PyDict_SetItemString(configurationView, "recommendedSwapchainSampleCount", PyLong_FromLong(src.recommendedSwapchainSampleCount));
                PyDict_SetItemString(configurationView, "maxSwapchainSampleCount", PyLong_FromLong(src.maxSwapchainSampleCount));

                return configurationView;
            }
    };

    template <> struct type_caster<ActionState>{
        public:
            PYBIND11_TYPE_CASTER(ActionState, _("ActionState"));

            // conversion from C++ to Python
            static handle cast(ActionState src, return_value_policy /* policy */, handle /* parent */){
                
                PyObject * state = PyDict_New();
                PyDict_SetItemString(state, "type", PyLong_FromLong(src.type));
                PyDict_SetItemString(state, "path", PyUnicode_FromString(src.path));
                PyDict_SetItemString(state, "isActive", PyBool_FromLong(src.isActive));
                PyDict_SetItemString(state, "stateBool", PyBool_FromLong(src.stateBool));
                PyDict_SetItemString(state, "stateFloat", PyFloat_FromDouble(src.stateFloat));
                PyDict_SetItemString(state, "stateVectorX", PyFloat_FromDouble(src.stateVectorX));
                PyDict_SetItemString(state, "stateVectorY", PyFloat_FromDouble(src.stateVectorY));

                return state;
            }
    };

    template <> struct type_caster<ActionPoseState>{
        public:
            PYBIND11_TYPE_CASTER(ActionPoseState, _("ActionPoseState"));

            // conversion from C++ to Python
            static handle cast(ActionPoseState src, return_value_policy /* policy */, handle /* parent */){
                
                PyObject * state = PyDict_New();
                PyDict_SetItemString(state, "type", PyLong_FromLong(src.type));
                PyDict_SetItemString(state, "path", PyUnicode_FromString(src.path));
                PyDict_SetItemString(state, "isActive", PyBool_FromLong(src.isActive));
                
                PyObject * position = PyDict_New();
                PyDict_SetItemString(position, "x", PyFloat_FromDouble(src.pose.position.x));
                PyDict_SetItemString(position, "y", PyFloat_FromDouble(src.pose.position.y));
                PyDict_SetItemString(position, "z", PyFloat_FromDouble(src.pose.position.z));

                PyObject * orientation = PyDict_New();
                PyDict_SetItemString(orientation, "x", PyFloat_FromDouble(src.pose.orientation.x));
                PyDict_SetItemString(orientation, "y", PyFloat_FromDouble(src.pose.orientation.y));
                PyDict_SetItemString(orientation, "z", PyFloat_FromDouble(src.pose.orientation.z));
                PyDict_SetItemString(orientation, "w", PyFloat_FromDouble(src.pose.orientation.w));

                PyObject * pose = PyDict_New();
                PyDict_SetItemString(pose, "position", position);
                PyDict_SetItemString(pose, "orientation", orientation);

                PyDict_SetItemString(state, "pose", pose);

                return state;
            }
    };
}}


PYBIND11_MODULE(xrlib_p, m){
    py::class_<OpenXrApplication>(m, "OpenXrApplication")
        .def(py::init<>())
        // utils
        .def("destroy", &OpenXrApplication::destroy)
        .def("isSessionRunning", &OpenXrApplication::isSessionRunning)
        .def("getViewConfigurationViews", &OpenXrApplication::getViewConfigurationViews)
        .def("getViewConfigurationViewsSize", &OpenXrApplication::getViewConfigurationViewsSize)
        // setup app
        .def("createInstance", &OpenXrApplication::createInstance)
        .def("getSystem", [](OpenXrApplication &m, int formFactor, int blendMode, int configurationType){
                return m.getSystem(XrFormFactor(formFactor), XrEnvironmentBlendMode(blendMode), XrViewConfigurationType(configurationType));
            })
        .def("createSession", &OpenXrApplication::createSession)
        // actions
        .def("addAction", [](OpenXrApplication &m, string stringPath, int actionType, int referenceSpaceType){
                return m.addAction(stringPath, XrActionType(actionType), XrReferenceSpaceType(referenceSpaceType));
            })
        .def("applyHapticFeedback", [](OpenXrApplication &m, string stringPath, float amplitude, int64_t duration, float frequency){
                XrHapticVibration vibration = {XR_TYPE_HAPTIC_VIBRATION};
                vibration.amplitude = amplitude;
                vibration.duration = XrDuration(duration);
                vibration.frequency = frequency;
                return m.applyHapticFeedback(stringPath, (XrHapticBaseHeader*)&vibration);
            })
        .def("stopHapticFeedback", &OpenXrApplication::stopHapticFeedback)
        // poll data
        .def("pollEvents", [](OpenXrApplication &m){
                bool exitLoop = true;
                bool returnValue = m.pollEvents(&exitLoop); 
                return std::make_tuple(returnValue, exitLoop); 
            })
        .def("pollActions", [](OpenXrApplication &m){
                vector<ActionState> actionStates;
                bool returnValue = m.pollActions(actionStates);
                return std::make_tuple(returnValue, actionStates); 
            })
        // render
        .def("renderViews", [](OpenXrApplication &m, int referenceSpaceType){
                vector<ActionPoseState> actionPoseState;
                bool returnValue = m.renderViews(XrReferenceSpaceType(referenceSpaceType), actionPoseState);
                return std::make_tuple(returnValue, actionPoseState); 
            })
        // render utilities
        .def("setRenderCallback", &OpenXrApplication::setRenderCallbackFromFunction)
        .def("setFrames", [](OpenXrApplication &m, py::array_t<uint8_t> left, py::array_t<uint8_t> right, bool rgba){
                py::buffer_info leftInfo = left.request();
                if(m.getViewConfigurationViewsSize() == 1)
                    return m.setFrameByIndex(0, leftInfo.shape[1], leftInfo.shape[0], leftInfo.ptr, rgba);
                else if(m.getViewConfigurationViewsSize() == 2){
                    py::buffer_info rightInfo = right.request();
                    bool status = m.setFrameByIndex(0, leftInfo.shape[1], leftInfo.shape[0], leftInfo.ptr, rgba);
                    return status && m.setFrameByIndex(1, rightInfo.shape[1], rightInfo.shape[0], rightInfo.ptr, rgba);
                }
                return false;
            });
}