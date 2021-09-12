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
#include <SDL2/SDL_image.h>
#endif

#include <vector>
#include <stdio.h>
#include <string.h> 
#include <iostream>
using namespace std;

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof((x)[0]))
#endif




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

bool xrCheckResult(XrInstance xr_instance, XrResult xr_result, const char * message = nullptr){
	if(XR_SUCCEEDED(xr_result))
		return true;

	if (xr_instance != NULL){
		char xr_result_as_string[XR_MAX_RESULT_STRING_SIZE];
		xrResultToString(xr_instance, xr_result, xr_result_as_string);
		if (message != NULL)
			std::cout << "FAILED with code: " << xr_result << " (" << xr_result_as_string << "). " << message << std::endl;
		else
			std::cout << "FAILED with code: " << xr_result << " (" << xr_result_as_string << ")" << std::endl;
	}
	else{
		if (message != NULL)
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
	void renderViewFromImage(const XrCompositionLayerProjectionView &, const XrSwapchainImageBaseHeader *, int64_t, int, int, void *);

	uint32_t getSupportedSwapchainSampleCount(XrViewConfigurationView){ return 1; }
};

OpenGLHandler::OpenGLHandler()
{
}

OpenGLHandler::~OpenGLHandler()
{
}

void OpenGLHandler::loadTexture(string path, GLuint * textureId){
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
	// loadTexture("texture.png", &texture);
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

void OpenGLHandler::renderViewFromImage(const XrCompositionLayerProjectionView & layerView, const XrSwapchainImageBaseHeader * swapchainImage, int64_t swapchainFormat, int frameWidth, int frameHeight, void * frameData){
		// load texture
		glBindTexture(GL_TEXTURE_2D, texture);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frameWidth, frameHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, frameData);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

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



typedef void (*CallbackRender)(int cViews, int * pViews);

class OpenXrApplication{
private:
	XrResult xr_result;

	XrInstance xr_instance = {};
	XrSystemId xr_system_id = XR_NULL_SYSTEM_ID;
	XrSession xr_session = {};

	XrSpace xr_space_view = {};
	XrSpace xr_space_local = {};
	XrSpace xr_space_stage = {};
	
	XrActionSet xr_action_set;

	vector<XrPath> subactionPaths;
	XrAction actionHeadPose;

	vector<XrViewConfigurationView> viewConfigurationViews;
	vector<SwapchainHandler> swapchainsHandlers;

	vector<int> framesWidth;
	vector<int> framesHeight;
	vector<void*> framesData;
	void (*renderCallback)(int, XrView*, XrViewConfigurationView*);

	bool flagSessionRunning = false;

	// config
	XrEnvironmentBlendMode environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
	XrViewConfigurationType configViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

	// actions
	XrSpace spaceHead;

#ifdef XR_USE_GRAPHICS_API_VULKAN
	XrGraphicsBindingVulkan2KHR xr_graphics_binding = {XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR};
	VulkanHandler xr_graphics_handler;
#endif
#ifdef XR_USE_GRAPHICS_API_OPENGL
	XrGraphicsBindingOpenGLXlibKHR xr_graphics_binding = {XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR};
	OpenGLHandler xr_graphics_handler;
#endif

	bool defineLayers(vector<const char*> requestedApiLayers, vector<const char*> & enabledApiLayers);
	bool defineExtensions(vector<const char*> requestedExtensions, vector<const char*> & enabledExtensions);

	bool getInstanceProperties();
	bool getSystemProperties();
	bool getBlendModes(XrEnvironmentBlendMode);
	bool getViewConfiguration(XrViewConfigurationType);	

	bool defineReferenceSpaces();
	bool defineSessionSpaces();
	bool defineSwapchains();

public:
	OpenXrApplication();
	~OpenXrApplication();

	bool createInstance(const char *, const char *, vector<const char*>, vector<const char*>);
	bool getSystem(XrFormFactor, XrEnvironmentBlendMode, XrViewConfigurationType); 
	bool createActionSet();
	bool createSession();

	bool pollEvents(bool *);
	bool pollActions();

	bool renderViews(); 
	bool setFrameByIndex(int, int, int, void *);
	void setRenderCallback(void (*callback)(int, XrView*, XrViewConfigurationView*)){ renderCallback = callback; };

	bool isSessionRunning(){ return flagSessionRunning; }
	int getViewSize(){ return viewConfigurationViews.size(); }
};

OpenXrApplication::OpenXrApplication(){
	renderCallback = nullptr;
}

OpenXrApplication::~OpenXrApplication(){
	xrDestroySpace(xr_space_view);
	xrDestroySpace(xr_space_local);
	xrDestroySpace(xr_space_stage);

	xrDestroySession(xr_session);
 	xrDestroyInstance(xr_instance);
}

bool OpenXrApplication::defineLayers(vector<const char*> requestedApiLayers, vector<const char*> & enabledApiLayers){
	// TODO: check if all required layers are availables
	uint32_t propertyCountOutput;
	xr_result = xrEnumerateApiLayerProperties(0, &propertyCountOutput, nullptr);
	if(!xrCheckResult(NULL, xr_result, "xrEnumerateApiLayerProperties"))
		return false;
	vector<XrApiLayerProperties> apiLayerProperties(propertyCountOutput, {XR_TYPE_API_LAYER_PROPERTIES});
	xr_result = xrEnumerateApiLayerProperties(propertyCountOutput, &propertyCountOutput, apiLayerProperties.data());
	if(!xrCheckResult(NULL, xr_result, "xrEnumerateApiLayerProperties"))
		return false;

	std::cout << "OpenXR API layers available (" << apiLayerProperties.size() << ")" << std::endl;
	for (size_t i = 0; i < apiLayerProperties.size(); i++){
		std::cout << "  |-- " << apiLayerProperties[i].layerName << std::endl;
		for (size_t j = 0; j < requestedApiLayers.size(); j++)
			if (strcmp(apiLayerProperties[i].layerName, requestedApiLayers[j]) == 0){
				enabledApiLayers.push_back(requestedApiLayers[j]);
				std::cout << "  |   (requested)" << std::endl;
				break;
			}
	}
	return true;
}

bool OpenXrApplication::defineExtensions(vector<const char*> requestedExtensions, vector<const char*> & enabledExtensions){
	// TODO: check if all required extensions are availables 
	uint32_t propertyCountOutput;
	xr_result = xrEnumerateInstanceExtensionProperties(nullptr, 0, &propertyCountOutput, nullptr);
	if(!xrCheckResult(NULL, xr_result, "xrEnumerateInstanceExtensionProperties"))
		return false;
	vector<XrExtensionProperties> extensionProperties(propertyCountOutput, {XR_TYPE_EXTENSION_PROPERTIES});
	xr_result = xrEnumerateInstanceExtensionProperties(nullptr, propertyCountOutput, &propertyCountOutput, extensionProperties.data());
	if(!xrCheckResult(NULL, xr_result, "xrEnumerateInstanceExtensionProperties"))
		return false;
	
	std::cout << "OpenXR extensions available (" << extensionProperties.size() << ")" << std::endl;
	for (size_t i = 0; i < extensionProperties.size(); i++){
		std::cout << "  |-- " << extensionProperties[i].extensionName << std::endl;
		for (size_t j = 0; j < requestedExtensions.size(); j++)
			if (strcmp(extensionProperties[i].extensionName, requestedExtensions[j]) == 0){
				enabledExtensions.push_back(requestedExtensions[j]);
				std::cout << "  |   (requested)" << std::endl;
				break;
			}
	}
	return true;
}

bool OpenXrApplication::getInstanceProperties(){
	XrInstanceProperties instanceProperties = {XR_TYPE_INSTANCE_PROPERTIES};
	xr_result = xrGetInstanceProperties(xr_instance, &instanceProperties);
	if(!xrCheckResult(xr_instance, xr_result, "xrGetInstanceProperties"))
		return false;

	std::cout << "Runtime" << std::endl;
	std::cout << "  |-- name: " << instanceProperties.runtimeName << std::endl;
	std::cout << "  |-- version: " << XR_VERSION_MAJOR(instanceProperties.runtimeVersion) << "." << XR_VERSION_MINOR(instanceProperties.runtimeVersion) << "." << XR_VERSION_PATCH(instanceProperties.runtimeVersion) << std::endl;
	return true;
}

bool OpenXrApplication::getSystemProperties(){
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

bool OpenXrApplication::getBlendModes(XrEnvironmentBlendMode blendMode){
	// TODO: check if the environmentBlendMode is available
	uint32_t propertyCountOutput;
	xr_result = xrEnumerateEnvironmentBlendModes(xr_instance, xr_system_id, configViewConfigurationType, 0, &propertyCountOutput, nullptr);
	if(!xrCheckResult(NULL, xr_result, "xrEnumerateEnvironmentBlendModes"))
		return false;
	vector<XrEnvironmentBlendMode> environmentBlendModes(propertyCountOutput);
	xr_result = xrEnumerateEnvironmentBlendModes(xr_instance, xr_system_id, configViewConfigurationType, propertyCountOutput, &propertyCountOutput, environmentBlendModes.data());
	if(!xrCheckResult(NULL, xr_result, "xrEnumerateEnvironmentBlendModes"))
		return false;

	environmentBlendMode = blendMode;
	std::cout << "Environment blend modes (" << environmentBlendModes.size() << ")" << std::endl;
	for (size_t i = 0; i < environmentBlendModes.size(); i++)
		std::cout << "  |-- mode: " << environmentBlendModes[i] << " (https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrEnvironmentBlendMode)" << std::endl;
	return true;
}

bool OpenXrApplication::getViewConfiguration(XrViewConfigurationType configurationType){
	// TODO: check if configurationType is available
	uint32_t propertyCountOutput;
	xr_result = xrEnumerateViewConfigurations(xr_instance, xr_system_id, 0, &propertyCountOutput, nullptr);
	if(!xrCheckResult(NULL, xr_result, "xrEnumerateViewConfigurations"))
		return false;
	vector<XrViewConfigurationType> viewConfigurationTypes(propertyCountOutput);
	xr_result = xrEnumerateViewConfigurations(xr_instance, xr_system_id, propertyCountOutput, &propertyCountOutput, viewConfigurationTypes.data());
	if(!xrCheckResult(NULL, xr_result, "xrEnumerateViewConfigurations"))
		return false;

	std::cout << "View configurations (" << viewConfigurationTypes.size() << ")" << std::endl;
	for (size_t i = 0; i < viewConfigurationTypes.size(); i++)
		std::cout << "  |-- type " << viewConfigurationTypes[i] << " (https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrViewConfigurationType)" << std::endl;
	
	// view configuration properties
	configViewConfigurationType = configurationType;
	XrViewConfigurationProperties viewConfigurationProperties = {XR_TYPE_VIEW_CONFIGURATION_PROPERTIES};
	xr_result = xrGetViewConfigurationProperties(xr_instance, xr_system_id, configViewConfigurationType, &viewConfigurationProperties);
	if(!xrCheckResult(NULL, xr_result, "xrGetViewConfigurationProperties"))
		return false;

	std::cout << "View configuration properties" << std::endl;
		std::cout << "  |-- configuration type: " << viewConfigurationProperties.viewConfigurationType << " (https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrViewConfigurationType)" << std::endl;
		std::cout << "  |-- fov mutable (bool): " << viewConfigurationProperties.fovMutable << std::endl;
	
	// view configuration views
	xr_result = xrEnumerateViewConfigurationViews(xr_instance, xr_system_id, configViewConfigurationType, 0, &propertyCountOutput, nullptr);
	if(!xrCheckResult(NULL, xr_result, "xrEnumerateViewConfigurationViews"))
		return false;
	viewConfigurationViews.resize(propertyCountOutput, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
	xr_result = xrEnumerateViewConfigurationViews(xr_instance, xr_system_id, configViewConfigurationType, propertyCountOutput, &propertyCountOutput, viewConfigurationViews.data());
	if(!xrCheckResult(NULL, xr_result, "xrEnumerateViewConfigurationViews"))
		return false;
	
	std::cout << "View configuration views (" << viewConfigurationViews.size() << ")" << std::endl;
	for (size_t i = 0; i < viewConfigurationViews.size(); i++){
		std::cout << "  |-- view " << i << std::endl;
		std::cout << "  |     |-- recommended resolution: " << viewConfigurationViews[i].recommendedImageRectWidth << " x " << viewConfigurationViews[i].recommendedImageRectHeight << std::endl;
		std::cout << "  |     |-- max resolution: " << viewConfigurationViews[i].maxImageRectWidth << " x " << viewConfigurationViews[i].maxImageRectHeight << std::endl;
		std::cout << "  |     |-- recommended swapchain samples: " << viewConfigurationViews[i].recommendedSwapchainSampleCount << std::endl;
		std::cout << "  |     |-- max swapchain samples: " << viewConfigurationViews[i].maxSwapchainSampleCount << std::endl;
	}

	// resize frame buffers
	framesData.resize(viewConfigurationViews.size());
	framesWidth.resize(viewConfigurationViews.size());
	framesHeight.resize(viewConfigurationViews.size());

	return true;
}

bool OpenXrApplication::defineReferenceSpaces(){
	// get reference spaces
	uint32_t propertyCountOutput;
	xr_result = xrEnumerateReferenceSpaces(xr_session, 0, &propertyCountOutput, nullptr);
	if(!xrCheckResult(NULL, xr_result, "xrEnumerateReferenceSpaces"))
		return false;
	vector<XrReferenceSpaceType> referenceSpaces(propertyCountOutput);
	xr_result = xrEnumerateReferenceSpaces(xr_session, propertyCountOutput, &propertyCountOutput, referenceSpaces.data());
	if(!xrCheckResult(NULL, xr_result, "xrEnumerateReferenceSpaces"))
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
			if(!xrCheckResult(NULL, xr_result, "xrCreateReferenceSpace (XR_REFERENCE_SPACE_TYPE_VIEW)"))
				return false;

			// get bounds
			xr_result = xrGetReferenceSpaceBoundsRect(xr_session, XR_REFERENCE_SPACE_TYPE_VIEW, &spaceBounds);
			if(!xrCheckResult(NULL, xr_result, "xrGetReferenceSpaceBoundsRect (XR_REFERENCE_SPACE_TYPE_VIEW)"))
				return false;
			
			std::cout << "  |     |-- reference space bounds" << std::endl;
			std::cout << "  |     |     |-- width: " << spaceBounds.width << std::endl;
			std::cout << "  |     |     |-- height: " << spaceBounds.height << std::endl;
		}
		// local
		else if(referenceSpaces[i] == XR_REFERENCE_SPACE_TYPE_LOCAL){
			xr_result = xrCreateReferenceSpace(xr_session, &referenceSpaceCreateInfo, &xr_space_local);
			if(!xrCheckResult(NULL, xr_result, "xrCreateReferenceSpace (XR_REFERENCE_SPACE_TYPE_LOCAL)"))
				return false;

			// get bounds
			xr_result = xrGetReferenceSpaceBoundsRect(xr_session, XR_REFERENCE_SPACE_TYPE_LOCAL, &spaceBounds);
			if(!xrCheckResult(NULL, xr_result, "xrGetReferenceSpaceBoundsRect (XR_REFERENCE_SPACE_TYPE_LOCAL)"))
				return false;
			
			std::cout << "  |     |-- reference space bounds" << std::endl;
			std::cout << "  |     |     |-- width: " << spaceBounds.width << std::endl;
			std::cout << "  |     |     |-- height: " << spaceBounds.height << std::endl;
		}
		// stage
		else if(referenceSpaces[i] == XR_REFERENCE_SPACE_TYPE_STAGE){
			xr_result = xrCreateReferenceSpace(xr_session, &referenceSpaceCreateInfo, &xr_space_stage);
			if(!xrCheckResult(NULL, xr_result, "xrCreateReferenceSpace (XR_REFERENCE_SPACE_TYPE_STAGE)"))
				return false;

			// get bounds
			xr_result = xrGetReferenceSpaceBoundsRect(xr_session, XR_REFERENCE_SPACE_TYPE_STAGE, &spaceBounds);
			if(!xrCheckResult(NULL, xr_result, "xrGetReferenceSpaceBoundsRect (XR_REFERENCE_SPACE_TYPE_STAGE)"))
				return false;
			
			std::cout << "  |     |-- reference space bounds" << std::endl;
			std::cout << "  |     |     |-- width: " << spaceBounds.width << std::endl;
			std::cout << "  |     |     |-- height: " << spaceBounds.height << std::endl;
		}
	}
	return true;
}

bool OpenXrApplication::defineSessionSpaces(){
	// TODO: use an action handler
	XrSessionActionSetsAttachInfo attachInfo = {XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
	attachInfo.countActionSets = 1;
	attachInfo.actionSets = &xr_action_set;
	xr_result = xrAttachSessionActionSets(xr_session, &attachInfo);
	if(!xrCheckResult(NULL, xr_result, "xrAttachSessionActionSets"))
		return false;

	XrActionSpaceCreateInfo actionSpaceInfo = {XR_TYPE_ACTION_SPACE_CREATE_INFO};
	actionSpaceInfo.action = actionHeadPose;
	actionSpaceInfo.poseInActionSpace.orientation.w = 1.f;
	actionSpaceInfo.subactionPath = subactionPaths[0];
	xr_result = xrCreateActionSpace(xr_session, &actionSpaceInfo, &spaceHead);
	if(!xrCheckResult(NULL, xr_result, "xrCreateActionSpace"))
		return false;
}

bool OpenXrApplication::defineSwapchains(){
	// get swapchain Formats
	uint32_t propertyCountOutput;
	xr_result = xrEnumerateSwapchainFormats(xr_session, 0, &propertyCountOutput, nullptr);
	if(!xrCheckResult(NULL, xr_result, "xrEnumerateSwapchainFormats"))
		return false;
	vector<int64_t> swapchainFormats(propertyCountOutput);
	xr_result = xrEnumerateSwapchainFormats(xr_session, propertyCountOutput, &propertyCountOutput, swapchainFormats.data());
	if(!xrCheckResult(NULL, xr_result, "xrEnumerateSwapchainFormats"))
		return false;

	// select swapchain format
#ifdef XR_USE_GRAPHICS_API_VULKAN
	int64_t supportedSwapchainFormats[] = {VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM};
#endif
#ifdef XR_USE_GRAPHICS_API_OPENGL
	int64_t supportedSwapchainFormats[] = {GL_RGB10_A2, GL_RGBA16F, GL_RGBA8, GL_RGBA8_SNORM};
#endif

	int64_t selectedSwapchainFormats = -1;
	for (size_t i = 0; i < swapchainFormats.size(); i++){
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
	std::cout << "Created swapchain (" << viewConfigurationViews.size() << ")" << std::endl;

	for(uint32_t i = 0; i < viewConfigurationViews.size(); i++){
		XrSwapchainCreateInfo swapchainCreateInfo = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
		swapchainCreateInfo.arraySize = 1;
		swapchainCreateInfo.format = selectedSwapchainFormats;
		swapchainCreateInfo.width = viewConfigurationViews[i].recommendedImageRectWidth;
		swapchainCreateInfo.height = viewConfigurationViews[i].recommendedImageRectHeight;
		swapchainCreateInfo.mipCount = 1;
		swapchainCreateInfo.faceCount = 1;
		swapchainCreateInfo.sampleCount = xr_graphics_handler.getSupportedSwapchainSampleCount(viewConfigurationViews[i]);
		swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
		
		SwapchainHandler swapchain;
		xr_result = xrCreateSwapchain(xr_session, &swapchainCreateInfo, &swapchain.handle);
		if(!xrCheckResult(NULL, xr_result, "xrCreateSwapchain"))
			return false;

		swapchain.width = swapchainCreateInfo.width;
		swapchain.height = swapchainCreateInfo.height;

		std::cout << "  |-- swapchain: " << i << std::endl;
		std::cout << "  |     |-- width: " << swapchainCreateInfo.width << std::endl;
		std::cout << "  |     |-- height: " << swapchainCreateInfo.height << std::endl;
		std::cout << "  |     |-- sample count: " << swapchainCreateInfo.sampleCount << std::endl;
	
		// enumerate swapchain images
		xr_result = xrEnumerateSwapchainImages(swapchain.handle, 0, &propertyCountOutput, nullptr);
		if(!xrCheckResult(NULL, xr_result, "xrEnumerateSwapchainImages"))
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

		swapchainsHandlers.push_back(swapchain);
	}

#ifdef XR_USE_GRAPHICS_API_OPENGL
	// acquire GL context
	glXMakeCurrent(xr_graphics_binding.xDisplay, xr_graphics_binding.glxDrawable, xr_graphics_binding.glxContext);
#endif
	return true;
}


bool OpenXrApplication::createInstance(const char * applicationName, const char * engineName, vector<const char*> requestedApiLayers, vector<const char*> requestedExtensions){
	vector<const char*> enabledApiLayers;
	vector<const char*> enabledExtensions;

	// layers
	if(!defineLayers(requestedApiLayers, enabledApiLayers))
		return false;

	// extensions
	if(!defineExtensions(requestedExtensions, enabledExtensions))
		return false;

	// initialize OpenXR (create instance) with the enabled extensions and layers
	XrInstanceCreateInfo createInfo = {XR_TYPE_INSTANCE_CREATE_INFO};
	createInfo.next = NULL;
	createInfo.createFlags = 0;
	createInfo.enabledApiLayerCount = enabledApiLayers.size();
	createInfo.enabledApiLayerNames = enabledApiLayers.data();
	createInfo.enabledExtensionCount = enabledExtensions.size();
	createInfo.enabledExtensionNames = enabledExtensions.data();
	createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
	createInfo.applicationInfo.applicationVersion = 1;
	createInfo.applicationInfo.engineVersion = 1;

	strncpy(createInfo.applicationInfo.applicationName, applicationName, XR_MAX_APPLICATION_NAME_SIZE);
	strncpy(createInfo.applicationInfo.engineName, engineName, XR_MAX_ENGINE_NAME_SIZE);

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
	
	if(!getInstanceProperties())
		return false;
	if(!getSystemProperties())
		return false;
 	if(!getBlendModes(blendMode))
	 	return false;
	if(!getViewConfiguration(configurationType))
		return false;
	return true;
}

bool OpenXrApplication::createActionSet(){
	// TODO: use an action handler
	
	XrActionSetCreateInfo actionSetInfo = {XR_TYPE_ACTION_SET_CREATE_INFO};
	strcpy(actionSetInfo.actionSetName, "gameplay");
	strcpy(actionSetInfo.localizedActionSetName, "Gameplay");
	actionSetInfo.priority = 0;

	xr_result = xrCreateActionSet(xr_instance, &actionSetInfo, &xr_action_set);
	if(!xrCheckResult(NULL, xr_result, "xrCreateActionSet"))
		return false;

	// action space for headset pose
	subactionPaths.resize(1);
	xr_result = xrStringToPath(xr_instance, "/user/head", &subactionPaths[0]);
	if(!xrCheckResult(NULL, xr_result, "xrStringToPath"))
		return false;

	XrActionCreateInfo actionInfo = {XR_TYPE_ACTION_CREATE_INFO};
	actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
	strcpy(actionInfo.actionName, "head_pose");
	strcpy(actionInfo.localizedActionName, "Head Pose");
	actionInfo.countSubactionPaths = uint32_t(subactionPaths.size());
	actionInfo.subactionPaths = subactionPaths.data();
	xr_result = xrCreateAction(xr_action_set, &actionInfo, &actionHeadPose);
	if(!xrCheckResult(NULL, xr_result, "xrCreateAction"))
		return false;

	// // suggest bindings for KHR
	// {
	// 	XrPath khrSimpleInteractionProfilePath;
	// 	xr_result = xrStringToPath(xr_instance, "/interaction_profiles/khr/simple_controller", &khrSimpleInteractionProfilePath);
	// 	if(!xrCheckResult(NULL, xr_result, "xrStringToPath"))
	// 		return false;
	// 	vector<XrActionSuggestedBinding> bindings = {{{actionHeadPose, },
	// 													{m_input.grabAction, selectPath[Side::RIGHT]},
	// 													{m_input.poseAction, posePath[Side::LEFT]},
	// 													{m_input.poseAction, posePath[Side::RIGHT]},
	// 													{m_input.quitAction, menuClickPath[Side::LEFT]},
	// 													{m_input.quitAction, menuClickPath[Side::RIGHT]},
	// 													{m_input.vibrateAction, hapticPath[Side::LEFT]},
	// 													{m_input.vibrateAction, hapticPath[Side::RIGHT]}}};
	// 	XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
	// 	suggestedBindings.interactionProfile = khrSimpleInteractionProfilePath;
	// 	suggestedBindings.suggestedBindings = bindings.data();
	// 	suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
	// 	CHECK_XRCMD(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings));
	// }

	// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#_sample_code
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
											viewConfigurationViews[0].recommendedImageRectWidth,
											viewConfigurationViews[0].recommendedImageRectHeight))
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
	if(!xrCheckResult(NULL, xr_result, "xrCreateSession"))
		return false;

	// reference spaces
	if(!defineReferenceSpaces())
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
		if(!xrCheckResult(NULL, xr_result, "xrPollEvent"))
			return false;

		// process messages
		switch (event.type){
			// session state changed
			case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
				const XrEventDataSessionStateChanged & sessionStateChangedEvent = *reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
				
				// check session
				if((sessionStateChangedEvent.session != XR_NULL_HANDLE) && (sessionStateChangedEvent.session != xr_session)){
					std::cout << "XrEventDataSessionStateChanged for unknown session " << sessionStateChangedEvent.session << std::endl;
					return false;
				}

				switch (sessionStateChangedEvent.state){
					case XR_SESSION_STATE_READY: {
						XrSessionBeginInfo sessionBeginInfo = {XR_TYPE_SESSION_BEGIN_INFO};
						sessionBeginInfo.primaryViewConfigurationType = configViewConfigurationType;
						xr_result = xrBeginSession(xr_session, &sessionBeginInfo);
						if(!xrCheckResult(NULL, xr_result, "xrBeginSession"))
							return false;

						flagSessionRunning = true;
						std::cout << "Event: XR_SESSION_STATE_READY (xrBeginSession)" << std::endl;
						break;
					}
					case XR_SESSION_STATE_STOPPING: {
						xr_result = xrEndSession(xr_session);
						if(!xrCheckResult(NULL, xr_result, "xrEndSession"))
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
				// LogActionSourceName(m_input.grabAction, "Grab");
				// LogActionSourceName(m_input.quitAction, "Quit");
				// LogActionSourceName(m_input.poseAction, "Pose");
				// LogActionSourceName(m_input.vibrateAction, "Vibrate");
				break;
			}
			case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
				break;
			case XR_EVENT_UNAVAILABLE:
				return true;
			case XR_TYPE_EVENT_DATA_BUFFER:
				return true;
			default:
				break;
		}
	}
	return true;
}

bool OpenXrApplication::pollActions(){
	// TODO: implement
	return true;
}

bool OpenXrApplication::setFrameByIndex(int index, int width, int height, void * frame){
	if(index < 0 || index >= framesData.size())
		return false;
	framesWidth[index] = width;
	framesHeight[index] = height;
	framesData[index] = frame;
	return true;
}

bool OpenXrApplication::renderViews(){
	xr_graphics_handler.acquireContext(xr_graphics_binding, "xrWaitFrame");

	XrFrameWaitInfo frameWaitInfo = {XR_TYPE_FRAME_WAIT_INFO};
	XrFrameState frameState = {XR_TYPE_FRAME_STATE};
	xr_result = xrWaitFrame(xr_session, &frameWaitInfo, &frameState);
	if(!xrCheckResult(NULL, xr_result, "xrWaitFrame"))
		return false;

	XrFrameBeginInfo frameBeginInfo = {XR_TYPE_FRAME_BEGIN_INFO};
	xr_result = xrBeginFrame(xr_session, &frameBeginInfo);
	if(!xrCheckResult(NULL, xr_result, "xrBeginFrame"))
		return false;

	vector<XrCompositionLayerBaseHeader*> layers;
	XrCompositionLayerProjection layer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
	vector<XrCompositionLayerProjectionView> projectionLayerViews;

	if(frameState.shouldRender == XR_TRUE){
		XrSpaceLocation spaceLocation = {XR_TYPE_SPACE_LOCATION};
		xr_result = xrLocateSpace(spaceHead, xr_space_local, frameState.predictedDisplayTime, &spaceLocation);
		if(!xrCheckResult(NULL, xr_result, "xrLocateSpace"))
			return false;

		XrSpaceLocationFlags flags = spaceLocation.locationFlags;
		if((flags & XR_SPACE_LOCATION_POSITION_VALID_BIT) && (flags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))
			std::cout << "pose: " << spaceLocation.pose.position.x << "\t" << spaceLocation.pose.position.y << "\t" << spaceLocation.pose.position.z << std::endl;
		
		vector<XrView> views(viewConfigurationViews.size(), {XR_TYPE_VIEW});

		XrViewState viewState = {XR_TYPE_VIEW_STATE};
		uint32_t viewCapacityInput = (uint32_t)views.size();
		uint32_t viewCountOutput;

		XrViewLocateInfo viewLocateInfo = {XR_TYPE_VIEW_LOCATE_INFO};
		viewLocateInfo.viewConfigurationType = configViewConfigurationType;
		viewLocateInfo.displayTime = frameState.predictedDisplayTime;
		viewLocateInfo.space = xr_space_local;

		xr_result = xrLocateViews(xr_session, &viewLocateInfo, &viewState, viewCapacityInput, &viewCountOutput, views.data());
		if(!xrCheckResult(NULL, xr_result, "xrLocateViews"))
			return false;
		if ((viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0 || (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0)
			return false;  // there is no valid tracking poses for the views

		projectionLayerViews.resize(viewCountOutput);

		// call render callback to get frames
		if(renderCallback)
			renderCallback(views.size(), views.data(), viewConfigurationViews.data());

		// render view to the appropriate part of the swapchain image
		for (uint32_t i = 0; i < viewCountOutput; i++){
			// Each view has a separate swapchain which is acquired, rendered to, and released
			const SwapchainHandler viewSwapchain = swapchainsHandlers[i];

			XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};

			uint32_t swapchainImageIndex;
			xr_result = xrAcquireSwapchainImage(viewSwapchain.handle, &acquireInfo, &swapchainImageIndex);
			if(!xrCheckResult(NULL, xr_result, "xrAcquireSwapchainImage"))
				return false;
			xr_graphics_handler.acquireContext(xr_graphics_binding, "xrAcquireSwapchainImage");

			XrSwapchainImageWaitInfo waitInfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
			waitInfo.timeout = XR_INFINITE_DURATION;
			xr_result = xrWaitSwapchainImage(viewSwapchain.handle, &waitInfo);
			if(!xrCheckResult(NULL, xr_result, "xrWaitSwapchainImage"))
				return false;
			xr_graphics_handler.acquireContext(xr_graphics_binding, "xrWaitSwapchainImage");

			projectionLayerViews[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
			projectionLayerViews[i].pose = views[i].pose;
			projectionLayerViews[i].fov = views[i].fov;
			projectionLayerViews[i].subImage.swapchain = viewSwapchain.handle;
			projectionLayerViews[i].subImage.imageRect.offset = {0, 0};
			projectionLayerViews[i].subImage.imageRect.extent = {viewSwapchain.width, viewSwapchain.height};

			// render frame
			if(renderCallback){
				const XrSwapchainImageBaseHeader* const swapchainImage = (XrSwapchainImageBaseHeader*)&viewSwapchain.images[swapchainImageIndex];
				// FIXME: use format (vulkan: 43, opengl: 34842)
				// xr_graphics_handler.renderView(projectionLayerViews[i], swapchainImage, 43);
				xr_graphics_handler.renderViewFromImage(projectionLayerViews[i], swapchainImage, 43, framesWidth[i], framesHeight[i], framesData[i]);
			}

			XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
			xr_result = xrReleaseSwapchainImage(viewSwapchain.handle, &releaseInfo);
			if(!xrCheckResult(NULL, xr_result, "xrReleaseSwapchainImage"))
				return false;
		}

		layer.space = xr_space_local;
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
	if(!xrCheckResult(NULL, xr_result, "xrEndFrame"))
		return false;

	return true;
}





int main(){
	OpenXrApplication * app = new OpenXrApplication();

	// create instance
	char applicationName[] = {"Omniverse (VR)"};
	char engineName[] = {"OpenXR Engine"};
	vector<const char*> requestedApiLayers = { "XR_APILAYER_LUNARG_core_validation" };
	vector<const char*> requestedExtensions = {
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
	app->createActionSet();
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
			app->pollActions();
			// app->renderFrame();
		}
		else{
			// Throttle loop since xrWaitFrame won't be called.
			// std::this_thread::sleep_for(std::chrono::milliseconds(250));
		}
	}
	return 0;
}


extern "C"
{
    OpenXrApplication * openXrApplication(){ return new OpenXrApplication(); }
    
	bool createInstance(OpenXrApplication * app, const char * applicationName, const char * engineName, const char ** apiLayers, int apiLayersLength, const char ** extensions, int extensionsLength){
		vector<const char*> requestedApiLayers;
		for(int i = 0; i < apiLayersLength; i++)
			requestedApiLayers.push_back(apiLayers[i]);
		vector<const char*> requestedExtensions;
		for(int i = 0; i < extensionsLength; i++)
			requestedExtensions.push_back(extensions[i]);
		return app->createInstance(applicationName, engineName, requestedApiLayers, requestedExtensions); 
	}
    bool getSystem(OpenXrApplication * app, int formFactor, int blendMode, int configurationType){ 
		return app->getSystem(XrFormFactor(formFactor), XrEnvironmentBlendMode(blendMode), XrViewConfigurationType(configurationType)); 
	}
    bool createActionSet(OpenXrApplication * app){ return app->createActionSet(); }
    bool createSession(OpenXrApplication * app){ return app->createSession(); }

    bool pollEvents(OpenXrApplication * app, bool * exitLoop){ return app->pollEvents(exitLoop); }
    bool isSessionRunning(OpenXrApplication * app){ return app->isSessionRunning(); }

    bool pollActions(OpenXrApplication * app){ return app->pollActions(); }
    bool renderViews(OpenXrApplication * app){ return app->renderViews(); }

	bool setFrames(OpenXrApplication * app, int leftWidth, int leftHeight, void * leftData, int rightWidth, int rightHeight, void * rightData){
		if(app->getViewSize() == 1)
			return app->setFrameByIndex(0, leftWidth, leftHeight, leftData);
		else if(app->getViewSize() == 2){
			bool status = app->setFrameByIndex(0, leftWidth, leftHeight, leftData);
			return status && app->setFrameByIndex(1, rightWidth, rightHeight, rightData);
		}
		return false;
	}

	void setRenderCallback(OpenXrApplication * app, void (*callback)(int, XrView*, XrViewConfigurationView*)){ app->setRenderCallback(callback); }
}