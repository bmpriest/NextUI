#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <SDL2/SDL.h>

static void print_file(const char *label, const char *path) {
    char value[256] = {0};
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        printf("%s: unavailable (%s)\n", label, strerror(errno));
        return;
    }
    ssize_t n = read(fd, value, sizeof(value) - 1);
    close(fd);
    if (n < 0) {
        printf("%s: read failed (%s)\n", label, strerror(errno));
        return;
    }
    value[n] = '\0';
    value[strcspn(value, "\r\n")] = '\0';
    printf("%s: %s\n", label, value);
}

static void print_framebuffer(void) {
    int fd = open("/dev/fb0", O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        printf("fb0: unavailable (%s)\n", strerror(errno));
        return;
    }

    struct fb_var_screeninfo var = {0};
    struct fb_fix_screeninfo fix = {0};
    if (ioctl(fd, FBIOGET_VSCREENINFO, &var) == 0 &&
        ioctl(fd, FBIOGET_FSCREENINFO, &fix) == 0) {
        printf("fb0: %ux%u virtual=%ux%u bpp=%u stride=%u id=%.16s\n",
               var.xres, var.yres, var.xres_virtual, var.yres_virtual,
               var.bits_per_pixel, fix.line_length, fix.id);
    } else {
        printf("fb0: ioctl failed (%s)\n", strerror(errno));
    }
    close(fd);
}

static void print_input_devices(void) {
    DIR *dir = opendir("/sys/class/input");
    if (!dir) {
        printf("input: unavailable (%s)\n", strerror(errno));
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (strncmp(entry->d_name, "event", 5) != 0) continue;
        char path[512];
        snprintf(path, sizeof(path), "/sys/class/input/%s/device/name",
                 entry->d_name);
        char label[sizeof(entry->d_name) + sizeof("input ")];
        snprintf(label, sizeof(label), "input %s", entry->d_name);
        print_file(label, path);
    }
    closedir(dir);
}

static void print_sdl_drivers(void) {
    SDL_version compiled;
    SDL_version linked;
    SDL_VERSION(&compiled);
    SDL_GetVersion(&linked);
    printf("SDL compiled: %u.%u.%u\n", compiled.major, compiled.minor,
           compiled.patch);
    printf("SDL linked: %u.%u.%u\n", linked.major, linked.minor,
           linked.patch);

    for (int i = 0; i < SDL_GetNumVideoDrivers(); ++i)
        printf("SDL video driver[%d]: %s\n", i, SDL_GetVideoDriver(i));
    for (int i = 0; i < SDL_GetNumAudioDrivers(); ++i)
        printf("SDL audio driver[%d]: %s\n", i, SDL_GetAudioDriver(i));
}

static void draw_quadrants(int width, int height) {
    // OpenGL's origin is bottom-left. A correctly oriented landscape display
    // shows red/green on top and blue/yellow on the bottom.
    glScissor(0, height / 2, width / 2, height - height / 2);
    glClearColor(0.85f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glScissor(width / 2, height / 2, width - width / 2,
              height - height / 2);
    glClearColor(0.05f, 0.75f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glScissor(0, 0, width / 2, height / 2);
    glClearColor(0.05f, 0.20f, 0.85f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glScissor(width / 2, 0, width - width / 2, height / 2);
    glClearColor(0.90f, 0.80f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

static GLuint compile_shader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        char log[512] = {0};
        glGetShaderInfoLog(shader, sizeof(log) - 1, NULL, log);
        printf("GL shader compile failed: %s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint create_present_program(void) {
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

    GLuint vertex = compile_shader(GL_VERTEX_SHADER, vertex_source);
    GLuint fragment = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
    if (!vertex || !fragment) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glBindAttribLocation(program, 0, "a_position");
    glBindAttribLocation(program, 1, "a_texcoord");
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        char log[512] = {0};
        glGetProgramInfoLog(program, sizeof(log) - 1, NULL, log);
        printf("GL program link failed: %s\n", log);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

static int probe_video(void) {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    printf("SDL current video driver: %s\n",
           SDL_GetCurrentVideoDriver() ?: "(none)");
    SDL_Window *window = SDL_CreateWindow(
        "NextUI my282 probe", SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, 640, 480,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!window) {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        printf("SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    const GLubyte *gl_vendor = glGetString(GL_VENDOR);
    const GLubyte *gl_renderer = glGetString(GL_RENDERER);
    const GLubyte *gl_version = glGetString(GL_VERSION);
    const GLubyte *glsl_version = glGetString(GL_SHADING_LANGUAGE_VERSION);
    const char *egl_version =
        eglQueryString(eglGetCurrentDisplay(), EGL_VERSION);
    printf("GL vendor: %s\n", gl_vendor ? (const char *)gl_vendor : "(none)");
    printf("GL renderer: %s\n",
           gl_renderer ? (const char *)gl_renderer : "(none)");
    printf("GL version: %s\n",
           gl_version ? (const char *)gl_version : "(none)");
    printf("GLSL version: %s\n",
           glsl_version ? (const char *)glsl_version : "(none)");
    printf("EGL version: %s\n", egl_version ? egl_version : "(none)");

    int drawable_width = 0;
    int drawable_height = 0;
    SDL_GL_GetDrawableSize(window, &drawable_width, &drawable_height);
    printf("SDL drawable: %dx%d\n", drawable_width, drawable_height);

    GLint max_texture = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture);
    printf("GL max texture: %d\n", max_texture);

    GLuint texture = 0;
    GLuint framebuffer = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 640, 480, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, NULL);
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, texture, 0);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    printf("GL RGBA8 640x480 FBO: 0x%04x (%s)\n", status,
           status == GL_FRAMEBUFFER_COMPLETE ? "complete" : "incomplete");

    GLuint program = create_present_program();
    if (!program || status != GL_FRAMEBUFFER_COMPLETE) {
        glDeleteProgram(program);
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &texture);
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Draw NextUI's logical landscape frame.
    glViewport(0, 0, 640, 480);
    glEnable(GL_SCISSOR_TEST);
    draw_quadrants(640, 480);
    glDisable(GL_SCISSOR_TEST);

    // Pre-rotate the logical frame counter-clockwise. The A30's portrait
    // framebuffer is mounted so mali-fbdev presents it clockwise on screen.
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

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, drawable_width, drawable_height);
    glUseProgram(program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(program, "u_texture"), 0);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, positions);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, texcoords_ccw);
    printf("GL presentation: logical 640x480, rotate 90 degrees CCW\n");

    Uint32 end_time = SDL_GetTicks() + 10000;
    while ((Sint32)(end_time - SDL_GetTicks()) > 0) {
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        SDL_GL_SwapWindow(window);
        SDL_Delay(16);
    }

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDeleteProgram(program);
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &texture);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

int main(void) {
    printf("NextUI Miyoo A30 hardware probe\n");
    print_file("firmware", "/usr/miyoo/version");
    print_framebuffer();
    print_input_devices();
    print_file("battery capacity",
               "/sys/class/power_supply/battery/capacity");
    print_file("battery status", "/sys/class/power_supply/battery/status");
    print_file("AC online", "/sys/class/power_supply/ac/online");
    print_file("brightness",
               "/sys/devices/virtual/disp/disp/attr/lcdbl");
    print_sdl_drivers();
    return probe_video();
}
