// Miyoo A30 presentation shim
//
// Keep the shared video implementation untouched. For this platform only,
// redirect its SDL entry points so the 640x480 UI is composed offscreen and
// rotated into the A30's physical 480x640 Mali scanout.

static SDL_Texture *my282_composite;
static int my282_logical_width;
static int my282_logical_height;
static int my282_window_width;
static int my282_window_height;
static GLuint my282_game_fbo;
static GLuint my282_game_texture;
static GLuint my282_game_present_program;
static GLint my282_game_position_attrib;
static GLint my282_game_texcoord_attrib;
static int my282_hold_backlight_for_exit;

#define MY282_BACKLIGHT_PATH "/sys/devices/virtual/disp/disp/attr/lcdbl"
#define MY282_BACKLIGHT_HANDOFF "/tmp/nextui-backlight"

#ifndef FBIO_WAITFORVSYNC
#define FBIO_WAITFORVSYNC _IOW('F', 0x20, __u32)
#endif

static void my282_set_framebuffer_blank(int blank) {
	int fd = open("/dev/fb0", O_RDWR | O_CLOEXEC);
	if (fd < 0) return;
	ioctl(fd, FBIOBLANK, blank ? FB_BLANK_POWERDOWN : FB_BLANK_UNBLANK);
	close(fd);
}

static void my282_wait_for_vsync(void) {
	int fd = open("/dev/fb0", O_RDONLY | O_CLOEXEC);
	if (fd < 0) return;
	__u32 display = 0;
	ioctl(fd, FBIO_WAITFORVSYNC, &display);
	close(fd);
}

static void my282_dim_backlight(void) {
	if (access(MY282_BACKLIGHT_HANDOFF, F_OK) != 0) {
		int raw = getInt(MY282_BACKLIGHT_PATH);
		if (raw > 0) putInt(MY282_BACKLIGHT_HANDOFF, raw);
	}
	SetRawBrightness(0);
}

static void my282_restore_backlight(void) {
	// Once this process starts its exit handoff, cleanup presents must not
	// expose its final (potentially reoriented) framebuffer. A fresh frontend
	// process has its own zero-initialized latch and restores the saved level.
	if (my282_hold_backlight_for_exit) return;
	if (access(MY282_BACKLIGHT_HANDOFF, F_OK) != 0) return;
	int raw = getInt(MY282_BACKLIGHT_HANDOFF);
	unlink(MY282_BACKLIGHT_HANDOFF);
	if (raw > 0) SetRawBrightness(raw);
}

static void my282_prepare_for_process_exit(void) {
	my282_hold_backlight_for_exit = 1;
	my282_dim_backlight();
}

static void my282_configure_gles2(void) {
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
	                    SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
}

static SDL_Window *my282_SDL_CreateWindow(
	const char *title, int x, int y, int width, int height, Uint32 flags
) {
	my282_logical_width = width;
	my282_logical_height = height;
	my282_window_width = height;
	my282_window_height = width;

	// These attributes must be set before mali-fbdev creates the window.
	my282_configure_gles2();
	my282_dim_backlight();
	SDL_Window *window = SDL_CreateWindow(
		title, x, y, my282_window_width, my282_window_height, flags);
	// Mali-fbdev can unblank during window creation. Keep the panel dark until
	// the first correctly rotated frame is ready.
	if (window) my282_dim_backlight();
	else my282_restore_backlight();
	return window;
}

static SDL_Renderer *my282_SDL_CreateRenderer(
	SDL_Window *window, int index, Uint32 flags
) {
	SDL_Renderer *renderer = SDL_CreateRenderer(window, index, flags);
	if (!renderer) return NULL;

	my282_composite = SDL_CreateTexture(
		renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET,
		my282_logical_width, my282_logical_height);
	if (!my282_composite) {
		LOG_error("Could not create A30 composition target: %s\n",
		          SDL_GetError());
		SDL_DestroyRenderer(renderer);
		my282_restore_backlight();
		return NULL;
	}

	SDL_SetRenderTarget(renderer, my282_composite);
	SDL_RenderSetScale(renderer, 1.0f, 1.0f);
	SDL_RenderSetViewport(renderer, &(SDL_Rect){
		0, 0, my282_logical_width, my282_logical_height
	});
	SDL_RenderSetClipRect(renderer, NULL);

	int output_width = 0;
	int output_height = 0;
	SDL_GetRendererOutputSize(renderer, &output_width, &output_height);
	LOG_info("A30 presentation logical=%dx%d window=%dx%d output=%dx%d SDL\n",
	         my282_logical_width, my282_logical_height,
	         my282_window_width, my282_window_height,
	         output_width, output_height);
	return renderer;
}

static GLuint my282_compile_game_shader(GLenum type, const char *source) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	GLint compiled = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (compiled != GL_TRUE) {
		char log[512] = {0};
		glGetShaderInfoLog(shader, sizeof(log) - 1, NULL, log);
		LOG_error("A30 game presentation shader failed: %s\n", log);
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

static int my282_init_game_presentation(void) {
	static const char *vertex_source =
		"attribute vec2 a_position;\n"
		"attribute vec2 a_texcoord;\n"
		"varying vec2 v_texcoord;\n"
		"void main() {\n"
		"  gl_Position = vec4(a_position, 0.0, 1.0);\n"
		"  v_texcoord = a_texcoord;\n"
		"}\n";
	static const char *fragment_source =
		"precision mediump float;\n"
		"uniform sampler2D u_texture;\n"
		"varying vec2 v_texcoord;\n"
		"void main() {\n"
		"  gl_FragColor = texture2D(u_texture, v_texcoord);\n"
		"}\n";

	GLuint vertex = my282_compile_game_shader(
		GL_VERTEX_SHADER, vertex_source);
	GLuint fragment = my282_compile_game_shader(
		GL_FRAGMENT_SHADER, fragment_source);
	if (!vertex || !fragment) {
		if (vertex) glDeleteShader(vertex);
		if (fragment) glDeleteShader(fragment);
		return -1;
	}

	my282_game_present_program = glCreateProgram();
	glAttachShader(my282_game_present_program, vertex);
	glAttachShader(my282_game_present_program, fragment);

	GLint max_attribs = 0;
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_attribs);
	if (max_attribs < 2) {
		LOG_error("A30 GLES2 exposes too few vertex attributes: %d\n",
		          max_attribs);
		return -1;
	}
	my282_game_position_attrib = max_attribs - 2;
	my282_game_texcoord_attrib = max_attribs - 1;
	glBindAttribLocation(
		my282_game_present_program,
		(GLuint)my282_game_position_attrib, "a_position");
	glBindAttribLocation(
		my282_game_present_program,
		(GLuint)my282_game_texcoord_attrib, "a_texcoord");
	glLinkProgram(my282_game_present_program);
	glDeleteShader(vertex);
	glDeleteShader(fragment);

	GLint linked = GL_FALSE;
	glGetProgramiv(
		my282_game_present_program, GL_LINK_STATUS, &linked);
	if (linked != GL_TRUE) {
		char log[512] = {0};
		glGetProgramInfoLog(
			my282_game_present_program, sizeof(log) - 1, NULL, log);
		LOG_error("A30 game presentation program failed: %s\n", log);
		return -1;
	}

	glGenTextures(1, &my282_game_texture);
	glBindTexture(GL_TEXTURE_2D, my282_game_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_RGBA,
		my282_logical_width, my282_logical_height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glGenFramebuffers(1, &my282_game_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, my282_game_fbo);
	glFramebufferTexture2D(
		GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, my282_game_texture, 0);
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		LOG_error("A30 game framebuffer incomplete: 0x%x\n", status);
		return -1;
	}
	glViewport(
		0, 0, my282_logical_width, my282_logical_height);
	LOG_info("A30 game presentation framebuffer=%dx%d GLES2\n",
	         my282_logical_width, my282_logical_height);
	return 0;
}

static SDL_GLContext my282_SDL_GL_CreateContext(SDL_Window *window) {
	/*
	 * The common implementation requests ES3 after creating its window.
	 * Reset the request here because Mali-400 exposes GLES2 only.
	 */
	my282_configure_gles2();
	SDL_GLContext context = SDL_GL_CreateContext(window);
	if (!context) my282_restore_backlight();
	if (context && SDL_GL_SetSwapInterval(1) < 0)
		LOG_warn("A30 could not enable EGL swap interval: %s\n",
		         SDL_GetError());
	if (context && my282_init_game_presentation() < 0)
		LOG_error("Could not initialize A30 game presentation\n");
	return context;
}

static void my282_glBindFramebuffer(GLenum target, GLuint framebuffer) {
	if (target == GL_FRAMEBUFFER && framebuffer == 0 && my282_game_fbo)
		framebuffer = my282_game_fbo;
	glBindFramebuffer(target, framebuffer);
}

static void my282_SDL_GL_SwapWindow(SDL_Window *window) {
	if (!my282_game_fbo || !my282_game_texture ||
	    !my282_game_present_program) {
		SDL_GL_SwapWindow(window);
		return;
	}

	GLint previous_program = 0;
	GLint previous_array_buffer = 0;
	GLint previous_active_texture = 0;
	GLint previous_texture = 0;
	GLint position_enabled = 0;
	GLint texcoord_enabled = 0;
	GLboolean previous_color_mask[4];
	GLboolean blend_enabled = glIsEnabled(GL_BLEND);
	GLboolean cull_enabled = glIsEnabled(GL_CULL_FACE);
	GLboolean depth_enabled = glIsEnabled(GL_DEPTH_TEST);
	GLboolean scissor_enabled = glIsEnabled(GL_SCISSOR_TEST);

	glGetIntegerv(GL_CURRENT_PROGRAM, &previous_program);
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previous_array_buffer);
	glGetIntegerv(GL_ACTIVE_TEXTURE, &previous_active_texture);
	glActiveTexture(GL_TEXTURE0);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture);
	glGetVertexAttribiv(
		(GLuint)my282_game_position_attrib,
		GL_VERTEX_ATTRIB_ARRAY_ENABLED, &position_enabled);
	glGetVertexAttribiv(
		(GLuint)my282_game_texcoord_attrib,
		GL_VERTEX_ATTRIB_ARRAY_ENABLED, &texcoord_enabled);
	glGetBooleanv(GL_COLOR_WRITEMASK, previous_color_mask);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, my282_window_width, my282_window_height);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glUseProgram(my282_game_present_program);
	glBindTexture(GL_TEXTURE_2D, my282_game_texture);
	glUniform1i(
		glGetUniformLocation(
			my282_game_present_program, "u_texture"), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	static const GLfloat positions[] = {
		-1.0f, -1.0f,
		 1.0f, -1.0f,
		-1.0f,  1.0f,
		 1.0f,  1.0f,
	};
	static const GLfloat texcoords_ccw[] = {
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
	};
	glEnableVertexAttribArray((GLuint)my282_game_position_attrib);
	glEnableVertexAttribArray((GLuint)my282_game_texcoord_attrib);
	glVertexAttribPointer(
		(GLuint)my282_game_position_attrib,
		2, GL_FLOAT, GL_FALSE, 0, positions);
	glVertexAttribPointer(
		(GLuint)my282_game_texcoord_attrib,
		2, GL_FLOAT, GL_FALSE, 0, texcoords_ccw);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	my282_wait_for_vsync();
	SDL_GL_SwapWindow(window);
	my282_set_framebuffer_blank(0);
	my282_restore_backlight();

	if (!position_enabled)
		glDisableVertexAttribArray((GLuint)my282_game_position_attrib);
	if (!texcoord_enabled)
		glDisableVertexAttribArray((GLuint)my282_game_texcoord_attrib);
	glUseProgram((GLuint)previous_program);
	glBindBuffer(GL_ARRAY_BUFFER, (GLuint)previous_array_buffer);
	glBindTexture(GL_TEXTURE_2D, (GLuint)previous_texture);
	glActiveTexture((GLenum)previous_active_texture);
	if (blend_enabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
	if (cull_enabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
	if (depth_enabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
	if (scissor_enabled) glEnable(GL_SCISSOR_TEST);
	else glDisable(GL_SCISSOR_TEST);
	glColorMask(
		previous_color_mask[0], previous_color_mask[1],
		previous_color_mask[2], previous_color_mask[3]);
	glBindFramebuffer(GL_FRAMEBUFFER, my282_game_fbo);
	glViewport(
		0, 0, my282_logical_width, my282_logical_height);
}

static void my282_SDL_GL_DeleteContext(SDL_GLContext context) {
	/*
	 * The firmware Mali driver exposes an unrotated buffer while GPU objects
	 * are destroyed. Every caller exits immediately after GFX_quit, so let the
	 * process reclaim these objects atomically instead of visibly dismantling
	 * them one by one.
	 */
	(void)context;
	my282_set_framebuffer_blank(1);
	my282_game_fbo = 0;
	my282_game_texture = 0;
	my282_game_present_program = 0;
}

static int my282_SDL_SetRenderTarget(
	SDL_Renderer *renderer, SDL_Texture *target
) {
	if (!target) target = my282_composite;
	int result = SDL_SetRenderTarget(renderer, target);
	if (result == 0 && target == my282_composite) {
		SDL_RenderSetScale(renderer, 1.0f, 1.0f);
		SDL_RenderSetViewport(renderer, &(SDL_Rect){
			0, 0, my282_logical_width, my282_logical_height
		});
		SDL_RenderSetClipRect(renderer, NULL);
	}
	return result;
}

static void my282_SDL_RenderPresent(SDL_Renderer *renderer) {
	if (!my282_composite) return;

	SDL_SetRenderTarget(renderer, NULL);
	SDL_RenderSetScale(renderer, 1.0f, 1.0f);
	SDL_RenderSetViewport(renderer, &(SDL_Rect){
		0, 0, my282_window_width, my282_window_height
	});
	SDL_RenderSetClipRect(renderer, NULL);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	SDL_Vertex vertices[] = {
		{{0.0f,                      0.0f},                       {255,255,255,255}, {1.0f, 0.0f}},
		{{(float)my282_window_width, 0.0f},                       {255,255,255,255}, {1.0f, 1.0f}},
		{{0.0f,                      (float)my282_window_height}, {255,255,255,255}, {0.0f, 0.0f}},
		{{(float)my282_window_width, (float)my282_window_height}, {255,255,255,255}, {0.0f, 1.0f}},
	};
	static const int indices[] = {0, 1, 2, 1, 3, 2};
	if (SDL_RenderGeometry(
		    renderer, my282_composite, vertices, 4, indices, 6) < 0) {
		LOG_error("Could not present A30 composition target: %s\n",
		          SDL_GetError());
	}
	my282_wait_for_vsync();
	SDL_RenderPresent(renderer);
	my282_set_framebuffer_blank(0);
	my282_restore_backlight();

	my282_SDL_SetRenderTarget(renderer, NULL);
}

static void my282_SDL_DestroyRenderer(SDL_Renderer *renderer) {
	(void)renderer;
	my282_dim_backlight();
	my282_set_framebuffer_blank(1);
	my282_composite = NULL;
}

static void my282_blank_framebuffer(void) {
	int fd = open("/dev/fb0", O_WRONLY | O_CLOEXEC);
	if (fd < 0) return;

	struct fb_fix_screeninfo fixed;
	if (ioctl(fd, FBIOGET_FSCREENINFO, &fixed) == 0) {
		static const unsigned char black[4096] = {0};
		unsigned long remaining = fixed.smem_len;
		lseek(fd, 0, SEEK_SET);
		while (remaining > 0) {
			size_t chunk =
				remaining > sizeof(black) ? sizeof(black) : remaining;
			ssize_t written = write(fd, black, chunk);
			if (written <= 0) break;
			remaining -= (unsigned long)written;
		}
	}
	close(fd);
}

static void my282_SDL_DestroyWindow(SDL_Window *window) {
	/*
	 * Mali-fbdev briefly exposes its unrotated backing buffer while the
	 * window is destroyed. Blank both sides of that transition directly;
	 * the shell-based common fallback arrives one visible frame too late.
	 */
	my282_blank_framebuffer();
	my282_dim_backlight();
	my282_set_framebuffer_blank(1);
	(void)window;
}

static void my282_SDL_QuitSubSystem(Uint32 flags) {
	Uint32 remaining = flags & ~SDL_INIT_VIDEO;
	if (remaining) SDL_QuitSubSystem(remaining);
}

/*
 * Mali-400 exposes GLES2. The shared shader runner uses optional ES3 program
 * binary caching and VAO entry points, neither of which is required for
 * rendering. Disable caching and reduce VAOs to ordinary GLES2 VBO state for
 * this translation unit only.
 */
static void my282_glProgramBinary(
	GLuint program, GLenum binary_format, const void *binary, GLsizei length
) {
	(void)program;
	(void)binary_format;
	(void)binary;
	(void)length;
}

static void my282_glProgramParameteri(
	GLuint program, GLenum pname, GLint value
) {
	(void)program;
	(void)pname;
	(void)value;
}

static void my282_glGetProgramiv(
	GLuint program, GLenum pname, GLint *params
) {
	if (pname == GL_PROGRAM_BINARY_LENGTH) {
		*params = 0;
		return;
	}
	glGetProgramiv(program, pname, params);
}

static void my282_glGetProgramBinary(
	GLuint program, GLsizei buffer_size, GLsizei *length,
	GLenum *binary_format, void *binary
) {
	(void)program;
	(void)buffer_size;
	(void)binary;
	if (length) *length = 0;
	if (binary_format) *binary_format = 0;
}

static void my282_glGenVertexArrays(GLsizei count, GLuint *arrays) {
	for (GLsizei i = 0; i < count; ++i) arrays[i] = (GLuint)(i + 1);
}

static void my282_glBindVertexArray(GLuint array) {
	(void)array;
}

static void my282_glDeleteVertexArrays(GLsizei count, const GLuint *arrays) {
	(void)count;
	(void)arrays;
}

#define SDL_CreateWindow my282_SDL_CreateWindow
#define SDL_CreateRenderer my282_SDL_CreateRenderer
#define SDL_GL_CreateContext my282_SDL_GL_CreateContext
#define SDL_GL_SwapWindow my282_SDL_GL_SwapWindow
#define SDL_GL_DeleteContext my282_SDL_GL_DeleteContext
#define SDL_SetRenderTarget my282_SDL_SetRenderTarget
#define SDL_RenderPresent my282_SDL_RenderPresent
#define SDL_DestroyRenderer my282_SDL_DestroyRenderer
#define SDL_DestroyWindow my282_SDL_DestroyWindow
#define SDL_QuitSubSystem my282_SDL_QuitSubSystem
#define glProgramBinary my282_glProgramBinary
#define glProgramParameteri my282_glProgramParameteri
#define glGetProgramiv my282_glGetProgramiv
#define glGetProgramBinary my282_glGetProgramBinary
#define glGenVertexArrays my282_glGenVertexArrays
#define glBindVertexArray my282_glBindVertexArray
#define glDeleteVertexArrays my282_glDeleteVertexArrays
#define glBindFramebuffer my282_glBindFramebuffer
