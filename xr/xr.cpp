#define XR_USE_PLATFORM_XLIB

// #define XR_USE_GRAPHICS_API_VULKAN
#define XR_USE_GRAPHICS_API_OPENGL

// Vulkan libraries
#ifdef XR_USE_GRAPHICS_API_VULKAN
#define APP_USE_VULKAN2
#define STB_IMAGE_IMPLEMENTATION
#include <vulkan/vulkan.h>
#include "stb_image.h"
#endif

// OpenGL libraries
#ifdef XR_USE_GRAPHICS_API_OPENGL
#define GL_GLEXT_PROTOTYPES
#define GL3_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#ifdef APPLICATION_IMAGE
#include <SDL2/SDL_image.h>
#endif
#endif

#include <vector>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <algorithm>
#include <functional> 
using namespace std;

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_reflection.h>
	
#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof((x)[0]))
#endif

// generate stringify functions for OpenXR enumerations
#define ENUM_CASE_STR(name, val) case name: return #name;
#define MAKE_TO_STRING_FUNC(enumType)                  \
    inline const char* _enum_to_string(enumType e) {   \
        switch (e) {                                   \
            XR_LIST_ENUM_##enumType(ENUM_CASE_STR)     \
            default: return "Unknown " #enumType;      \
        }                                              \
    }

MAKE_TO_STRING_FUNC(XrReferenceSpaceType);
MAKE_TO_STRING_FUNC(XrViewConfigurationType);
MAKE_TO_STRING_FUNC(XrEnvironmentBlendMode);
MAKE_TO_STRING_FUNC(XrSessionState);
MAKE_TO_STRING_FUNC(XrResult);
MAKE_TO_STRING_FUNC(XrFormFactor);


struct ActionState{
  	XrActionType type;
	const char * path;
	bool isActive;
    bool stateBool;
    float stateFloat;
    float stateVectorX;
    float stateVectorY;
};

struct SwapchainHandler{
  	XrSwapchain handle;
    int32_t width;
    int32_t height;
	uint32_t length;
#ifdef XR_USE_GRAPHICS_API_VULKAN
	vector<XrSwapchainImageVulkan2KHR> images;
#endif
#ifdef XR_USE_GRAPHICS_API_OPENGL
	vector<XrSwapchainImageOpenGLKHR> images;
#endif
};

vector<const char*> cast_to_vector_char_p(const vector<string> & input_list){
	vector<const char*> output_list;
	for (size_t i = 0; i < input_list.size(); i++)
		output_list.push_back(input_list[i].data());
	return output_list;
}

bool xrCheckResult(XrInstance xr_instance, XrResult xr_result, string message = ""){
	if(XR_SUCCEEDED(xr_result))
		return true;

	if(xr_instance != NULL){
		char xr_result_as_string[XR_MAX_RESULT_STRING_SIZE];
		xrResultToString(xr_instance, xr_result, xr_result_as_string);
		if(!message.empty())
			std::cout << "FAILED with code: " << xr_result << " (" << xr_result_as_string << "). " << message << std::endl;
		else
			std::cout << "FAILED with code: " << xr_result << " (" << xr_result_as_string << ")" << std::endl;
	}
	else{
		if(!message.empty())
			std::cout << "FAILED with code: " << xr_result << " (https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#return-codes). " << message << std::endl;
		else
			std::cout << "FAILED with code: " << xr_result << " (https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#return-codes)" << std::endl;
	}
	return false;
}




// Vulkan graphics API
#ifdef XR_USE_GRAPHICS_API_VULKAN
class VulkanHandler{

private:
	XrResult xr_result;
	VkResult vk_result;
	
	VkInstance vk_instance;
	VkPhysicalDevice vk_physicalDevice = VK_NULL_HANDLE;
	VkDevice vk_logicalDevice;
	uint32_t vk_queueFamilyIndex;
	VkQueue vk_graphicsQueue;
	VkCommandPool vk_cmdPool;
	VkPipelineCache vk_pipelineCache;

	bool getRequirements(XrInstance xr_instance, XrSystemId xr_system_id);
	bool defineLayers(vector<const char*> requestedLayers, vector<const char*> & enabledLayers);
	bool defineExtensions(vector<const char*> requestedExtensions, vector<const char*> & enabledExtensions);


public:
	VulkanHandler();
	~VulkanHandler();

	void createInstance(XrInstance xr_instance, XrSystemId xr_system_id);
	void getPhysicalDevice(XrInstance xr_instance, XrSystemId xr_system_id);
	void createLogicalDevice(XrInstance xr_instance, XrSystemId xr_system_id);

	VkInstance getInstance(){ return vk_instance; }
	VkPhysicalDevice getPhysicalDevice(){ return vk_physicalDevice; }
	VkDevice getLogicalDevice(){ return vk_logicalDevice; }
	uint32_t getQueueFamilyIndex(){ return vk_queueFamilyIndex; }
	void renderView(const XrCompositionLayerProjectionView &, const XrSwapchainImageBaseHeader *, int64_t);

	uint32_t getSupportedSwapchainSampleCount(XrViewConfigurationView){ return VK_SAMPLE_COUNT_1_BIT; }

	void loadImageFromFile();
};

VulkanHandler::VulkanHandler(){
	loadImageFromFile();
}

VulkanHandler::~VulkanHandler(){
	// vkDestroyInstance(vk_instance, nullptr);
	// vkDestroyDevice(vk_logicalDevice, nullptr);
}

void VulkanHandler::loadImageFromFile(){
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
}

bool VulkanHandler::getRequirements(XrInstance xr_instance, XrSystemId xr_system_id){
#ifdef APP_USE_VULKAN2
	PFN_xrGetVulkanGraphicsRequirements2KHR pfn_xrGetVulkanGraphicsRequirements2KHR = nullptr;
	xrGetInstanceProcAddr(xr_instance, "xrGetVulkanGraphicsRequirements2KHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfn_xrGetVulkanGraphicsRequirements2KHR));
	XrGraphicsRequirementsVulkan2KHR graphicsRequirement = {XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR};
	xr_result = pfn_xrGetVulkanGraphicsRequirements2KHR(xr_instance, xr_system_id, &graphicsRequirement);
	if(!xrCheckResult(NULL, xr_result, "PFN_xrGetVulkanGraphicsRequirementsKHR"))
		return false;
	std::cout << "Vulkan (Vulkan2) requirements" << std::endl;
#else
	PFN_xrGetVulkanGraphicsRequirementsKHR pfn_xrGetVulkanGraphicsRequirementsKHR = nullptr;
	xrGetInstanceProcAddr(xr_instance, "xrGetVulkanGraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfn_xrGetVulkanGraphicsRequirementsKHR));
	XrGraphicsRequirementsVulkanKHR graphicsRequirement = {XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR};
	xr_result = pfn_xrGetVulkanGraphicsRequirementsKHR(xr_instance, xr_system_id, &graphicsRequirement);
	if(!xrCheckResult(NULL, xr_result, "PFN_xrGetVulkanGraphicsRequirementsKHR"))
		return false;
	std::cout << "Vulkan (Vulkan) requirements" << std::endl;
#endif

	std::cout << "  |-- min API version: " << XR_VERSION_MAJOR(graphicsRequirement.minApiVersionSupported) << "." << XR_VERSION_MINOR(graphicsRequirement.minApiVersionSupported) << "." << XR_VERSION_PATCH(graphicsRequirement.minApiVersionSupported) << std::endl;
	std::cout << "  |-- max API version: " << XR_VERSION_MAJOR(graphicsRequirement.maxApiVersionSupported) << "." << XR_VERSION_MINOR(graphicsRequirement.maxApiVersionSupported) << "." << XR_VERSION_PATCH(graphicsRequirement.maxApiVersionSupported) << std::endl;
	return true;
}

bool VulkanHandler::defineLayers(vector<const char*> requestedLayers, vector<const char*> & enabledLayers){
	uint32_t propertyCount = 0;

	vkEnumerateInstanceLayerProperties(&propertyCount, nullptr);
	std::vector<VkLayerProperties> layerProperties(propertyCount);
	vkEnumerateInstanceLayerProperties(&propertyCount, layerProperties.data());

	std::cout << "Vulkan layers available (" << layerProperties.size() << ")" << std::endl;
	for (size_t i = 0; i < layerProperties.size(); i++){
		std::cout << "  |-- " << layerProperties[i].layerName << std::endl;
		for (size_t j = 0; j < requestedLayers.size(); j++)
			if (strcmp(layerProperties[i].layerName, requestedLayers[j]) == 0){
				enabledLayers.push_back(requestedLayers[j]);
				std::cout << "  |   (requested)" << std::endl;
				break;
			}
	}
	return true;
}

bool VulkanHandler::defineExtensions(vector<const char*> requestedExtensions, vector<const char*> & enabledExtensions){
	uint32_t propertyCount = 0;

	vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, nullptr);
	vector<VkExtensionProperties> extensionsProperties(propertyCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, extensionsProperties.data());

	std::cout << "Vulkan extension properties (" << extensionsProperties.size() << ")" << std::endl;
	for (size_t i = 0; i < extensionsProperties.size(); i++){
		std::cout << "  |-- " << extensionsProperties[i].extensionName << std::endl;
		for (size_t j = 0; j < requestedExtensions.size(); j++)
			if (strcmp(extensionsProperties[i].extensionName, requestedExtensions[j]) == 0){
				enabledExtensions.push_back(requestedExtensions[j]);
				std::cout << "  |   (requested)" << std::endl;
				break;
			}
	}
	return true;
}

void VulkanHandler::createInstance(XrInstance xr_instance, XrSystemId xr_system_id){
	// requirements
	getRequirements(xr_instance, xr_system_id);

	// layers
	vector<const char*> enabledLayers;
	vector<const char*> requestedLayers = { "VK_LAYER_LUNARG_core_validation" };
	// TODO: see why it crashes the application 
	// defineLayers(requestedLayers, enabledLayers);

	// extensions
	vector<const char*> enabledExtensions;
	vector<const char*> requestedExtensions = { "VK_EXT_debug_report" };
	defineExtensions(requestedExtensions, enabledExtensions);

	VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
	appInfo.pApplicationName = "Isaac Sim (VR)";
	appInfo.applicationVersion = 1;
	appInfo.pEngineName = "Isaac Sim (VR)";
	appInfo.engineVersion = 1;
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo instanceInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.enabledLayerCount = (uint32_t)enabledLayers.size();
	instanceInfo.ppEnabledLayerNames = enabledLayers.empty() ? nullptr : enabledLayers.data();
	instanceInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
	instanceInfo.ppEnabledExtensionNames = enabledExtensions.empty() ? nullptr : enabledExtensions.data();

	XrVulkanInstanceCreateInfoKHR createInfo = {XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR};
	createInfo.systemId = xr_system_id;
	createInfo.pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
	createInfo.vulkanCreateInfo = &instanceInfo;
	createInfo.vulkanAllocator = nullptr;

	PFN_xrCreateVulkanInstanceKHR pfn_xrCreateVulkanInstanceKHR = nullptr;
	xr_result = xrGetInstanceProcAddr(xr_instance, "xrCreateVulkanInstanceKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfn_xrCreateVulkanInstanceKHR));
	xr_result = pfn_xrCreateVulkanInstanceKHR(xr_instance, &createInfo, &vk_instance, &vk_result);
}

void VulkanHandler::getPhysicalDevice(XrInstance xr_instance, XrSystemId xr_system_id){
	// enumerate device
	uint32_t propertyCount = 0;
	vkEnumeratePhysicalDevices(vk_instance, &propertyCount, nullptr);
	if(!propertyCount)
		throw std::runtime_error("Failed to find GPUs with Vulkan support");

	vector<VkPhysicalDevice> devices(propertyCount);
	vkEnumeratePhysicalDevices(vk_instance, &propertyCount, devices.data());

	// get physical device
	XrVulkanGraphicsDeviceGetInfoKHR deviceGetInfo = {XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR};
	deviceGetInfo.systemId = xr_system_id;
	deviceGetInfo.vulkanInstance = vk_instance;
	deviceGetInfo.next = nullptr;

	PFN_xrGetVulkanGraphicsDevice2KHR pfn_xrGetVulkanGraphicsDevice2KHR = nullptr;
	xr_result = xrGetInstanceProcAddr(xr_instance, "xrGetVulkanGraphicsDevice2KHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfn_xrGetVulkanGraphicsDevice2KHR));
	xr_result = pfn_xrGetVulkanGraphicsDevice2KHR(xr_instance, &deviceGetInfo, &vk_physicalDevice);

	std::cout << "Vulkan physical devices (" << devices.size() << ")" << std::endl;
	for(const auto& device : devices){
		if(device == vk_physicalDevice){
			std::cout << "  |-- handle: " << device << std::endl;
			std::cout << "  |   (selected)"<< std::endl;
		}
		else
			std::cout << "  |-- handle: " << device << std::endl;
	}
}

void VulkanHandler::createLogicalDevice(XrInstance xr_instance, XrSystemId xr_system_id){
	uint32_t propertyCount = 0;
	float queuePriorities = 0;

	VkDeviceQueueCreateInfo queueInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
	queueInfo.queueCount = 1;
	queueInfo.pQueuePriorities = &queuePriorities;

	// queue families index
	vkGetPhysicalDeviceQueueFamilyProperties(vk_physicalDevice, &propertyCount, nullptr);
	vector<VkQueueFamilyProperties> queueFamilyProps(propertyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(vk_physicalDevice, &propertyCount, &queueFamilyProps[0]);

	for (uint32_t i = 0; i < propertyCount; ++i)
		if ((queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0u){
			queueInfo.queueFamilyIndex = i;
			vk_queueFamilyIndex = queueInfo.queueFamilyIndex;
			break;
		}

	vector<const char*> deviceExtensions;
	deviceExtensions.push_back("VK_KHR_external_memory");
    deviceExtensions.push_back("VK_KHR_external_memory_fd");
    deviceExtensions.push_back("VK_KHR_external_semaphore");
    deviceExtensions.push_back("VK_KHR_external_semaphore_fd");
    deviceExtensions.push_back("VK_KHR_get_memory_requirements2");

	VkPhysicalDeviceFeatures features = {};
	features.samplerAnisotropy = true;

	VkDeviceCreateInfo deviceInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.pQueueCreateInfos = &queueInfo;
	deviceInfo.enabledLayerCount = 0;
	deviceInfo.ppEnabledLayerNames = nullptr;
	deviceInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
	deviceInfo.ppEnabledExtensionNames = deviceExtensions.empty() ? nullptr : deviceExtensions.data();
	deviceInfo.pEnabledFeatures = &features;

	XrVulkanDeviceCreateInfoKHR createInfo = {XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR};
	createInfo.systemId = xr_system_id;
	createInfo.pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
	createInfo.vulkanCreateInfo = &deviceInfo;
	createInfo.vulkanPhysicalDevice = vk_physicalDevice;
	createInfo.vulkanAllocator = nullptr;
	createInfo.createFlags = 0;
	createInfo.next = nullptr;

	PFN_xrCreateVulkanDeviceKHR pfn_xrCreateVulkanDeviceKHR = nullptr;
	xr_result = xrGetInstanceProcAddr(xr_instance, "xrCreateVulkanDeviceKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfn_xrCreateVulkanDeviceKHR));
	xr_result = pfn_xrCreateVulkanDeviceKHR(xr_instance, &createInfo, &vk_logicalDevice, &vk_result);

	// get queue
	vkGetDeviceQueue(vk_logicalDevice, vk_queueFamilyIndex, 0, &vk_graphicsQueue);
	// m_memAllocator.Init(m_vkPhysicalDevice, m_vkDevice);

	VkPipelineCacheCreateInfo info = {VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    vkCreatePipelineCache(vk_logicalDevice, &info, nullptr, &vk_pipelineCache);

	VkCommandPoolCreateInfo cmdPoolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	cmdPoolInfo.queueFamilyIndex = vk_queueFamilyIndex;
    vk_result =  vkCreateCommandPool(vk_logicalDevice, &cmdPoolInfo, nullptr, &vk_cmdPool);
}

void VulkanHandler::renderView(const XrCompositionLayerProjectionView & layerView, const XrSwapchainImageBaseHeader * swapchainImage, int64_t swapchainFormat){
	std::cout << "layerView.subImage.imageArrayIndex: " << layerView.subImage.imageArrayIndex << std::endl;
}

#endif

// OpenGL graphics API
#ifdef XR_USE_GRAPHICS_API_OPENGL

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam){
	std::cout << "GL CALLBACK: " << (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "") << " type = 0x" << type << ", severity = 0x" << severity << ", message = " << message << std::endl;
}

static const char* glslShaderVertex = R"_(
    #version 410
	out vec2 v_tex;

	const vec2 pos[4]=vec2[4](vec2(-1.0, 1.0),
								vec2(-1.0,-1.0),
								vec2( 1.0, 1.0),
								vec2( 1.0,-1.0));

	void main(){
		v_tex=0.5*pos[gl_VertexID] + vec2(0.5);
		gl_Position=vec4(pos[gl_VertexID], 0.0, 1.0);
	}
)_";

static const char* glslShaderFragment = R"_(
	#version 410

	in vec2 v_tex;
	uniform sampler2D texSampler;

	out vec4 color;

	void main(){
		color=texture(texSampler, v_tex);
	}
)_";

class OpenGLHandler{
private:
	XrResult xr_result;

	SDL_Window * sdl_window;
	SDL_GLContext gl_context;
	PFNGLBLITNAMEDFRAMEBUFFERPROC _glBlitNamedFramebuffer;

	GLuint vao;
	GLuint program;
	GLuint texture;
	GLuint swapchainFramebuffer;

	bool checkShader(GLuint);
	bool checkProgram(GLuint);

	void loadTexture(string, GLuint *);

public:
	OpenGLHandler();
	~OpenGLHandler();

	bool getRequirements(XrInstance xr_instance, XrSystemId xr_system_id);
	bool initGraphicsBinding(Display** xDisplay, uint32_t* visualid, GLXFBConfig* glxFBConfig, GLXDrawable* glxDrawable, GLXContext* glxContext, int witdh, int height);
	bool initResources(XrInstance xr_instance, XrSystemId xr_system_id);

	void acquireContext(XrGraphicsBindingOpenGLXlibKHR, string);
	void renderView(const XrCompositionLayerProjectionView &, const XrSwapchainImageBaseHeader *, int64_t);
	void renderViewFromImage(const XrCompositionLayerProjectionView &, const XrSwapchainImageBaseHeader *, int64_t, int, int, void *, bool);

	uint32_t getSupportedSwapchainSampleCount(XrViewConfigurationView){ return 1; }
};

OpenGLHandler::OpenGLHandler()
{
}

OpenGLHandler::~OpenGLHandler()
{
}

void OpenGLHandler::loadTexture(string path, GLuint * textureId){
#ifdef APPLICATION_IMAGE
	int mode = GL_RGB;
	SDL_Surface *tex = IMG_Load(path.c_str());
    
	if(tex->format->BitsPerPixel >= 4)
        mode = GL_RGBA;
    else
        mode = GL_RGB;

    glGenTextures(1, textureId);
    glBindTexture(GL_TEXTURE_2D, *textureId);

    glTexImage2D(GL_TEXTURE_2D, 0, mode, tex->w, tex->h, 0, mode, GL_UNSIGNED_BYTE, tex->pixels);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#endif
}

bool OpenGLHandler::checkShader(GLuint shader){
	GLint r = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &r);
	if(r == GL_FALSE){
		GLchar msg[4096] = {};
		GLsizei length;
		glGetShaderInfoLog(shader, sizeof(msg), &length, msg);
		std::cout << "GL SHADER: " << msg << std::endl;
		return false;
	}
	return true;
}

bool OpenGLHandler::checkProgram(GLuint prog) {
	GLint r = 0;
	glGetProgramiv(prog, GL_LINK_STATUS, &r);
	if(r == GL_FALSE){
		GLchar msg[4096] = {};
		GLsizei length;
		glGetProgramInfoLog(prog, sizeof(msg), &length, msg);
		std::cout << "GL SHADER: " << msg << std::endl;
		return false;
	}
	return true;
}

void OpenGLHandler::acquireContext(XrGraphicsBindingOpenGLXlibKHR graphicsBinding, string message){
	GLXContext context = glXGetCurrentContext();
	if(context != graphicsBinding.glxContext){
		// std::cout << "glxContext changed (" << context << " != " << graphicsBinding.glxContext << ") in "<< message << std::endl;
		glXMakeCurrent(graphicsBinding.xDisplay, graphicsBinding.glxDrawable, graphicsBinding.glxContext);
	}
}

bool OpenGLHandler::initGraphicsBinding(Display** xDisplay, uint32_t* visualid, GLXFBConfig* glxFBConfig, GLXDrawable* glxDrawable, GLXContext* glxContext, int witdh, int height){
	if(SDL_Init(SDL_INIT_VIDEO) < 0){
		std::cout << "Unable to initialize SDL" << std::endl;
		return false;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);

	// create our window centered at half the VR resolution
	sdl_window = SDL_CreateWindow("Omniverse (VR)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, witdh / 2, height / 2, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (!sdl_window){
		std::cout << "Unable to create SDL window" << std::endl;
		return false;
	}

	gl_context = SDL_GL_CreateContext(sdl_window);
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);

	SDL_GL_SetSwapInterval(0);

	_glBlitNamedFramebuffer = (PFNGLBLITNAMEDFRAMEBUFFERPROC)glXGetProcAddressARB((GLubyte*)"glBlitNamedFramebuffer");

	*xDisplay = XOpenDisplay(NULL);
	*glxContext = glXGetCurrentContext();
	*glxDrawable = glXGetCurrentDrawable();

	SDL_HideWindow(sdl_window);
	return true;
}

bool OpenGLHandler::getRequirements(XrInstance xr_instance, XrSystemId xr_system_id){
	PFN_xrGetOpenGLGraphicsRequirementsKHR pfn_xrGetOpenGLGraphicsRequirementsKHR = nullptr;
	xr_result = xrGetInstanceProcAddr(xr_instance, "xrGetOpenGLGraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfn_xrGetOpenGLGraphicsRequirementsKHR));
	if(!xrCheckResult(NULL, xr_result, "xrGetOpenGLGraphicsRequirementsKHR (xrGetInstanceProcAddr)"))
		return false;
	XrGraphicsRequirementsOpenGLKHR graphicsRequirement = {XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
	xr_result = pfn_xrGetOpenGLGraphicsRequirementsKHR(xr_instance, xr_system_id, &graphicsRequirement);
	if(!xrCheckResult(NULL, xr_result, "xrGetOpenGLGraphicsRequirementsKHR"))
		return false;

	std::cout << "OpenGL requirements" << std::endl;
	std::cout << "  |-- min API version: " << XR_VERSION_MAJOR(graphicsRequirement.minApiVersionSupported) << "." << XR_VERSION_MINOR(graphicsRequirement.minApiVersionSupported) << "." << XR_VERSION_PATCH(graphicsRequirement.minApiVersionSupported) << std::endl;
	std::cout << "  |-- max API version: " << XR_VERSION_MAJOR(graphicsRequirement.maxApiVersionSupported) << "." << XR_VERSION_MINOR(graphicsRequirement.maxApiVersionSupported) << "." << XR_VERSION_PATCH(graphicsRequirement.maxApiVersionSupported) << std::endl;
	return true;
}

bool OpenGLHandler::initResources(XrInstance xr_instance, XrSystemId xr_system_id){
	glGenTextures(1, &texture);
	glGenVertexArrays(1, &vao);
	glGenFramebuffers(1, &swapchainFramebuffer);

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &glslShaderVertex, nullptr);
	glCompileShader(vertexShader);
	if(!checkShader(vertexShader))
		return false;

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &glslShaderFragment, nullptr);
	glCompileShader(fragmentShader);
	if(!checkShader(fragmentShader))
		return false;

	program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	if(!checkProgram(program))
		return false;

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	return true;
}

void OpenGLHandler::renderView(const XrCompositionLayerProjectionView & layerView, const XrSwapchainImageBaseHeader * swapchainImage, int64_t swapchainFormat){
        glBindFramebuffer(GL_FRAMEBUFFER, swapchainFramebuffer);

        const uint32_t colorTexture = reinterpret_cast<const XrSwapchainImageOpenGLKHR*>(swapchainImage)->image;

        glViewport(static_cast<GLint>(layerView.subImage.imageRect.offset.x),
                   static_cast<GLint>(layerView.subImage.imageRect.offset.y),
                   static_cast<GLsizei>(layerView.subImage.imageRect.extent.width),
                   static_cast<GLsizei>(layerView.subImage.imageRect.extent.height));
 		
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

        glUseProgram(program);
		glBindTexture(GL_TEXTURE_2D, texture);
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glBindTexture(GL_TEXTURE_2D, 0);
		glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// render to window
		int width, height;
		SDL_GetWindowSize(sdl_window, &width, &height);
		glViewport(0, 0, width, height);

		glUseProgram(program);
		glBindTexture(GL_TEXTURE_2D, texture);
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // swap window every other eye
        static int everyOther = 0;
        if((everyOther++ & 1) != 0)
			SDL_GL_SwapWindow(sdl_window);

		glBindTexture(GL_TEXTURE_2D, 0);
		glUseProgram(0);
}

void OpenGLHandler::renderViewFromImage(const XrCompositionLayerProjectionView & layerView, const XrSwapchainImageBaseHeader * swapchainImage, int64_t swapchainFormat, int frameWidth, int frameHeight, void * frameData, bool rgba){
		// load texture
		glBindTexture(GL_TEXTURE_2D, texture);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frameWidth, frameHeight, 0, rgba ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, frameData);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		// render to hmd
        glBindFramebuffer(GL_FRAMEBUFFER, swapchainFramebuffer);

        const uint32_t colorTexture = reinterpret_cast<const XrSwapchainImageOpenGLKHR*>(swapchainImage)->image;

        glViewport(static_cast<GLint>(layerView.subImage.imageRect.offset.x),
                   static_cast<GLint>(layerView.subImage.imageRect.offset.y),
                   static_cast<GLsizei>(layerView.subImage.imageRect.extent.width),
                   static_cast<GLsizei>(layerView.subImage.imageRect.extent.height));
 		
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

        glUseProgram(program);
		glBindTexture(GL_TEXTURE_2D, texture);
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glBindTexture(GL_TEXTURE_2D, 0);
		glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// // render to window
		// int width, height;
		// SDL_GetWindowSize(sdl_window, &width, &height);
		// glViewport(0, 0, width, height);

		// glUseProgram(program);
		// glBindTexture(GL_TEXTURE_2D, texture);
		// glBindVertexArray(vao);
		// glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // // swap window every other eye
        // static int everyOther = 0;
        // if((everyOther++ & 1) != 0)
		// 	SDL_GL_SwapWindow(sdl_window);

		// glBindTexture(GL_TEXTURE_2D, 0);
		// glUseProgram(0);
}

#endif




class OpenXrApplication{
private:
	XrResult xr_result;

	XrInstance xr_instance = {};
	XrSystemId xr_system_id = XR_NULL_SYSTEM_ID;
	XrSession xr_session = {};

	XrSpace xr_space_view = {};
	XrSpace xr_space_local = {};
	XrSpace xr_space_stage = {};

	// actions
	XrActionSet xr_action_set;

	vector<XrSpace> xr_space_actions_pose;

	vector<XrAction> xr_actions_boolean;
	vector<XrAction> xr_actions_float;
	vector<XrAction> xr_actions_vector2f;
	vector<XrAction> xr_actions_pose;
	vector<XrAction> xr_actions_vibration;

	vector<XrPath> xr_action_paths_boolean;
	vector<XrPath> xr_action_paths_float;
	vector<XrPath> xr_action_paths_vector2f;
	vector<XrPath> xr_action_paths_pose;
	vector<XrPath> xr_action_paths_vibration;

	vector<string> xr_action_string_paths_boolean;
	vector<string> xr_action_string_paths_float;
	vector<string> xr_action_string_paths_vector2f;
	vector<string> xr_action_string_paths_pose;
	vector<string> xr_action_string_paths_vibration;

	vector<SwapchainHandler> xr_swapchains_handlers;
	vector<XrViewConfigurationView> xr_view_configuration_views;

	bool framesRGBA;
	vector<int> framesWidth;
	vector<int> framesHeight;
	vector<void*> framesData;
	void (*renderCallback)(int, XrView*, XrViewConfigurationView*);
	function<void(int, vector<XrView>, vector<XrViewConfigurationView>)> renderCallbackFunction;

	bool flagSessionRunning = false;

	// config
	XrEnvironmentBlendMode environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_MAX_ENUM;
	XrViewConfigurationType configViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM;

#ifdef XR_USE_GRAPHICS_API_VULKAN
	XrGraphicsBindingVulkan2KHR xr_graphics_binding = {XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR};
	VulkanHandler xr_graphics_handler;
#endif
#ifdef XR_USE_GRAPHICS_API_OPENGL
	XrGraphicsBindingOpenGLXlibKHR xr_graphics_binding = {XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR};
	OpenGLHandler xr_graphics_handler;
#endif

	bool defineLayers(vector<string>, vector<string> &);
	bool defineExtensions(vector<string>, vector<string> &);

	bool acquireInstanceProperties();
	bool acquireSystemProperties();
	bool acquireBlendModes(XrEnvironmentBlendMode);
	bool acquireViewConfiguration(XrViewConfigurationType);	

	bool defineReferenceSpaces();
	bool defineSessionSpaces();
	bool defineSwapchains();

	void defineInteractionProfileBindings(vector<XrActionSuggestedBinding> &, vector<string>);
	void cleanFrames(){
		// for(size_t i = 0; i < framesData.size(); i++){
		// 	framesData[i] = nullptr;
		// 	framesWidth[i] = 0;
		// 	framesHeight[i] = 0;
		// }
	};

public:
	OpenXrApplication();
	~OpenXrApplication();

	bool createInstance(string, string, vector<string>, vector<string>);
	bool getSystem(XrFormFactor, XrEnvironmentBlendMode, XrViewConfigurationType); 
	bool createSession();

	bool addAction(string, XrActionType);
	bool suggestInteractionProfileBindings();

	bool pollEvents(bool *);
	bool pollActions(vector<ActionState> &);
	bool renderViews(XrReferenceSpaceType);

	bool setFrameByIndex(int, int, int, void *, bool);
	void setRenderCallbackFromPointer(void (*callback)(int, XrView*, XrViewConfigurationView*)){ renderCallback = callback; };
	void setRenderCallbackFromFunction(function<void(int, vector<XrView>, vector<XrViewConfigurationView>)> &callback){ renderCallbackFunction = callback; };

	bool isSessionRunning(){ return flagSessionRunning; }
	int getViewConfigurationViewsSize(){ return xr_view_configuration_views.size(); }
	vector<XrViewConfigurationView> getViewConfigurationViews(){ return xr_view_configuration_views; }
};

OpenXrApplication::OpenXrApplication(){
	renderCallback = nullptr;
	renderCallbackFunction = nullptr;
}

OpenXrApplication::~OpenXrApplication(){
	if(xr_instance != NULL){
		xrDestroySpace(xr_space_view);
		xrDestroySpace(xr_space_local);
		xrDestroySpace(xr_space_stage);

		for(size_t i = 0; i < xr_space_actions_pose.size(); i++)
			xrDestroySpace(xr_space_actions_pose[i]);

		xrDestroySession(xr_session);
		xrDestroyInstance(xr_instance);
	}
}

bool OpenXrApplication::defineLayers(vector<string> requestedApiLayers, vector<string> & enabledApiLayers){
	uint32_t propertyCountOutput;
	xr_result = xrEnumerateApiLayerProperties(0, &propertyCountOutput, nullptr);
	if(!xrCheckResult(NULL, xr_result, "xrEnumerateApiLayerProperties"))
		return false;
	vector<XrApiLayerProperties> apiLayerProperties(propertyCountOutput, {XR_TYPE_API_LAYER_PROPERTIES});
	xr_result = xrEnumerateApiLayerProperties(propertyCountOutput, &propertyCountOutput, apiLayerProperties.data());
	if(!xrCheckResult(NULL, xr_result, "xrEnumerateApiLayerProperties"))
		return false;

	std::cout << "OpenXR API layers (" << apiLayerProperties.size() << ")" << std::endl;
	for(size_t i = 0; i < apiLayerProperties.size(); i++){
		std::cout << "  |-- " << apiLayerProperties[i].layerName << std::endl;
		for(size_t j = 0; j < requestedApiLayers.size(); j++)
			if(!requestedApiLayers[j].compare(apiLayerProperties[i].layerName)){
				enabledApiLayers.push_back(requestedApiLayers[j]);
				std::cout << "  |   (requested)" << std::endl;
				break;
			}
	}

	// check for unavailable layers
	if(requestedApiLayers.size() != enabledApiLayers.size()){
		bool used = false;
		std::cout << "Unavailable OpenXR API layers" << std::endl;
		for(size_t i = 0; i < requestedApiLayers.size(); i++){
			used = false;
			for(size_t j = 0; j < enabledApiLayers.size(); j++)
				if(!requestedApiLayers[i].compare(enabledApiLayers[j])){
					used = true;
					break;
				}
			if(!used)
				std::cout << "  |-- " << requestedApiLayers[i] << std::endl;
		}
		return false;
	}
	return true;
}

bool OpenXrApplication::defineExtensions(vector<string> requestedExtensions, vector<string> & enabledExtensions){
	uint32_t propertyCountOutput;
	xr_result = xrEnumerateInstanceExtensionProperties(nullptr, 0, &propertyCountOutput, nullptr);
	if(!xrCheckResult(NULL, xr_result, "xrEnumerateInstanceExtensionProperties"))
		return false;
	vector<XrExtensionProperties> extensionProperties(propertyCountOutput, {XR_TYPE_EXTENSION_PROPERTIES});
	xr_result = xrEnumerateInstanceExtensionProperties(nullptr, propertyCountOutput, &propertyCountOutput, extensionProperties.data());
	if(!xrCheckResult(NULL, xr_result, "xrEnumerateInstanceExtensionProperties"))
		return false;
	
	std::cout << "OpenXR extensions (" << extensionProperties.size() << ")" << std::endl;
	for(size_t i = 0; i < extensionProperties.size(); i++){
		std::cout << "  |-- " << extensionProperties[i].extensionName << std::endl;
		for(size_t j = 0; j < requestedExtensions.size(); j++)
			if(!requestedExtensions[j].compare(extensionProperties[i].extensionName)){
				enabledExtensions.push_back(requestedExtensions[j]);
				std::cout << "  |   (requested)" << std::endl;
				break;
			}
	}

	// check for unavailable extensions
	if(requestedExtensions.size() != enabledExtensions.size()){
		bool used = false;
		std::cout << "Unavailable OpenXR extensions" << std::endl;
		for(size_t i = 0; i < requestedExtensions.size(); i++){
			used = false;
			for(size_t j = 0; j < enabledExtensions.size(); j++)
				if(!requestedExtensions[i].compare(enabledExtensions[j])){
					used = true;
					break;
				}
			if(!used)
				std::cout << "  |-- " << requestedExtensions[i] << std::endl;
		}
		return false;
	}
	return true;
}

bool OpenXrApplication::acquireInstanceProperties(){
	XrInstanceProperties instanceProperties = {XR_TYPE_INSTANCE_PROPERTIES};
	xr_result = xrGetInstanceProperties(xr_instance, &instanceProperties);
	if(!xrCheckResult(xr_instance, xr_result, "xrGetInstanceProperties"))
		return false;

	std::cout << "Runtime" << std::endl;
	std::cout << "  |-- name: " << instanceProperties.runtimeName << std::endl;
	std::cout << "  |-- version: " << XR_VERSION_MAJOR(instanceProperties.runtimeVersion) << "." << XR_VERSION_MINOR(instanceProperties.runtimeVersion) << "." << XR_VERSION_PATCH(instanceProperties.runtimeVersion) << std::endl;
	return true;
}

bool OpenXrApplication::acquireSystemProperties(){
	XrSystemProperties systemProperties = {XR_TYPE_SYSTEM_PROPERTIES};
	xr_result = xrGetSystemProperties(xr_instance, xr_system_id, &systemProperties);
	if(!xrCheckResult(xr_instance, xr_result, "xrGetSystemProperties"))
		return false;

	std::cout << "System" << std::endl;
	std::cout << "  |-- system id: " << systemProperties.systemId << std::endl;
	std::cout << "  |-- system name: " << systemProperties.systemName << std::endl;
	std::cout << "  |-- vendor id: " << systemProperties.vendorId << std::endl;
	std::cout << "  |-- max layers: " << systemProperties.graphicsProperties.maxLayerCount << std::endl;
	std::cout << "  |-- max swapchain height: " << systemProperties.graphicsProperties.maxSwapchainImageHeight << std::endl;
	std::cout << "  |-- max swapchain width: " << systemProperties.graphicsProperties.maxSwapchainImageWidth << std::endl;
	std::cout << "  |-- orientation tracking: " << systemProperties.trackingProperties.orientationTracking << std::endl;
	std::cout << "  |-- position tracking: " << systemProperties.trackingProperties.positionTracking << std::endl;
	return true;
}

bool OpenXrApplication::acquireBlendModes(XrEnvironmentBlendMode blendMode){
	uint32_t propertyCountOutput;
	xr_result = xrEnumerateEnvironmentBlendModes(xr_instance, xr_system_id, configViewConfigurationType, 0, &propertyCountOutput, nullptr);
	if(!xrCheckResult(xr_instance, xr_result, "xrEnumerateEnvironmentBlendModes"))
		return false;
	vector<XrEnvironmentBlendMode> environmentBlendModes(propertyCountOutput);
	xr_result = xrEnumerateEnvironmentBlendModes(xr_instance, xr_system_id, configViewConfigurationType, propertyCountOutput, &propertyCountOutput, environmentBlendModes.data());
	if(!xrCheckResult(xr_instance, xr_result, "xrEnumerateEnvironmentBlendModes"))
		return false;

	std::cout << "Environment blend modes (" << environmentBlendModes.size() << ")" << std::endl;
	for (size_t i = 0; i < environmentBlendModes.size(); i++){
		std::cout << "  |-- mode: " << environmentBlendModes[i] << " (https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrEnvironmentBlendMode)" << std::endl;
		if(environmentBlendModes[i] == blendMode){
			std::cout << "  |   (requested)" << std::endl;
			environmentBlendMode = blendMode;
		}
	}
	if(environmentBlendMode == XR_ENVIRONMENT_BLEND_MODE_MAX_ENUM){
		std::cout << "Unavailable blend mode: " << blendMode << " (https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrEnvironmentBlendMode)" << std::endl;
		return false;
	}
	return true;
}

bool OpenXrApplication::acquireViewConfiguration(XrViewConfigurationType configurationType){
	uint32_t propertyCountOutput;
	xr_result = xrEnumerateViewConfigurations(xr_instance, xr_system_id, 0, &propertyCountOutput, nullptr);
	if(!xrCheckResult(xr_instance, xr_result, "xrEnumerateViewConfigurations"))
		return false;
	vector<XrViewConfigurationType> viewConfigurationTypes(propertyCountOutput);
	xr_result = xrEnumerateViewConfigurations(xr_instance, xr_system_id, propertyCountOutput, &propertyCountOutput, viewConfigurationTypes.data());
	if(!xrCheckResult(xr_instance, xr_result, "xrEnumerateViewConfigurations"))
		return false;

	std::cout << "View configurations (" << viewConfigurationTypes.size() << ")" << std::endl;
	for(size_t i = 0; i < viewConfigurationTypes.size(); i++){
		std::cout << "  |-- type " << viewConfigurationTypes[i] << " (https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrViewConfigurationType)" << std::endl;
		if(viewConfigurationTypes[i] == configurationType){
			std::cout << "  |   (requested)" << std::endl;
			configViewConfigurationType = configurationType;
		}
	}
	if(configViewConfigurationType == XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM){
		std::cout << "Unavailable view configuration type: " << configurationType << " (https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrViewConfigurationType)" << std::endl;
		return false;
	}
	
	// view configuration properties
	XrViewConfigurationProperties viewConfigurationProperties = {XR_TYPE_VIEW_CONFIGURATION_PROPERTIES};
	xr_result = xrGetViewConfigurationProperties(xr_instance, xr_system_id, configViewConfigurationType, &viewConfigurationProperties);
	if(!xrCheckResult(xr_instance, xr_result, "xrGetViewConfigurationProperties"))
		return false;

	std::cout << "View configuration properties" << std::endl;
		std::cout << "  |-- configuration type: " << viewConfigurationProperties.viewConfigurationType << " (https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrViewConfigurationType)" << std::endl;
		std::cout << "  |-- fov mutable (bool): " << viewConfigurationProperties.fovMutable << std::endl;
	
	// view configuration views
	xr_result = xrEnumerateViewConfigurationViews(xr_instance, xr_system_id, configViewConfigurationType, 0, &propertyCountOutput, nullptr);
	if(!xrCheckResult(xr_instance, xr_result, "xrEnumerateViewConfigurationViews"))
		return false;
	xr_view_configuration_views.resize(propertyCountOutput, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
	xr_result = xrEnumerateViewConfigurationViews(xr_instance, xr_system_id, configViewConfigurationType, propertyCountOutput, &propertyCountOutput, xr_view_configuration_views.data());
	if(!xrCheckResult(xr_instance, xr_result, "xrEnumerateViewConfigurationViews"))
		return false;
	
	std::cout << "View configuration views (" << xr_view_configuration_views.size() << ")" << std::endl;
	for(size_t i = 0; i < xr_view_configuration_views.size(); i++){
		std::cout << "  |-- view " << i << std::endl;
		std::cout << "  |     |-- recommended resolution: " << xr_view_configuration_views[i].recommendedImageRectWidth << " x " << xr_view_configuration_views[i].recommendedImageRectHeight << std::endl;
		std::cout << "  |     |-- max resolution: " << xr_view_configuration_views[i].maxImageRectWidth << " x " << xr_view_configuration_views[i].maxImageRectHeight << std::endl;
		std::cout << "  |     |-- recommended swapchain samples: " << xr_view_configuration_views[i].recommendedSwapchainSampleCount << std::endl;
		std::cout << "  |     |-- max swapchain samples: " << xr_view_configuration_views[i].maxSwapchainSampleCount << std::endl;
	}

	// resize frame buffers
	framesData.resize(xr_view_configuration_views.size());
	framesWidth.resize(xr_view_configuration_views.size());
	framesHeight.resize(xr_view_configuration_views.size());
	cleanFrames();

	return true;
}

bool OpenXrApplication::defineReferenceSpaces(){
	// get reference spaces
	uint32_t propertyCountOutput;
	xr_result = xrEnumerateReferenceSpaces(xr_session, 0, &propertyCountOutput, nullptr);
	if(!xrCheckResult(xr_instance, xr_result, "xrEnumerateReferenceSpaces"))
		return false;
	vector<XrReferenceSpaceType> referenceSpaces(propertyCountOutput);
	xr_result = xrEnumerateReferenceSpaces(xr_session, propertyCountOutput, &propertyCountOutput, referenceSpaces.data());
	if(!xrCheckResult(xr_instance, xr_result, "xrEnumerateReferenceSpaces"))
		return false;

	// create reference space
	XrPosef pose;
    pose.orientation = { .x = 0, .y = 0, .z = 0, .w = 1.0 };
    pose.position = { .x = 0, .y = 0, .z = 0 };
	
	XrReferenceSpaceCreateInfo referenceSpaceCreateInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
	referenceSpaceCreateInfo.poseInReferenceSpace = pose;
	
	XrExtent2Df spaceBounds;
	
	std::cout << "Reference spaces (" << referenceSpaces.size() << ")" << std::endl;
	for (size_t i = 0; i < referenceSpaces.size(); i++){
		referenceSpaceCreateInfo.referenceSpaceType = referenceSpaces[i];
		std::cout << "  |-- type: " << referenceSpaces[i] << " (https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrReferenceSpaceType)" << std::endl;
		// view
		if(referenceSpaces[i] == XR_REFERENCE_SPACE_TYPE_VIEW){
			xr_result = xrCreateReferenceSpace(xr_session, &referenceSpaceCreateInfo, &xr_space_view);
			if(!xrCheckResult(xr_instance, xr_result, "xrCreateReferenceSpace (XR_REFERENCE_SPACE_TYPE_VIEW)"))
				return false;

			// get bounds
			xr_result = xrGetReferenceSpaceBoundsRect(xr_session, XR_REFERENCE_SPACE_TYPE_VIEW, &spaceBounds);
			if(!xrCheckResult(xr_instance, xr_result, "xrGetReferenceSpaceBoundsRect (XR_REFERENCE_SPACE_TYPE_VIEW)"))
				return false;
			
			std::cout << "  |     |-- reference space bounds" << std::endl;
			std::cout << "  |     |     |-- width: " << spaceBounds.width << std::endl;
			std::cout << "  |     |     |-- height: " << spaceBounds.height << std::endl;
		}
		// local
		else if(referenceSpaces[i] == XR_REFERENCE_SPACE_TYPE_LOCAL){
			xr_result = xrCreateReferenceSpace(xr_session, &referenceSpaceCreateInfo, &xr_space_local);
			if(!xrCheckResult(xr_instance, xr_result, "xrCreateReferenceSpace (XR_REFERENCE_SPACE_TYPE_LOCAL)"))
				return false;

			// get bounds
			xr_result = xrGetReferenceSpaceBoundsRect(xr_session, XR_REFERENCE_SPACE_TYPE_LOCAL, &spaceBounds);
			if(!xrCheckResult(xr_instance, xr_result, "xrGetReferenceSpaceBoundsRect (XR_REFERENCE_SPACE_TYPE_LOCAL)"))
				return false;
			
			std::cout << "  |     |-- reference space bounds" << std::endl;
			std::cout << "  |     |     |-- width: " << spaceBounds.width << std::endl;
			std::cout << "  |     |     |-- height: " << spaceBounds.height << std::endl;
		}
		// stage
		else if(referenceSpaces[i] == XR_REFERENCE_SPACE_TYPE_STAGE){
			xr_result = xrCreateReferenceSpace(xr_session, &referenceSpaceCreateInfo, &xr_space_stage);
			if(!xrCheckResult(xr_instance, xr_result, "xrCreateReferenceSpace (XR_REFERENCE_SPACE_TYPE_STAGE)"))
				return false;

			// get bounds
			xr_result = xrGetReferenceSpaceBoundsRect(xr_session, XR_REFERENCE_SPACE_TYPE_STAGE, &spaceBounds);
			if(!xrCheckResult(xr_instance, xr_result, "xrGetReferenceSpaceBoundsRect (XR_REFERENCE_SPACE_TYPE_STAGE)"))
				return false;
			
			std::cout << "  |     |-- reference space bounds" << std::endl;
			std::cout << "  |     |     |-- width: " << spaceBounds.width << std::endl;
			std::cout << "  |     |     |-- height: " << spaceBounds.height << std::endl;
		}
	}
	return true;
}

bool OpenXrApplication::defineSessionSpaces(){
	XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
	attachInfo.countActionSets = 1;
	attachInfo.actionSets = &xr_action_set;
	xr_result = xrAttachSessionActionSets(xr_session, &attachInfo);
	if(!xrCheckResult(xr_instance, xr_result, "xrAttachSessionActionSets"))
		return false;

	XrActionSpaceCreateInfo actionSpaceInfo = {XR_TYPE_ACTION_SPACE_CREATE_INFO};
	actionSpaceInfo.poseInActionSpace.orientation.w = 1.f;
	actionSpaceInfo.subactionPath = XR_NULL_PATH;

	for(size_t i = 0; i < xr_actions_pose.size(); i++){
		XrSpace space;
		actionSpaceInfo.action = xr_actions_pose[i];
		xr_result = xrCreateActionSpace(xr_session, &actionSpaceInfo, &space);
		if(!xrCheckResult(xr_instance, xr_result, "xrCreateActionSpace"))
			return false;
		xr_space_actions_pose.push_back(space);
	}
	return true;
}

bool OpenXrApplication::defineSwapchains(){
	// get swapchain Formats
	uint32_t propertyCountOutput;
	xr_result = xrEnumerateSwapchainFormats(xr_session, 0, &propertyCountOutput, nullptr);
	if(!xrCheckResult(xr_instance, xr_result, "xrEnumerateSwapchainFormats"))
		return false;
	vector<int64_t> swapchainFormats(propertyCountOutput);
	xr_result = xrEnumerateSwapchainFormats(xr_session, propertyCountOutput, &propertyCountOutput, swapchainFormats.data());
	if(!xrCheckResult(xr_instance, xr_result, "xrEnumerateSwapchainFormats"))
		return false;

	// select swapchain format
#ifdef XR_USE_GRAPHICS_API_VULKAN
	int64_t supportedSwapchainFormats[] = {VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM};
#endif
#ifdef XR_USE_GRAPHICS_API_OPENGL
	int64_t supportedSwapchainFormats[] = {GL_RGB10_A2, GL_RGBA16F, GL_RGBA8, GL_RGBA8_SNORM};
#endif

	int64_t selectedSwapchainFormats = -1;
	for(size_t i = 0; i < swapchainFormats.size(); i++){
		for (size_t j = 0; j < _countof(supportedSwapchainFormats); j++)
			if(swapchainFormats[i] == supportedSwapchainFormats[j]){
				selectedSwapchainFormats = swapchainFormats[i];
				break;
			}
		if(selectedSwapchainFormats != -1)
			break;
	}

	std::cout << "Swapchain formats (" << swapchainFormats.size() << ")" << std::endl;
	for (size_t i = 0; i < swapchainFormats.size(); i++){
		std::cout << "  |-- format: " << swapchainFormats[i] << std::endl;
		if (swapchainFormats[i] == selectedSwapchainFormats)
			std::cout << "  |   (selected)" << std::endl;
	}

	// create swapchain per view
	std::cout << "Created swapchain (" << xr_view_configuration_views.size() << ")" << std::endl;

	for(uint32_t i = 0; i < xr_view_configuration_views.size(); i++){
		XrSwapchainCreateInfo swapchainCreateInfo = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
		swapchainCreateInfo.arraySize = 1;
		swapchainCreateInfo.format = selectedSwapchainFormats;
		swapchainCreateInfo.width = xr_view_configuration_views[i].recommendedImageRectWidth;
		swapchainCreateInfo.height = xr_view_configuration_views[i].recommendedImageRectHeight;
		swapchainCreateInfo.mipCount = 1;
		swapchainCreateInfo.faceCount = 1;
		swapchainCreateInfo.sampleCount = xr_graphics_handler.getSupportedSwapchainSampleCount(xr_view_configuration_views[i]);
		swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
		
		SwapchainHandler swapchain;
		xr_result = xrCreateSwapchain(xr_session, &swapchainCreateInfo, &swapchain.handle);
		if(!xrCheckResult(xr_instance, xr_result, "xrCreateSwapchain"))
			return false;

		swapchain.width = swapchainCreateInfo.width;
		swapchain.height = swapchainCreateInfo.height;

		std::cout << "  |-- swapchain: " << i << std::endl;
		std::cout << "  |     |-- width: " << swapchainCreateInfo.width << std::endl;
		std::cout << "  |     |-- height: " << swapchainCreateInfo.height << std::endl;
		std::cout << "  |     |-- sample count: " << swapchainCreateInfo.sampleCount << std::endl;
	
		// enumerate swapchain images
		xr_result = xrEnumerateSwapchainImages(swapchain.handle, 0, &propertyCountOutput, nullptr);
		if(!xrCheckResult(xr_instance, xr_result, "xrEnumerateSwapchainImages"))
			return false;

		swapchain.length = propertyCountOutput;
#ifdef XR_USE_GRAPHICS_API_VULKAN
		swapchain.images.resize(propertyCountOutput, {XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR});
#endif
#ifdef XR_USE_GRAPHICS_API_OPENGL
		swapchain.images.resize(propertyCountOutput, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR});
#endif
		xrEnumerateSwapchainImages(swapchain.handle, propertyCountOutput, &propertyCountOutput, (XrSwapchainImageBaseHeader*)swapchain.images.data());
		std::cout << "  |     |-- swapchain images: " << propertyCountOutput << std::endl;

		xr_swapchains_handlers.push_back(swapchain);
	}

#ifdef XR_USE_GRAPHICS_API_OPENGL
	// acquire GL context
	glXMakeCurrent(xr_graphics_binding.xDisplay, xr_graphics_binding.glxDrawable, xr_graphics_binding.glxContext);
#endif
	return true;
}


bool OpenXrApplication::createInstance(string applicationName, string engineName, vector<string> requestedApiLayers, vector<string> requestedExtensions){
	vector<string> enabledApiLayers;
	vector<string> enabledExtensions;

	// layers
	if(!defineLayers(requestedApiLayers, enabledApiLayers))
		return false;

	// extensions
	if(!defineExtensions(requestedExtensions, enabledExtensions))
		return false;

	vector<const char*> enabledApiLayerNames = cast_to_vector_char_p(enabledApiLayers);
	vector<const char*> enabledExtensionNames = cast_to_vector_char_p(enabledExtensions);

	// initialize OpenXR (create instance) with the enabled extensions and layers
	XrInstanceCreateInfo createInfo = {XR_TYPE_INSTANCE_CREATE_INFO};
	createInfo.next = NULL;
	createInfo.createFlags = 0;
	createInfo.enabledApiLayerCount = enabledApiLayers.size();
	createInfo.enabledApiLayerNames = enabledApiLayerNames.data();
	createInfo.enabledExtensionCount = enabledExtensions.size();
	createInfo.enabledExtensionNames = enabledExtensionNames.data();
	createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
	createInfo.applicationInfo.applicationVersion = 1;
	createInfo.applicationInfo.engineVersion = 1;

	strncpy(createInfo.applicationInfo.applicationName, applicationName.c_str(), XR_MAX_APPLICATION_NAME_SIZE);
	strncpy(createInfo.applicationInfo.engineName, engineName.c_str(), XR_MAX_ENGINE_NAME_SIZE);

	xr_result = xrCreateInstance(&createInfo, &xr_instance);
	if(!xrCheckResult(NULL, xr_result, "xrCreateInstance"))
		return false;
	return true;
}

bool OpenXrApplication::getSystem(XrFormFactor formFactor, XrEnvironmentBlendMode blendMode, XrViewConfigurationType configurationType){
	XrSystemGetInfo systemInfo = {XR_TYPE_SYSTEM_GET_INFO};
	systemInfo.formFactor = formFactor;

	xr_result = xrGetSystem(xr_instance, &systemInfo, &xr_system_id);
	if(!xrCheckResult(xr_instance, xr_result, "xrGetSystem"))
		return false;
	
	if(!acquireInstanceProperties())
		return false;
	if(!acquireSystemProperties())
		return false;
	if(!acquireViewConfiguration(configurationType))
		return false;
 	if(!acquireBlendModes(blendMode))
	 	return false;

	// create action set
	XrActionSetCreateInfo actionSetInfo = {XR_TYPE_ACTION_SET_CREATE_INFO};
	strcpy(actionSetInfo.actionSetName, "actionset");
	strcpy(actionSetInfo.localizedActionSetName, "localized_actionset");
	actionSetInfo.priority = 0;

	xr_result = xrCreateActionSet(xr_instance, &actionSetInfo, &xr_action_set);
	if(!xrCheckResult(xr_instance, xr_result, "xrCreateActionSet"))
		return false;
	return true;
}

bool OpenXrApplication::addAction(string stringPath, XrActionType actionType){
	XrPath path;
	XrAction action;
	xrStringToPath(xr_instance, stringPath.c_str(), &path);

	string actionName = "";
	string localizedActionName = "";

	if(actionType == XR_ACTION_TYPE_BOOLEAN_INPUT)
		actionName = "action_boolean_" + std::to_string(xr_actions_boolean.size());
	else if(actionType == XR_ACTION_TYPE_FLOAT_INPUT)
		actionName = "action_float_" + std::to_string(xr_actions_float.size());
	else if(actionType == XR_ACTION_TYPE_VECTOR2F_INPUT)
		actionName = "action_vector2f_" + std::to_string(xr_actions_vector2f.size());
	else if(actionType == XR_ACTION_TYPE_POSE_INPUT)
		actionName = "action_pose_" + std::to_string(xr_actions_pose.size());
	else if(actionType == XR_ACTION_TYPE_VIBRATION_OUTPUT)
		actionName = "action_vibration_" + std::to_string(xr_actions_vibration.size());
	localizedActionName = "localized_" + actionName;

	XrActionCreateInfo actionInfo = {XR_TYPE_ACTION_CREATE_INFO};
	actionInfo.actionType = actionType;
	strcpy(actionInfo.actionName, actionName.c_str());
	strcpy(actionInfo.localizedActionName, localizedActionName.c_str());
	actionInfo.countSubactionPaths = 0;
	actionInfo.subactionPaths = nullptr;
	
	xr_result = xrCreateAction(xr_action_set, &actionInfo, &action);
	if(!xrCheckResult(xr_instance, xr_result, "xrCreateAction"))
		return false;
	
	if(actionType == XR_ACTION_TYPE_BOOLEAN_INPUT){
		xr_actions_boolean.push_back(action); 
		xr_action_paths_boolean.push_back(path); 
		xr_action_string_paths_boolean.push_back(stringPath); 
	}
	else if(actionType == XR_ACTION_TYPE_FLOAT_INPUT){
		xr_actions_float.push_back(action); 
		xr_action_paths_float.push_back(path); 
		xr_action_string_paths_float.push_back(stringPath); 
	}
	else if(actionType == XR_ACTION_TYPE_VECTOR2F_INPUT){
		xr_actions_vector2f.push_back(action); 
		xr_action_paths_vector2f.push_back(path); 
		xr_action_string_paths_vector2f.push_back(stringPath); 
	}
	else if(actionType == XR_ACTION_TYPE_POSE_INPUT){
		xr_actions_pose.push_back(action); 
		xr_action_paths_pose.push_back(path); 
		xr_action_string_paths_pose.push_back(stringPath); 
	}
	else if(actionType == XR_ACTION_TYPE_VIBRATION_OUTPUT){
		xr_actions_vibration.push_back(action); 
		xr_action_paths_vibration.push_back(path); 
		xr_action_string_paths_vibration.push_back(stringPath); 
	}
	return true;
}

void OpenXrApplication::defineInteractionProfileBindings(vector<XrActionSuggestedBinding> & bindings, vector<string> validPaths){
	for(size_t i = 0; i < xr_actions_boolean.size(); i++)
		if(std::find(validPaths.begin(), validPaths.end(), xr_action_string_paths_boolean[i]) != validPaths.end()){
			XrActionSuggestedBinding binding;
			binding.action = xr_actions_boolean[i];
			binding.binding = xr_action_paths_boolean[i];
			bindings.push_back(binding);
		}
	for(size_t i = 0; i < xr_actions_float.size(); i++)
		if(std::find(validPaths.begin(), validPaths.end(), xr_action_string_paths_float[i]) != validPaths.end()){
			XrActionSuggestedBinding binding;
			binding.action = xr_actions_float[i];
			binding.binding = xr_action_paths_float[i];
			bindings.push_back(binding);
		}
	for(size_t i = 0; i < xr_actions_vector2f.size(); i++)
		if(std::find(validPaths.begin(), validPaths.end(), xr_action_string_paths_vector2f[i]) != validPaths.end()){
			XrActionSuggestedBinding binding;
			binding.action = xr_actions_vector2f[i];
			binding.binding = xr_action_paths_vector2f[i];
			bindings.push_back(binding);
		}
	for(size_t i = 0; i < xr_actions_pose.size(); i++)
		if(std::find(validPaths.begin(), validPaths.end(), xr_action_string_paths_pose[i]) != validPaths.end()){
			XrActionSuggestedBinding binding;
			binding.action = xr_actions_pose[i];
			binding.binding = xr_action_paths_pose[i];
			bindings.push_back(binding);
		}
	for(size_t i = 0; i < xr_actions_vibration.size(); i++)
		if(std::find(validPaths.begin(), validPaths.end(), xr_action_string_paths_vibration[i]) != validPaths.end()){
			XrActionSuggestedBinding binding;
			binding.action = xr_actions_vibration[i];
			binding.binding = xr_action_paths_vibration[i];
			bindings.push_back(binding);
		}
}

bool OpenXrApplication::suggestInteractionProfileBindings(){
	vector<string> validPathsKhronosSimpleController = {{"/user/hand/left/input/select/click"},
														{"/user/hand/left/input/menu/click"},
														{"/user/hand/left/input/grip/pose"},
														{"/user/hand/left/input/aim/pose"},
														{"/user/hand/left/output/haptic"},
														{"/user/hand/right/input/select/click"},
														{"/user/hand/right/input/menu/click"},
														{"/user/hand/right/input/grip/pose"},
														{"/user/hand/right/input/aim/pose"},
														{"/user/hand/right/output/haptic"}};

	vector<string> validPathsGoogleDaydreamController = {{"/user/hand/left/input/select/click"},
													     {"/user/hand/left/input/trackpad/x"},
													     {"/user/hand/left/input/trackpad/y"},
													     {"/user/hand/left/input/trackpad/click"},
													     {"/user/hand/left/input/trackpad/touch"},
													     {"/user/hand/left/input/grip/pose"},
													     {"/user/hand/left/input/aim/pose"},
														 {"/user/hand/right/input/select/click"},
														 {"/user/hand/right/input/trackpad/x"},
														 {"/user/hand/right/input/trackpad/y"},
														 {"/user/hand/right/input/trackpad/click"},
														 {"/user/hand/right/input/trackpad/touch"},
														 {"/user/hand/right/input/grip/pose"},
														 {"/user/hand/right/input/aim/pose"}};

	vector<string> validPathsHTCViveController = {{"/user/hand/left/input/system/click"},
												  {"/user/hand/left/input/squeeze/click"},
												  {"/user/hand/left/input/menu/click"},
												  {"/user/hand/left/input/trigger/click"},
												  {"/user/hand/left/input/trigger/value"},
												  {"/user/hand/left/input/trackpad/x"},
												  {"/user/hand/left/input/trackpad/y"},
												  {"/user/hand/left/input/trackpad/click"},
												  {"/user/hand/left/input/trackpad/touch"},
												  {"/user/hand/left/input/grip/pose"},
												  {"/user/hand/left/input/aim/pose"},
												  {"/user/hand/left/output/haptic"},
												  {"/user/hand/right/input/system/click"},
												  {"/user/hand/right/input/squeeze/click"},
												  {"/user/hand/right/input/menu/click"},
												  {"/user/hand/right/input/trigger/click"},
												  {"/user/hand/right/input/trigger/value"},
												  {"/user/hand/right/input/trackpad/x"},
												  {"/user/hand/right/input/trackpad/y"},
												  {"/user/hand/right/input/trackpad/click"},
												  {"/user/hand/right/input/trackpad/touch"},
												  {"/user/hand/right/input/grip/pose"},
												  {"/user/hand/right/input/aim/pose"},
												  {"/user/hand/right/output/haptic"}};
  
	vector<string> validPathsHTCVivePro = {{"/user/head/input/system/click"},
										   {"/user/head/input/volume_up/click"},
										   {"/user/head/input/volume_down/click"},
										   {"/user/head/input/mute_mic/click"}};

	vector<string> validPathsMicrosoftMixedRealityMotionController = {{"/user/hand/left/input/menu/click"},
																	  {"/user/hand/left/input/squeeze/click"},
																	  {"/user/hand/left/input/trigger/value"},
																	  {"/user/hand/left/input/thumbstick/x"},
																	  {"/user/hand/left/input/thumbstick/y"},
																	  {"/user/hand/left/input/thumbstick/click"},
																	  {"/user/hand/left/input/trackpad/x"},
																	  {"/user/hand/left/input/trackpad/y"},
																	  {"/user/hand/left/input/trackpad/click"},
																	  {"/user/hand/left/input/trackpad/touch"},
																	  {"/user/hand/left/input/grip/pose"},
																	  {"/user/hand/left/input/aim/pose"},
																	  {"/user/hand/left/output/haptic"},
																	  {"/user/hand/right/input/menu/click"},
																	  {"/user/hand/right/input/squeeze/click"},
																	  {"/user/hand/right/input/trigger/value"},
																	  {"/user/hand/right/input/thumbstick/x"},
																	  {"/user/hand/right/input/thumbstick/y"},
																	  {"/user/hand/right/input/thumbstick/click"},
																	  {"/user/hand/right/input/trackpad/x"},
																	  {"/user/hand/right/input/trackpad/y"},
																	  {"/user/hand/right/input/trackpad/click"},
																	  {"/user/hand/right/input/trackpad/touch"},
																	  {"/user/hand/right/input/grip/pose"},
																	  {"/user/hand/right/input/aim/pose"},
																	  {"/user/hand/right/output/haptic"}};
  
	vector<string> validPathsMicrosoftXboxController = {{"/user/gamepad/input/menu/click"},
														{"/user/gamepad/input/view/click"},
														{"/user/gamepad/input/a/click"},
														{"/user/gamepad/input/b/click"},
														{"/user/gamepad/input/x/click"},
														{"/user/gamepad/input/y/click"},
														{"/user/gamepad/input/dpad_down/click"},
														{"/user/gamepad/input/dpad_right/click"},
														{"/user/gamepad/input/dpad_up/click"},
														{"/user/gamepad/input/dpad_left/click"},
														{"/user/gamepad/input/shoulder_left/click"},
														{"/user/gamepad/input/shoulder_right/click"},
														{"/user/gamepad/input/thumbstick_left/click"},
														{"/user/gamepad/input/thumbstick_right/click"},
														{"/user/gamepad/input/trigger_left/value"},
														{"/user/gamepad/input/trigger_right/value"},
														{"/user/gamepad/input/thumbstick_left/x"},
														{"/user/gamepad/input/thumbstick_left/y"},
														{"/user/gamepad/input/thumbstick_right/x"},
														{"/user/gamepad/input/thumbstick_right/y"},
														{"/user/gamepad/output/haptic_left"},
														{"/user/gamepad/output/haptic_right"},
														{"/user/gamepad/output/haptic_left_trigger"},
														{"/user/gamepad/output/haptic_right_trigger"}};

	vector<string> validPathsOculusGoController = {{"/user/hand/left/input/system/click"},
													{"/user/hand/left/input/trigger/click"},
													{"/user/hand/left/input/back/click"},
													{"/user/hand/left/input/trackpad/x"},
													{"/user/hand/left/input/trackpad/y"},
													{"/user/hand/left/input/trackpad/click"},
													{"/user/hand/left/input/trackpad/touch"},
													{"/user/hand/left/input/grip/pose"},
													{"/user/hand/left/input/aim/pose"},
													{"/user/hand/right/input/system/click"},
													{"/user/hand/right/input/trigger/click"},
													{"/user/hand/right/input/back/click"},
													{"/user/hand/right/input/trackpad/x"},
													{"/user/hand/right/input/trackpad/y"},
													{"/user/hand/right/input/trackpad/click"},
													{"/user/hand/right/input/trackpad/touch"},
													{"/user/hand/right/input/grip/pose"},
													{"/user/hand/right/input/aim/pose"}};

	vector<string> validPathsOculusTouchController = {{"/user/hand/left/input/squeeze/value"},
														{"/user/hand/left/input/trigger/value"},
														{"/user/hand/left/input/trigger/touch"},
														{"/user/hand/left/input/thumbstick/x"},
														{"/user/hand/left/input/thumbstick/y"},
														{"/user/hand/left/input/thumbstick/click"},
														{"/user/hand/left/input/thumbstick/touch"},
														{"/user/hand/left/input/thumbrest/touch"},
														{"/user/hand/left/input/grip/pose"},
														{"/user/hand/left/input/aim/pose"},
														{"/user/hand/left/output/haptic"},
														{"/user/hand/left/input/x/click"},
														{"/user/hand/left/input/x/touch"},
														{"/user/hand/left/input/y/click"},
														{"/user/hand/left/input/y/touch"},
														{"/user/hand/left/input/menu/click"},
														{"/user/hand/right/input/squeeze/value"},
														{"/user/hand/right/input/trigger/value"},
														{"/user/hand/right/input/trigger/touch"},
														{"/user/hand/right/input/thumbstick/x"},
														{"/user/hand/right/input/thumbstick/y"},
														{"/user/hand/right/input/thumbstick/click"},
														{"/user/hand/right/input/thumbstick/touch"},
														{"/user/hand/right/input/thumbrest/touch"},
														{"/user/hand/right/input/grip/pose"},
														{"/user/hand/right/input/aim/pose"},
														{"/user/hand/right/output/haptic"},
														{"/user/hand/right/input/a/click"},
														{"/user/hand/right/input/a/touch"},
														{"/user/hand/right/input/b/click"},
														{"/user/hand/right/input/b/touch"},
														{"/user/hand/right/input/system/click"}};

	vector<string> validPathsValveIndexController = {{"/user/hand/left/input/system/click"},
													{"/user/hand/left/input/system/touch"},
													{"/user/hand/left/input/a/click"},
													{"/user/hand/left/input/a/touch"},
													{"/user/hand/left/input/b/click"},
													{"/user/hand/left/input/b/touch"},
													{"/user/hand/left/input/squeeze/value"},
													{"/user/hand/left/input/squeeze/force"},
													{"/user/hand/left/input/trigger/click"},
													{"/user/hand/left/input/trigger/value"},
													{"/user/hand/left/input/trigger/touch"},
													{"/user/hand/left/input/thumbstick/x"},
													{"/user/hand/left/input/thumbstick/y"},
													{"/user/hand/left/input/thumbstick/click"},
													{"/user/hand/left/input/thumbstick/touch"},
													{"/user/hand/left/input/trackpad/x"},
													{"/user/hand/left/input/trackpad/y"},
													{"/user/hand/left/input/trackpad/force"},
													{"/user/hand/left/input/trackpad/touch"},
													{"/user/hand/left/input/grip/pose"},
													{"/user/hand/left/input/aim/pose"},
													{"/user/hand/left/output/haptic"},
													{"/user/hand/right/input/system/click"},
													{"/user/hand/right/input/system/touch"},
													{"/user/hand/right/input/a/click"},
													{"/user/hand/right/input/a/touch"},
													{"/user/hand/right/input/b/click"},
													{"/user/hand/right/input/b/touch"},
													{"/user/hand/right/input/squeeze/value"},
													{"/user/hand/right/input/squeeze/force"},
													{"/user/hand/right/input/trigger/click"},
													{"/user/hand/right/input/trigger/value"},
													{"/user/hand/right/input/trigger/touch"},
													{"/user/hand/right/input/thumbstick/x"},
													{"/user/hand/right/input/thumbstick/y"},
													{"/user/hand/right/input/thumbstick/click"},
													{"/user/hand/right/input/thumbstick/touch"},
													{"/user/hand/right/input/trackpad/x"},
													{"/user/hand/right/input/trackpad/y"},
													{"/user/hand/right/input/trackpad/force"},
													{"/user/hand/right/input/trackpad/touch"},
													{"/user/hand/right/input/grip/pose"},
													{"/user/hand/right/input/aim/pose"},
													{"/user/hand/right/output/haptic"}};

	XrPath interactionProfilePath;
	vector<XrActionSuggestedBinding> bindings;
	XrInteractionProfileSuggestedBinding suggestedBindings = {XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
	std::cout << "Suggested interaction bindings by profiles" << std::endl;

	// Khronos Simple Controller
	bindings.clear();
	xrStringToPath(xr_instance, "/interaction_profiles/khr/simple_controller", &interactionProfilePath);
	defineInteractionProfileBindings(bindings, validPathsKhronosSimpleController);
	if(bindings.size()){
		suggestedBindings.interactionProfile = interactionProfilePath;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		xr_result = xrSuggestInteractionProfileBindings(xr_instance, &suggestedBindings);
		if(!xrCheckResult(xr_instance, xr_result, "xrSuggestInteractionProfileBindings /interaction_profiles/khr/simple_controller"))
			return false;
	}
	std::cout << "  |-- /interaction_profiles/khr/simple_controller (" << bindings.size() << ")" << std::endl;
	
	// Google Daydream Controller
	bindings.clear();
	xrStringToPath(xr_instance, "/interaction_profiles/google/daydream_controller", &interactionProfilePath);
	defineInteractionProfileBindings(bindings, validPathsGoogleDaydreamController);
	if(bindings.size()){
		suggestedBindings.interactionProfile = interactionProfilePath;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		xr_result = xrSuggestInteractionProfileBindings(xr_instance, &suggestedBindings);
		if(!xrCheckResult(xr_instance, xr_result, "xrSuggestInteractionProfileBindings /interaction_profiles/google/daydream_controller"))
			return false;
	}
	std::cout << "  |-- /interaction_profiles/google/daydream_controller (" << bindings.size() << ")" << std::endl;

	// HTC Vive Controller
	bindings.clear();
	xrStringToPath(xr_instance, "/interaction_profiles/htc/vive_controller", &interactionProfilePath);
	defineInteractionProfileBindings(bindings, validPathsHTCViveController);
	if(bindings.size()){
		suggestedBindings.interactionProfile = interactionProfilePath;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		xr_result = xrSuggestInteractionProfileBindings(xr_instance, &suggestedBindings);
		if(!xrCheckResult(xr_instance, xr_result, "xrSuggestInteractionProfileBindings /interaction_profiles/htc/vive_controller"))
			return false;
	}
	std::cout << "  |-- /interaction_profiles/htc/vive_controller (" << bindings.size() << ")" << std::endl;

	// HTC Vive Pro
	bindings.clear();
	xrStringToPath(xr_instance, "/interaction_profiles/htc/vive_pro", &interactionProfilePath);
	defineInteractionProfileBindings(bindings, validPathsHTCVivePro);
	if(bindings.size()){
		suggestedBindings.interactionProfile = interactionProfilePath;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		xr_result = xrSuggestInteractionProfileBindings(xr_instance, &suggestedBindings);
		if(!xrCheckResult(xr_instance, xr_result, "xrSuggestInteractionProfileBindings /interaction_profiles/htc/vive_pro"))
			return false;
	}
	std::cout << "  |-- /interaction_profiles/htc/vive_pro (" << bindings.size() << ")" << std::endl;

	// Microsoft Mixed Reality Motion Controller
	bindings.clear();
	xrStringToPath(xr_instance, "/interaction_profiles/microsoft/motion_controller", &interactionProfilePath);
	defineInteractionProfileBindings(bindings, validPathsMicrosoftMixedRealityMotionController);
	if(bindings.size()){
		suggestedBindings.interactionProfile = interactionProfilePath;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		xr_result = xrSuggestInteractionProfileBindings(xr_instance, &suggestedBindings);
		if(!xrCheckResult(xr_instance, xr_result, "xrSuggestInteractionProfileBindings /interaction_profiles/microsoft/motion_controller"))
			return false;
	}
	std::cout << "  |-- /interaction_profiles/microsoft/motion_controller (" << bindings.size() << ")" << std::endl;

	// Microsoft Xbox Controller
	bindings.clear();
	xrStringToPath(xr_instance, "/interaction_profiles/microsoft/xbox_controller", &interactionProfilePath);
	defineInteractionProfileBindings(bindings, validPathsMicrosoftXboxController);
	if(bindings.size()){
		suggestedBindings.interactionProfile = interactionProfilePath;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		xr_result = xrSuggestInteractionProfileBindings(xr_instance, &suggestedBindings);
		if(!xrCheckResult(xr_instance, xr_result, "xrSuggestInteractionProfileBindings /interaction_profiles/microsoft/xbox_controller"))
			return false;
	}
	std::cout << "  |-- /interaction_profiles/microsoft/xbox_controller (" << bindings.size() << ")" << std::endl;

	// Oculus Go Controller
	bindings.clear();
	xrStringToPath(xr_instance, "/interaction_profiles/oculus/go_controller", &interactionProfilePath);
	defineInteractionProfileBindings(bindings, validPathsOculusGoController);
	if(bindings.size()){
		suggestedBindings.interactionProfile = interactionProfilePath;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		xr_result = xrSuggestInteractionProfileBindings(xr_instance, &suggestedBindings);
		if(!xrCheckResult(xr_instance, xr_result, "xrSuggestInteractionProfileBindings /interaction_profiles/oculus/go_controller"))
			return false;
	}
	std::cout << "  |-- /interaction_profiles/oculus/go_controller (" << bindings.size() << ")" << std::endl;

	// Oculus Touch Controller
	bindings.clear();
	xrStringToPath(xr_instance, "/interaction_profiles/oculus/touch_controller", &interactionProfilePath);
	defineInteractionProfileBindings(bindings, validPathsOculusTouchController);
	if(bindings.size()){
		suggestedBindings.interactionProfile = interactionProfilePath;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		xr_result = xrSuggestInteractionProfileBindings(xr_instance, &suggestedBindings);
		if(!xrCheckResult(xr_instance, xr_result, "xrSuggestInteractionProfileBindings /interaction_profiles/oculus/touch_controller"))
			return false;
	}
	std::cout << "  |-- /interaction_profiles/oculus/touch_controller (" << bindings.size() << ")" << std::endl;

	// Valve Index Controller
	bindings.clear();
	xrStringToPath(xr_instance, "/interaction_profiles/valve/index_controller", &interactionProfilePath);
	defineInteractionProfileBindings(bindings, validPathsValveIndexController);
	if(bindings.size()){
		suggestedBindings.interactionProfile = interactionProfilePath;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		xr_result = xrSuggestInteractionProfileBindings(xr_instance, &suggestedBindings);
		if(!xrCheckResult(xr_instance, xr_result, "xrSuggestInteractionProfileBindings /interaction_profiles/valve/index_controller"))
			return false;
	}
	std::cout << "  |-- /interaction_profiles/valve/index_controller (" << bindings.size() << ")" << std::endl;
	
	return true;
}

bool OpenXrApplication::createSession(){
#ifdef XR_USE_GRAPHICS_API_VULKAN
	xr_graphics_handler.createInstance(xr_instance, xr_system_id);
	xr_graphics_handler.getPhysicalDevice(xr_instance, xr_system_id);
	xr_graphics_handler.createLogicalDevice(xr_instance, xr_system_id);

    xr_graphics_binding.instance = xr_graphics_handler.getInstance();
    xr_graphics_binding.physicalDevice = xr_graphics_handler.getPhysicalDevice();
    xr_graphics_binding.device = xr_graphics_handler.getLogicalDevice();
    xr_graphics_binding.queueFamilyIndex = xr_graphics_handler.getQueueFamilyIndex();
    xr_graphics_binding.queueIndex = 0;

	std::cout << "Graphics binding: Vulkan" << std::endl;
	std::cout << "  |-- instance: " << xr_graphics_binding.instance << std::endl;
	std::cout << "  |-- physical device: " << xr_graphics_binding.physicalDevice << std::endl;
	std::cout << "  |-- device: " << xr_graphics_binding.device << std::endl;
	std::cout << "  |-- queue family index: " << xr_graphics_binding.queueFamilyIndex << std::endl;
	std::cout << "  |-- queue index: " << xr_graphics_binding.queueIndex << std::endl;
#endif
#ifdef XR_USE_GRAPHICS_API_OPENGL
	if(!xr_graphics_handler.getRequirements(xr_instance, xr_system_id))
		return false;
	if(!xr_graphics_handler.initGraphicsBinding(&xr_graphics_binding.xDisplay, 
												&xr_graphics_binding.visualid,
												&xr_graphics_binding.glxFBConfig, 
												&xr_graphics_binding.glxDrawable,
												&xr_graphics_binding.glxContext,
												xr_view_configuration_views[0].recommendedImageRectWidth,
												xr_view_configuration_views[0].recommendedImageRectHeight))
		return false;
	if(!xr_graphics_handler.initResources(xr_instance, xr_system_id))
		return false;

	std::cout << "Graphics binding: OpenGL" << std::endl;
	std::cout << "  |-- xDisplay: " << xr_graphics_binding.xDisplay << std::endl;
	std::cout << "  |-- visualid: " << xr_graphics_binding.visualid << std::endl;
	std::cout << "  |-- glxFBConfig: " << xr_graphics_binding.glxFBConfig << std::endl;
	std::cout << "  |-- glxDrawable: " << xr_graphics_binding.glxDrawable << std::endl;
	std::cout << "  |-- glxContext: " << xr_graphics_binding.glxContext << std::endl;
#endif

	// create session
	XrSessionCreateInfo sessionInfo = {XR_TYPE_SESSION_CREATE_INFO};
	sessionInfo.next = &xr_graphics_binding;
	sessionInfo.systemId = xr_system_id;

	xr_result = xrCreateSession(xr_instance, &sessionInfo, &xr_session);
	if(!xrCheckResult(xr_instance, xr_result, "xrCreateSession"))
		return false;

	// reference spaces
	if(!defineReferenceSpaces())
		return false;

	// suggest interaction profile bindings
	if(!suggestInteractionProfileBindings())
		return false;

	// action spaces / attach session action sets
	if(!defineSessionSpaces())
		return false;

	// swapchains
	if(!defineSwapchains())
		return false;
	return true;
}

bool OpenXrApplication::pollEvents(bool * exitLoop){
	*exitLoop = false;
	XrEventDataBuffer event;

	while(true){
		// pool events
		event.type = XR_TYPE_EVENT_DATA_BUFFER;
		event.next = nullptr;

		xr_result = xrPollEvent(xr_instance, &event);
		if(!xrCheckResult(xr_instance, xr_result, "xrPollEvent"))
			return false;

		// process messages
		switch(event.type){
			case XR_EVENT_UNAVAILABLE: {
				return true;
				break;
			}
			case XR_TYPE_EVENT_DATA_BUFFER: {
				return true;
				break;
			}
			// event queue overflowed (some events were removed)
			case XR_TYPE_EVENT_DATA_EVENTS_LOST: {
                const XrEventDataEventsLost & eventsLost = *reinterpret_cast<XrEventDataEventsLost*>(&event);
				std::cout << "Event queue has overflowed (" << eventsLost.lostEventCount << " overflowed event(s))" << std::endl;
				break;
            }
			// session state changed
			case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
				const XrEventDataSessionStateChanged & sessionStateChangedEvent = *reinterpret_cast<XrEventDataSessionStateChanged*>(&event);

				std::cout << "XrEventDataSessionStateChanged to " << _enum_to_string(sessionStateChangedEvent.state) << " (" << sessionStateChangedEvent.state << ")" << std::endl;
				
				// check session
				if((sessionStateChangedEvent.session != XR_NULL_HANDLE) && (sessionStateChangedEvent.session != xr_session)){
					std::cout << "XrEventDataSessionStateChanged for unknown session " << sessionStateChangedEvent.session << std::endl;
					return false;
				}
				
				// handle session state
				switch(sessionStateChangedEvent.state){
					case XR_SESSION_STATE_READY: {
						XrSessionBeginInfo sessionBeginInfo = {XR_TYPE_SESSION_BEGIN_INFO};
						sessionBeginInfo.primaryViewConfigurationType = configViewConfigurationType;
						xr_result = xrBeginSession(xr_session, &sessionBeginInfo);
						if(!xrCheckResult(xr_instance, xr_result, "xrBeginSession"))
							return false;

						flagSessionRunning = true;
						std::cout << "Event: XR_SESSION_STATE_READY (xrBeginSession)" << std::endl;
						break;
					}
					case XR_SESSION_STATE_STOPPING: {
						xr_result = xrEndSession(xr_session);
						if(!xrCheckResult(xr_instance, xr_result, "xrEndSession"))
							return false;

						flagSessionRunning = false;
						std::cout << "Event: XR_SESSION_STATE_STOPPING (xrEndSession)" << std::endl;
						break;
					}
					case XR_SESSION_STATE_EXITING: {
						*exitLoop = true;
						std::cout << "Event: XR_SESSION_STATE_EXITING" << std::endl;
						break;
					}
					case XR_SESSION_STATE_LOSS_PENDING: {
						*exitLoop = true;
						std::cout << "Event: XR_SESSION_STATE_LOSS_PENDING" << std::endl;
						break;
					}
					default:
						break;
				}
				break;
			}
			// application is about to lose the XrInstance 
			case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
				const XrEventDataInstanceLossPending & instanceLossPending = *reinterpret_cast<XrEventDataInstanceLossPending*>(&event);				
				*exitLoop = true;
				std::cout << "XrEventDataInstanceLossPending by " << instanceLossPending.lossTime << std::endl;
				return true;
				break;
			}
			case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: {
				// TODO: implement
				std::cout << "TODO: XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED" << std::endl;
				break;
			}
			// reference space is changing
			case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING: {
				const XrEventDataReferenceSpaceChangePending & referenceSpaceChangePending = *reinterpret_cast<XrEventDataReferenceSpaceChangePending*>(&event);
				std::cout << "XrEventDataReferenceSpaceChangePending for " << _enum_to_string(referenceSpaceChangePending.referenceSpaceType) << std::endl;
				break;
			}
			default:
				break;
		}
	}
	return true;
}

bool OpenXrApplication::pollActions(vector<ActionState> & actionStates){
	// sync actions
	XrActiveActionSet activeActionSet = {xr_action_set, XR_NULL_PATH};
	XrActionsSyncInfo syncInfo = {XR_TYPE_ACTIONS_SYNC_INFO};
	syncInfo.countActiveActionSets = 1;
	syncInfo.activeActionSets = &activeActionSet;
	xr_result = xrSyncActions(xr_session, &syncInfo);
	if(!xrCheckResult(xr_instance, xr_result, "xrSyncActions"))
		return false;

	XrActionStateGetInfo getInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
	getInfo.next = nullptr;
	getInfo.subactionPath = XR_NULL_PATH;

	// boolean
	XrActionStateBoolean actionStateBoolean = {XR_TYPE_ACTION_STATE_BOOLEAN};
	for(size_t i = 0; i < xr_actions_boolean.size(); i++){
		getInfo.action = xr_actions_boolean[i];
		xr_result = xrGetActionStateBoolean(xr_session, &getInfo, &actionStateBoolean);
		if(!xrCheckResult(xr_instance, xr_result, "xrGetActionStateBoolean"))
			return false;
		if(actionStateBoolean.isActive && actionStateBoolean.changedSinceLastSync){
			ActionState state;
			state.type = XR_ACTION_TYPE_BOOLEAN_INPUT;
			state.path = xr_action_string_paths_boolean[i].c_str();
			state.isActive = actionStateBoolean.isActive;
			state.stateBool = (bool)actionStateBoolean.currentState;
			actionStates.push_back(state);
		}
	}

	// float
	XrActionStateFloat actionStateFloat = {XR_TYPE_ACTION_STATE_FLOAT};
	for(size_t i = 0; i < xr_actions_float.size(); i++){
		getInfo.action = xr_actions_float[i];
		xr_result = xrGetActionStateFloat(xr_session, &getInfo, &actionStateFloat);
		if(!xrCheckResult(xr_instance, xr_result, "xrGetActionStateFloat"))
			return false;
		if(actionStateFloat.isActive && actionStateFloat.changedSinceLastSync){
			ActionState state;
			state.type = XR_ACTION_TYPE_FLOAT_INPUT;
			state.path = xr_action_string_paths_float[i].c_str();
			state.isActive = actionStateFloat.isActive;
			state.stateFloat = actionStateFloat.currentState;
			actionStates.push_back(state);
		}
	}

	// vector2f
	XrActionStateVector2f actionStateVector2f = {XR_TYPE_ACTION_STATE_VECTOR2F};
	for(size_t i = 0; i < xr_actions_vector2f.size(); i++){
		getInfo.action = xr_actions_vector2f[i];
		xr_result = xrGetActionStateVector2f(xr_session, &getInfo, &actionStateVector2f);
		if(!xrCheckResult(xr_instance, xr_result, "xrGetActionStateVector2f"))
			return false;
		if(actionStateVector2f.isActive && actionStateVector2f.changedSinceLastSync){
			ActionState state;
			state.type = XR_ACTION_TYPE_VECTOR2F_INPUT;
			state.path = xr_action_string_paths_vector2f[i].c_str();
			state.isActive = actionStateVector2f.isActive;
			state.stateVectorX = actionStateVector2f.currentState.x;
			state.stateVectorY = actionStateVector2f.currentState.y;
			actionStates.push_back(state);
		}
	}

	// pose
	XrActionStatePose actionStatePose = {XR_TYPE_ACTION_STATE_POSE};
	for(size_t i = 0; i < xr_actions_pose.size(); i++){
		getInfo.action = xr_actions_pose[i];
		xr_result = xrGetActionStatePose(xr_session, &getInfo, &actionStatePose);
		if(!xrCheckResult(xr_instance, xr_result, "xrGetActionStatePose"))
			return false;
		if(actionStatePose.isActive){
			ActionState state;
			state.type = XR_ACTION_TYPE_POSE_INPUT;
			state.path = xr_action_string_paths_pose[i].c_str();
			state.isActive = actionStatePose.isActive;
			actionStates.push_back(state);
			// std::cout << "ACTION: actionStatePose " << xr_action_string_paths_pose[i] << std::endl;
		}
	}

	return true;
}

bool OpenXrApplication::renderViews(XrReferenceSpaceType referenceSpaceType){
	xr_graphics_handler.acquireContext(xr_graphics_binding, "xrWaitFrame");

	XrFrameWaitInfo frameWaitInfo = {XR_TYPE_FRAME_WAIT_INFO};
	XrFrameState frameState = {XR_TYPE_FRAME_STATE};
	xr_result = xrWaitFrame(xr_session, &frameWaitInfo, &frameState);
	if(!xrCheckResult(xr_instance, xr_result, "xrWaitFrame"))
		return false;

	XrFrameBeginInfo frameBeginInfo = {XR_TYPE_FRAME_BEGIN_INFO};
	xr_result = xrBeginFrame(xr_session, &frameBeginInfo);
	if(!xrCheckResult(xr_instance, xr_result, "xrBeginFrame"))
		return false;

	// locate actions
	// std::cout << std::endl;
	XrSpaceLocation spaceLocation = {XR_TYPE_SPACE_LOCATION};
	for(size_t i = 0; i < xr_space_actions_pose.size(); i++){
		xr_result = xrLocateSpace(xr_space_actions_pose[i], xr_space_view, frameState.predictedDisplayTime, &spaceLocation);
		if(!xrCheckResult(xr_instance, xr_result, "xrLocateSpace"))
			return false;
		
		// if((spaceLocation.locationFlags & XR_VIEW_STATE_POSITION_VALID_BIT) != 0 || (spaceLocation.locationFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) != 0){
		// 	std::cout << spaceLocation.pose.position.x << '\t' << spaceLocation.pose.position.y << '\t' << spaceLocation.pose.position.z << std::endl;
		// }
		// else
		// 	std::cout << "Invalid location space" << std::endl;
	}

	vector<XrCompositionLayerBaseHeader*> layers;
	XrCompositionLayerProjection layer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
	vector<XrCompositionLayerProjectionView> projectionLayerViews;

	if(frameState.shouldRender == XR_TRUE){
		// XrSpaceLocationFlags flags = spaceLocation.locationFlags;
		// if((flags & XR_SPACE_LOCATION_POSITION_VALID_BIT) && (flags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))
		// 	std::cout << "pose: " << spaceLocation.pose.position.x << "\t" << spaceLocation.pose.position.y << "\t" << spaceLocation.pose.position.z << std::endl;
		
		// locate views
		vector<XrView> views(xr_view_configuration_views.size(), {XR_TYPE_VIEW});

		XrViewState viewState = {XR_TYPE_VIEW_STATE};
		uint32_t viewCountOutput;

		XrViewLocateInfo viewLocateInfo = {XR_TYPE_VIEW_LOCATE_INFO};
		viewLocateInfo.viewConfigurationType = configViewConfigurationType;
		viewLocateInfo.displayTime = frameState.predictedDisplayTime;

		if(referenceSpaceType == XR_REFERENCE_SPACE_TYPE_VIEW){
			viewLocateInfo.space = xr_space_view;
			xr_result = xrLocateViews(xr_session, &viewLocateInfo, &viewState, (uint32_t)views.size(), &viewCountOutput, views.data());
			if(!xrCheckResult(xr_instance, xr_result, "xrLocateViews (XR_REFERENCE_SPACE_TYPE_VIEW)"))
				return false;
			if((viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0 || (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0)
				std::cout << "Invalid location view for XR_REFERENCE_SPACE_TYPE_VIEW" << std::endl;
		}
		else if(referenceSpaceType == XR_REFERENCE_SPACE_TYPE_LOCAL){
			viewLocateInfo.space = xr_space_local;
			xr_result = xrLocateViews(xr_session, &viewLocateInfo, &viewState, (uint32_t)views.size(), &viewCountOutput, views.data());
			if(!xrCheckResult(xr_instance, xr_result, "xrLocateViews (XR_REFERENCE_SPACE_TYPE_LOCAL)"))
				return false;
			if((viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0 || (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0)
				std::cout << "Invalid location view for XR_REFERENCE_SPACE_TYPE_LOCAL" << std::endl;
		}
		else if(referenceSpaceType == XR_REFERENCE_SPACE_TYPE_STAGE){
			viewLocateInfo.space = xr_space_stage;
			xr_result = xrLocateViews(xr_session, &viewLocateInfo, &viewState, (uint32_t)views.size(), &viewCountOutput, views.data());
			if(!xrCheckResult(xr_instance, xr_result, "xrLocateViews (XR_REFERENCE_SPACE_TYPE_STAGE)"))
				return false;
			if((viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0 || (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0)
				std::cout << "Invalid location view for XR_REFERENCE_SPACE_TYPE_STAGE" << std::endl;
		}
		else{
			std::cout << "Invalid reference space type (" << referenceSpaceType << ")" << std::endl;
			return false;
		}

		// call render callback to get frames
		if(renderCallback)
			renderCallback(views.size(), views.data(), xr_view_configuration_views.data());
		else if(renderCallbackFunction)
			renderCallbackFunction(views.size(), views, xr_view_configuration_views);
		// TODO: render if there are images

		// render view to the appropriate part of the swapchain image
		projectionLayerViews.resize(viewCountOutput);
		for(uint32_t i = 0; i < viewCountOutput; i++){
			// Each view has a separate swapchain which is acquired, rendered to, and released
			const SwapchainHandler viewSwapchain = xr_swapchains_handlers[i];

			XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};

			uint32_t swapchainImageIndex;
			xr_result = xrAcquireSwapchainImage(viewSwapchain.handle, &acquireInfo, &swapchainImageIndex);
			if(!xrCheckResult(xr_instance, xr_result, "xrAcquireSwapchainImage"))
				return false;
			xr_graphics_handler.acquireContext(xr_graphics_binding, "xrAcquireSwapchainImage");

			XrSwapchainImageWaitInfo waitInfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
			waitInfo.timeout = XR_INFINITE_DURATION;
			xr_result = xrWaitSwapchainImage(viewSwapchain.handle, &waitInfo);
			if(!xrCheckResult(xr_instance, xr_result, "xrWaitSwapchainImage"))
				return false;
			xr_graphics_handler.acquireContext(xr_graphics_binding, "xrWaitSwapchainImage");

			projectionLayerViews[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
			projectionLayerViews[i].pose = views[i].pose;
			projectionLayerViews[i].fov = views[i].fov;
			projectionLayerViews[i].subImage.swapchain = viewSwapchain.handle;
			projectionLayerViews[i].subImage.imageRect.offset = {0, 0};
			projectionLayerViews[i].subImage.imageRect.extent = {viewSwapchain.width, viewSwapchain.height};

			// render frame
			if((renderCallback || renderCallbackFunction) && (framesWidth[i] && framesHeight[i])){
				const XrSwapchainImageBaseHeader* const swapchainImage = (XrSwapchainImageBaseHeader*)&viewSwapchain.images[swapchainImageIndex];
				// FIXME: use format (vulkan: 43, opengl: 34842)
				// xr_graphics_handler.renderView(projectionLayerViews[i], swapchainImage, 43);
				xr_graphics_handler.renderViewFromImage(projectionLayerViews[i], swapchainImage, 43, framesWidth[i], framesHeight[i], framesData[i], framesRGBA);
				cleanFrames();
			}

			XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
			xr_result = xrReleaseSwapchainImage(viewSwapchain.handle, &releaseInfo);
			if(!xrCheckResult(xr_instance, xr_result, "xrReleaseSwapchainImage"))
				return false;
		}

		if(referenceSpaceType == XR_REFERENCE_SPACE_TYPE_VIEW)
			layer.space = xr_space_view;
		else if(referenceSpaceType == XR_REFERENCE_SPACE_TYPE_LOCAL)
			layer.space = xr_space_local;
		else if(referenceSpaceType == XR_REFERENCE_SPACE_TYPE_STAGE)
			layer.space = xr_space_stage;
		layer.viewCount = (uint32_t)projectionLayerViews.size();
		layer.views = projectionLayerViews.data();

		layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&layer));
	}

	// end frame
	XrFrameEndInfo frameEndInfo{XR_TYPE_FRAME_END_INFO};
	frameEndInfo.displayTime = frameState.predictedDisplayTime;
	frameEndInfo.environmentBlendMode = environmentBlendMode;
	frameEndInfo.layerCount = (uint32_t)layers.size();
	frameEndInfo.layers = layers.data();

	xr_result = xrEndFrame(xr_session, &frameEndInfo);
	if(!xrCheckResult(xr_instance, xr_result, "xrEndFrame"))
		return false;

	return true;
}

bool OpenXrApplication::setFrameByIndex(int index, int width, int height, void * frame, bool rgba){
	if(index < 0 || (size_t)index >= framesData.size())
		return false;
	framesWidth[index] = width;
	framesHeight[index] = height;
	framesData[index] = frame;
	framesRGBA = rgba;
	return true;
}


#ifdef APPLICATION
int main(){
	OpenXrApplication * app = new OpenXrApplication();

	// create instance
	string applicationName = "Omniverse (VR)";
	string engineName = "OpenXR Engine";
	vector<string> requestedApiLayers = { "XR_APILAYER_LUNARG_core_validation" };
	vector<string> requestedExtensions = {
#ifdef XR_USE_GRAPHICS_API_VULKAN
	#ifdef APP_USE_VULKAN2
		XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME,
	#else
		XR_KHR_VULKAN_ENABLE_EXTENSION_NAME,
	#endif
#endif
#ifdef XR_USE_GRAPHICS_API_OPENGL
		XR_KHR_OPENGL_ENABLE_EXTENSION_NAME,
#endif
	};
	app->createInstance(applicationName, engineName, requestedApiLayers, requestedExtensions);
	
	app->getSystem(XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY, XR_ENVIRONMENT_BLEND_MODE_OPAQUE, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO);
	app->createSession();

	bool exitRenderLoop = false;

#ifdef XR_USE_GRAPHICS_API_OPENGL
	SDL_Event sdl_event;
#endif

	while(true){
#ifdef XR_USE_GRAPHICS_API_OPENGL
		while(SDL_PollEvent(&sdl_event))
			if (sdl_event.type == SDL_QUIT || (sdl_event.type == SDL_KEYDOWN && sdl_event.key.keysym.sym == SDLK_ESCAPE))
				return 0;
#endif

		app->pollEvents(&exitRenderLoop);
		if(exitRenderLoop)
			break;

		if(app->isSessionRunning()){
			vector<ActionState> requestedActionStates;
			app->pollActions(requestedActionStates);
			app->renderViews(XR_REFERENCE_SPACE_TYPE_LOCAL);
		}
		else{
			// Throttle loop since xrWaitFrame won't be called.
			// std::this_thread::sleep_for(std::chrono::milliseconds(250));
		}
	}
	return 0;
}
#endif


#ifdef CTYPES
extern "C"
{
    OpenXrApplication * openXrApplication(){ return new OpenXrApplication(); }
    
	bool createInstance(OpenXrApplication * app, const char * applicationName, const char * engineName, const char ** apiLayers, int apiLayersLength, const char ** extensions, int extensionsLength){
		vector<string> requestedApiLayers;
		for(int i = 0; i < apiLayersLength; i++)
			requestedApiLayers.push_back(apiLayers[i]);
		vector<string> requestedExtensions;
		for(int i = 0; i < extensionsLength; i++)
			requestedExtensions.push_back(extensions[i]);
		return app->createInstance(applicationName, engineName, requestedApiLayers, requestedExtensions); 
	}
    bool getSystem(OpenXrApplication * app, int formFactor, int blendMode, int configurationType){ 
		return app->getSystem(XrFormFactor(formFactor), XrEnvironmentBlendMode(blendMode), XrViewConfigurationType(configurationType)); 
	}
	bool addAction(OpenXrApplication * app, const char * stringPath, int actionType){ return app->addAction(stringPath, XrActionType(actionType)); }

    bool createSession(OpenXrApplication * app){ return app->createSession(); }

    bool pollEvents(OpenXrApplication * app, bool * exitLoop){ return app->pollEvents(exitLoop); }
    bool isSessionRunning(OpenXrApplication * app){ return app->isSessionRunning(); }

    bool pollActions(OpenXrApplication * app, ActionState * actionStates, int actionStatesLength){
		vector<ActionState> requestedActionStates;
		bool status = app->pollActions(requestedActionStates);
		if(requestedActionStates.size() <= actionStatesLength)
			for(size_t i = 0; i < requestedActionStates.size(); i++)
				actionStates[i] = requestedActionStates[i];
		return status;
	}
    bool renderViews(OpenXrApplication * app, int referenceSpaceType){ return app->renderViews(XrReferenceSpaceType(referenceSpaceType)); }

	bool setFrames(OpenXrApplication * app, int leftWidth, int leftHeight, void * leftData, int rightWidth, int rightHeight, void * rightData, bool rgba){
		if(app->getViewConfigurationViewsSize() == 1)
			return app->setFrameByIndex(0, leftWidth, leftHeight, leftData, rgba);
		else if(app->getViewConfigurationViewsSize() == 2){
			bool status = app->setFrameByIndex(0, leftWidth, leftHeight, leftData, rgba);
			return status && app->setFrameByIndex(1, rightWidth, rightHeight, rightData, rgba);
		}
		return false;
	}

	void setRenderCallback(OpenXrApplication * app, void (*callback)(int, XrView*, XrViewConfigurationView*)){ app->setRenderCallbackFromPointer(callback); }

	int getViewConfigurationViewsSize(OpenXrApplication * app){ return app->getViewConfigurationViewsSize(); }
	bool getViewConfigurationViews(OpenXrApplication * app, XrViewConfigurationView * views, int viewsLength){
		if((size_t)viewsLength != app->getViewConfigurationViewsSize())
			return false;
		vector<XrViewConfigurationView> viewConfigurationView = app->getViewConfigurationViews();
		for(size_t i = 0; i < viewConfigurationView.size(); i++)
			views[i] = viewConfigurationView[i];
		return true;
	}
}
#endif