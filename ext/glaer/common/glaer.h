
/*** GLAER: begin manually authored code ***/

/* header guard, extern "C" and version done by script */

/*
 * Link Libraries:
 * Windows : opengl32, kernel32
 * Mac OSX : ???
 * Linux   : libGL
 */

#if defined(__gl_h_) || defined(__GL_H__)
#error gl.h included before glaer.h
#endif

/* prevent inclusion of gl.h */
#define __gl_h_
#define __GL_H__

/* specific bitwidth int types */
#include <stdint.h>

/* what was this for again? NULL? */
#include <stdlib.h>

/* calling convention */
#ifndef APIENTRY
#ifdef _WIN32
#define APIENTRY __stdcall
#else
#define APIENTRY
#endif
#endif

/* dll import / export */
#if defined(_WIN32) && !defined(GLAPI)
#if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__POCC__)
/* Microsoft Visual C++, Borland C++ Builder and Pelles C */
#if defined(GLAER_DLL)
/* Using GLAER as a DLL */
#define GLAERAPI __declspec(dllimport)
#elif defined(GLAER_BUILD_DLL)
/* Building GLAER as a DLL */
#define GLAERAPI __declspec(dllexport)
#endif
#endif
#endif

#ifndef GLAERAPI
#define GLAERAPI extern
#endif


/* 
 * Primary OpenGL types
 * https://www.opengl.org/wiki/OpenGL_Type
 */

#ifndef GLAER_TYPE_GL_BOOLEAN
#define GLAER_TYPE_GL_BOOLEAN unsigned char
#endif

/*
 * GLboolean is the only GL type of non-specific bitwidth.
 * If it needs to be defined as something else, define
 * GLAER_TYPE_GL_BOOLEAN to the appropriate (integral) type.
 */
typedef GLAER_TYPE_GL_BOOLEAN GLboolean;

typedef uint32_t GLenum;
typedef uint32_t GLbitfield;
typedef void GLvoid;
typedef signed char GLbyte;
typedef int16_t GLshort;
typedef int32_t GLint;
typedef GLint GLclampx;
typedef unsigned char GLubyte;
typedef uint16_t GLushort;
typedef uint32_t GLuint;
typedef GLint GLsizei;
typedef float GLfloat;
typedef GLfloat GLclampf;
typedef double GLdouble;
typedef GLdouble GLclampd;
typedef void *GLeglImageOES;
typedef char GLchar;
typedef GLchar GLcharARB;
#ifdef __APPLE__
typedef void *GLhandleARB;
#else
typedef GLuint GLhandleARB;
#endif
typedef GLushort GLhalf;
typedef GLhalf GLhalfARB;
typedef GLint GLfixed;
typedef intptr_t GLintptr;
typedef GLintptr GLsizeiptr;
typedef int64_t GLint64;
typedef uint64_t GLuint64;
typedef GLintptr GLintptrARB;
typedef GLsizeiptr GLsizeiptrARB;
typedef GLint64 GLint64EXT;
typedef GLuint64 GLuint64EXT;

struct __GLsync;
typedef struct __GLsync *GLsync;

/* OpenCL compatiblity */
struct _cl_context;
struct _cl_event;

/* callbacks */
typedef void (APIENTRY *GLDEBUGPROC)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);
typedef void (APIENTRY *GLDEBUGPROCARB)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);
typedef void (APIENTRY *GLDEBUGPROCKHR)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);

/* vendor extensions */
typedef void (APIENTRY *GLDEBUGPROCAMD)(GLuint id, GLenum category, GLenum severity, GLsizei length, const GLchar *message, void *userParam);
typedef GLushort GLhalfNV;
typedef GLintptr GLvdpauSurfaceNV;

/* GLAER types */
struct GlaerContext_;
typedef struct GlaerContext_ GlaerContext;
typedef GlaerContext * (*GlaerContextProvider)();
typedef void (*GlaerErrorCallback)(const GLchar *message);
typedef void (APIENTRY *GlaerPFn)();

/*
 * Set the function that will be called to determine the current context.
 * Thread-safety: main thread only.
 */
GLAERAPI void APIENTRY glaerSetCurrentContextProvider(GlaerContextProvider);

/*
 * Set the error callback.
 * Thread-safety: main thread only.
 */
GLAERAPI void APIENTRY glaerSetErrorCallback(GlaerErrorCallback);

/*
 * Get a pointer to the current GLAER context.
 * Wrapper for the user function pointer set by glaerSetContextProvider().
 * Thread-safety: as for the user context provider.
 */
GLAERAPI GlaerContext * APIENTRY glaerGetCurrentContext();

/*
 * Initialize the current GLAER context with function pointers for the current GL context.
 * Returns GL_TRUE on success, GL_FALSE otherwise.
 * Thread-safety: as for glaerGetCurrentContext(). Initialization itself is thread-safe.
 */
GLAERAPI GLboolean APIENTRY glaerInitCurrentContext();

/*
 * Test for the presence of a GL function in the current GLAER context.
 * Evaluates to GL_TRUE if function is available, GL_FALSE otherwise.
 * Expects the GLAER version of the function name; ordinarily the plain
 * GL function names are defined by GLAER such that they are usable here.
 * E.g. if (GLAER_HAVE_FUN(glUniform1d)) { ... }
 * Note that the presence of a function does not imply that it is
 * supported by the associated GL context.
 * Thread-safety: as for glaerGetCurrentContext().
 */
#define GLAER_HAVE_FUN(glaerFun) (glaerGetCurrentContext()->glaerFun != NULL)


/*** GLAER: end manually authored code ***/

