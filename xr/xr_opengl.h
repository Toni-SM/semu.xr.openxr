
// OpenGL libraries
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif
#ifndef GL3_PROTOTYPES
#define GL3_PROTOTYPES
#endif
#include <GL/gl.h>
#include <GL/glext.h>

// Extra OpenGL/X11 libraries
#include <openxr/xr_dependencies.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
// support for loading image from file
#ifdef APPLICATION_IMAGE
#include <SDL2/SDL_image.h> 
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_reflection.h>

#include <vector>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <algorithm>
#include <functional> 
using namespace std;


// shaders to display an image
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

	bool initResources(XrInstance xr_instance, XrSystemId xr_system_id);
	bool getRequirements(XrInstance xr_instance, XrSystemId xr_system_id);

#ifdef XR_USE_PLATFORM_XLIB 
	bool initGraphicsBinding(Display** xDisplay, uint32_t* visualid, GLXFBConfig* glxFBConfig, GLXDrawable* glxDrawable, GLXContext* glxContext, int width, int height);
	void acquireContext(XrGraphicsBindingOpenGLXlibKHR, string);
#endif
#ifdef XR_USE_PLATFORM_WIN32
	bool initGraphicsBinding(HDC* hDC, HGLRC* hRC, int width, int height);
	void acquireContext(XrGraphicsBindingOpenGLWin32KHR, string);
#endif

	void renderView(const XrCompositionLayerProjectionView &, const XrSwapchainImageBaseHeader *, int64_t);
	void renderViewFromImage(const XrCompositionLayerProjectionView &, const XrSwapchainImageBaseHeader *, int64_t, int, int, void *, bool);

	uint32_t getSupportedSwapchainSampleCount(XrViewConfigurationView){ return 1; }
};
