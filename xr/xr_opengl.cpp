#include "xr_opengl.h"


#ifdef XR_USE_PLATFORM_XLIB
void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam){
	std::cout << "GL CALLBACK: " << (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "") << " type = 0x" << type << ", severity = 0x" << severity << ", message = " << message << std::endl;
}
#endif
#ifdef XR_USE_PLATFORM_WIN32
void MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam){
	std::cout << "GL CALLBACK: " << (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "") << " type = 0x" << type << ", severity = 0x" << severity << ", message = " << message << std::endl;
}
#endif


OpenGLHandler::OpenGLHandler(){
}

OpenGLHandler::~OpenGLHandler(){
	if(gl_context){
		SDL_GL_DeleteContext(gl_context);
		gl_context = nullptr;
	}
	if(sdl_window){
		SDL_DestroyWindow(sdl_window);
		sdl_window = nullptr;
	}
}

/*
 *	Loads a texture from a file
 *
 *	@param[in] filename - the file to load
 *	@param[out] texture - the texture to load into
 */
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

/* 
 *	Checks if a shader is valid
 *
 *	@param[in] shader - the shader to check
 *	@returns true if the shader is valid, false otherwise
 */
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

/* 
 *	Checks if a program is valid
 *
 *	@param[in] program - the program to check
 *	@returns true if the program is valid, false otherwise
 */
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

#ifdef XR_USE_PLATFORM_XLIB
/*
 *	Acquires a lost context
 *
 * Some OpenXR functions change the current OpenGL context in Linux.
 * https://github.com/ValveSoftware/SteamVR-for-Linux/issues/421
 *
 *  @param[in] graphicsBinding - the graphics binding to use
 *  @param[in] message - the message to display
 */
void OpenGLHandler::acquireContext(XrGraphicsBindingOpenGLXlibKHR graphicsBinding, string message){
	GLXContext context = glXGetCurrentContext();
	if(context != graphicsBinding.glxContext){
		glXMakeCurrent(graphicsBinding.xDisplay, graphicsBinding.glxDrawable, graphicsBinding.glxContext);
	}
}

/*
 *	Initializes the graphics binding for OpenGL
 *
 *  SDL library is used to create a window and OpenGL context
 *
 *	@param[out] xDisplay - the X display
 *	@param[out] visualid - the visual id
 *	@param[out] glxFBConfig - the GLXFBConfig
 *	@param[out] glxDrawable - the GLXDrawable
 *	@param[out] glxContext - the GLXContext
 *	@param[in] width - XR resolution width
 *	@param[in] height - XR resolution height
 *	@returns true if the graphics binding was initialized, false otherwise
 */
bool OpenGLHandler::initGraphicsBinding(Display** xDisplay, uint32_t* visualid, GLXFBConfig* glxFBConfig, GLXDrawable* glxDrawable, GLXContext* glxContext, int width, int height){
	if(SDL_Init(SDL_INIT_VIDEO) < 0){
		std::cout << "Unable to initialize SDL" << std::endl;
		return false;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);

	sdl_window = SDL_CreateWindow("Omniverse (XR)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width / 2, height / 2, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
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
#endif

#ifdef XR_USE_PLATFORM_WIN32
/*
 *	Acquires a lost context
 *
 *  @param[in] graphicsBinding - the graphics binding to use
 *  @param[in] message - the message to display
 */
void OpenGLHandler::acquireContext(XrGraphicsBindingOpenGLWin32KHR graphicsBinding, string message){
	// TODO: Check if this is necessary in Windows
	HGLRC context = wglGetCurrentContext();
	if(context != graphicsBinding.hGLRC){
		wglMakeCurrent(graphicsBinding.hDC, graphicsBinding.hGLRC);
	}
}

/*
 *	Initializes the graphics binding for OpenGL
 *
 *	@param[out] hDC - the device context
 *	@param[out] hGLRC - the OpenGL rendering context
 *	@param[in] width - XR resolution width
 *	@param[in] height - XR resolution height
 *	@returns true if the graphics binding was initialized, false otherwise
 */
bool OpenGLHandler::initGraphicsBinding(HDC* hDC, HGLRC* hRC, int width, int height){
	// TODO
	// if(SDL_Init(SDL_INIT_VIDEO) < 0){
	// 	std::cout << "Unable to initialize SDL" << std::endl;
	// 	return false;
	// }

	// SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	// SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	// SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	// SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);

	// // create our window centered at half the VR resolution
	// sdl_window = SDL_CreateWindow("Omniverse (VR)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width / 2, height / 2, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	// if (!sdl_window){
	// 	std::cout << "Unable to create SDL window" << std::endl;
	// 	return false;
	// }

	// gl_context = SDL_GL_CreateContext(sdl_window);
	// glEnable(GL_DEBUG_OUTPUT);
	// glDebugMessageCallback(MessageCallback, 0);

	// SDL_GL_SetSwapInterval(0);

	// // TODO: check if this is necessary for Windows
	// // _glBlitNamedFramebuffer = (PFNGLBLITNAMEDFRAMEBUFFERPROC)glXGetProcAddressARB((GLubyte*)"glBlitNamedFramebuffer");

	// *hDC = GetDC(sdl_window);
	// *hRC = wglCreateContext(*hDC);
	// wglMakeCurrent(*hDC, *hRC);

	// SDL_HideWindow(sdl_window);
	return true;
}
#endif

/*
 *	Shows thr graphics binding requirements
 *
 *	@param[in] xr_instance - the XR instance
 *	@param[in] xr_system_id - the XR system id
 *	@return true if the graphics binding was initialized, false otherwise
 */
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

/*
 *	Initializes resources (shaders, buffers, etc.)
 *
 *	@param[in] xr_instance - the XR instance
 *	@param[in] xr_system_id - the XR system id
 *	@returns true if the graphics binding was initialized, false otherwise
 */
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
        // render to texture (HMD)
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

		// TODO: render to window (check if it is necessary)
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

		// render to texture (HMD)
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
}
