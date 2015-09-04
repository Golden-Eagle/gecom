
#include <cassert>
#include <cstdlib>
#include <map>
#include <sstream>

#include "Window.hpp"

using namespace std;

namespace gecom {

	namespace {

		struct WindowData {
			Window *window;
			GlaerContext context;
			bool init_done = false;
			WindowData(Window *window_) : window(window_) { }
		};

		WindowData * getWindowData(GLFWwindow *handle) {
			return (WindowData *) glfwGetWindowUserPointer(handle);
		}

		Window * getWindow(GLFWwindow *handle) {
			return getWindowData(handle)->window;
		}

		void callbackWindowPos(GLFWwindow *handle, int x, int y) {
			Window *win = getWindow(handle);
			window_pos_event e;
			e.proxy = win;
			e.pos = point2i(x, y);
			e.dispatch(*win);
		}

		void callbackWindowSize(GLFWwindow *handle, int w, int h) {
			Window *win = getWindow(handle);
			window_size_event e;
			e.proxy = win;
			e.size = size2i(w, h);
			e.dispatch(*win);
		}

		void callbackWindowClose(GLFWwindow *handle) {
			Window *win = getWindow(handle);
			window_close_event e;
			e.proxy = win;
			e.dispatch(*win);
		}

		void callbackWindowRefresh(GLFWwindow *handle) {
			Window *win = getWindow(handle);
			window_refresh_event e;
			e.proxy = win;
			e.dispatch(*win);
		}

		void callbackWindowFocus(GLFWwindow *handle, int focused) {
			Window *win = getWindow(handle);
			window_focus_event e;
			e.proxy = win;
			e.focused = focused;
			e.dispatch(*win);
		}

		void callbackWindowIconify(GLFWwindow *handle, int iconified) {
			Window *win = getWindow(handle);
			window_icon_event e;
			e.proxy = win;
			e.iconified = iconified;
			e.dispatch(*win);
		}

		void callbackFramebufferSize(GLFWwindow *handle, int w, int h) {
			Window *win = getWindow(handle);
			framebuffer_size_event e;
			e.proxy = win;
			e.size.w = w;
			e.size.h = h;
			e.dispatch(*win);
		}

		void callbackMouseButton(GLFWwindow *handle, int button, int action, int mods) {
			Window *win = getWindow(handle);
			mouse_button_event e;
			e.proxy = win;
			e.button = button;
			e.action = action;
			e.mods = mods;
			e.entered = false;
			e.exited = false;
			glfwGetCursorPos(handle, &e.pos.x, &e.pos.y);
			e.dispatch(*win);
		}

		void callbackCursorPos(GLFWwindow *handle, double x, double y) {
			Window *win = getWindow(handle);
			mouse_event e;
			e.proxy = win;
			e.pos = point2d(x, y);
			e.entered = false;
			e.exited = false;
			e.dispatch(*win);
		}

		void callbackCursorEnter(GLFWwindow *handle, int entered) {
			Window *win = getWindow(handle);
			mouse_event e;
			e.proxy = win;
			glfwGetCursorPos(handle, &e.pos.x, &e.pos.y);
			e.entered = entered;
			e.exited = !entered;
			e.dispatch(*win);
		}

		void callbackScroll(GLFWwindow *handle, double xoffset, double yoffset) {
			Window *win = getWindow(handle);
			mouse_scroll_event e;
			e.proxy = win;
			glfwGetCursorPos(handle, &e.pos.x, &e.pos.y);
			e.entered = false;
			e.exited = false;
			e.offset = size2d(xoffset, yoffset);
			e.dispatch(*win);
		}

		void callbackKey(GLFWwindow *handle, int key, int scancode, int action, int mods) {
			Window *win = getWindow(handle);
			key_event e;
			e.proxy = win;
			e.key = key;
			e.scancode = scancode;
			e.action = action;
			e.mods = mods;
			e.dispatch(*win);
		}

		void callbackChar(GLFWwindow *handle, unsigned codepoint) {
			Window *win = getWindow(handle);
			char_event e;
			e.proxy = win;
			e.codepoint = codepoint;
			e.dispatch(*win);
		}

		void callbackErrorGLFW(int error, const char *description) {
			Log::error("GLFW") << "Error " << error << " : " << description;
		}

		void callbackErrorGLAER(const char *msg) {
			Log::error("GLAER") << "Error: " << msg;
		}

		// init GLFW global state.
		// this should only be called from the main thread.
		// does nothing if already initialised.
		void init_glfw() {
			static bool done = false;
			if (!done) {
				Log::info() << "GLFW initializing...";
				// this is safe to call before glfwInit()
				glfwSetErrorCallback(callbackErrorGLFW);
				if (!glfwInit()) {
					Log::critical() << "GLFW initialization failed";
					// screw catching this, ever
					std::abort();
				}
				// set GLAER callbacks here too, but don't init anything yet
				glaerSetErrorCallback(callbackErrorGLAER);
				glaerSetCurrentContextProvider(getCurrentGlaerContext);
				Log::info().verbosity(0) << "GLFW initialized";
				done = true;
			}
		}

		// GL callback for debug information
		// gl.xml and glDebugMessageControl.xml disagree on whether userParam should point to const or not
		void APIENTRY callbackDebugGL(
			GLenum source,
			GLenum type,
			GLuint id,
			GLenum severity,
			GLsizei length,
			const GLchar *message,
			const void *userParam
		){
			// enum documentation:
			// https://www.opengl.org/sdk/docs/man4/html/glDebugMessageControl.xhtml
			
			(void) length;
			(void) userParam;

			bool exceptional = false;

			// message source within GL -> log source
			string log_source = "GL";
			switch (source) {
			case GL_DEBUG_SOURCE_API:
				log_source = "GL:API";
				break;
			case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
				log_source = "GL:Window";
				break;
			case GL_DEBUG_SOURCE_SHADER_COMPILER:
				log_source = "GL:Shader";
				break;
			case GL_DEBUG_SOURCE_THIRD_PARTY:
				log_source = "GL:ThirdParty";
				break;
			case GL_DEBUG_SOURCE_APPLICATION:
				log_source = "GL:App";
				break;
			case GL_DEBUG_SOURCE_OTHER:
				log_source = "GL:Other";
				break;
			default:
				break;
			}

			{
				// piecewise construct log message
				// new scope so it submits before we throw, if we throw
				auto logs = Log::info(log_source);

				// message type -> log type
				switch (type) {
				case GL_DEBUG_TYPE_ERROR:
					logs.error();
					logs << "Error";
					exceptional = true;
					break;
				case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
					logs.warning();
					logs << "Deprecated Behaviour";
					break;
				case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
					logs.warning();
					logs << "Undefined Behaviour";
					break;
				case GL_DEBUG_TYPE_PORTABILITY:
					logs.warning();
					logs << "Portability";
					break;
				case GL_DEBUG_TYPE_PERFORMANCE:
					logs.warning();
					logs << "Performance";
					break;
				case GL_DEBUG_TYPE_MARKER:
					logs << "Marker";
					break;
				case GL_DEBUG_TYPE_PUSH_GROUP:
					logs << "Push Group";
					break;
				case GL_DEBUG_TYPE_POP_GROUP:
					logs << "Pop Group";
					break;
				case GL_DEBUG_TYPE_OTHER:
					logs << "Other";
					break;
				}

				// severity -> log verbosity
				switch (severity) {
				case GL_DEBUG_SEVERITY_NOTIFICATION:
					logs.verbosity(3);
					break;
				case GL_DEBUG_SEVERITY_LOW:
					logs.verbosity(2);
					break;
				case GL_DEBUG_SEVERITY_MEDIUM:
					logs.verbosity(1);
					break;
				case GL_DEBUG_SEVERITY_HIGH:
					logs.verbosity(0);
					break;
				}

				// actual message. id = as returned by glGetError(), in some cases
				logs << " [" << id << "] : " << message;

			}

#ifndef GECOM_GL_NO_EXCEPTIONS
			if (exceptional) {
				throw gl_error(); //oss.str());
			}
#endif

		}

	}

	void WindowEventProxy::dispatchWindowRefreshEvent(const window_refresh_event &e) {
		window_refresh_event e2(e);
		e2.proxy = this;
		onRefresh.notify(e2);
		onEvent.notify(e2);
	}

	void WindowEventProxy::dispatchWindowCloseEvent(const window_close_event &e) {
		window_close_event e2(e);
		e2.proxy = this;
		onClose.notify(e2);
		onEvent.notify(e2);
	}

	void WindowEventProxy::dispatchWindowPosEvent(const window_pos_event &e) {
		onEvent.notify(e);
		onMove.notify(e);
	}

	void WindowEventProxy::dispatchWindowSizeEvent(const window_size_event &e) {
		window_size_event e2(e);
		e2.proxy = this;
		onResize.notify(e2);
		onEvent.notify(e2);
	}

	void WindowEventProxy::dispatchFramebufferSizeEvent(const framebuffer_size_event &e) {
		framebuffer_size_event e2(e);
		e2.proxy = this;
		onFramebufferResize.notify(e2);
		onEvent.notify(e2);
	}

	void WindowEventProxy::dispatchWindowFocusEvent(const window_focus_event &e) {
		window_focus_event e2(e);
		e2.proxy = this;
		if (e2.focused) {
			onFocusGain.notify(e2);
		} else {
			// lost focus, release all keys and mouse buttons
			m_keystates.reset();
			m_mbstates.reset();
			onFocusLose.notify(e2);
		}
		onFocus.notify(e2);
		onEvent.notify(e2);
	}

	void WindowEventProxy::dispatchWindowIconEvent(const window_icon_event &e) {
		window_icon_event e2(e);
		e2.proxy = this;
		if (e2.iconified) {
			onIconMinimize.notify(e2);
		} else {
			onIconRestore.notify(e2);
		}
		onIcon.notify(e2);
		onEvent.notify(e2);
	}

	void WindowEventProxy::dispatchMouseEvent(const mouse_event &e) {
		mouse_event e2(e);
		e2.proxy = this;
		m_mpos = e2.pos;
		if (e.entered) onMouseEnter.notify(e2);
		if (e.exited) onMouseExit.notify(e2);
		onMouseMove.notify(e2);
		onEvent.notify(e2);
	}

	void WindowEventProxy::dispatchMouseButtonEvent(const mouse_button_event &e) {
		mouse_button_event e2(e);
		e2.proxy = this;
		m_mpos = e2.pos;
		// i dont think mouse buttons get repeats, but whatever
		if (e2.action == GLFW_PRESS || e2.action == GLFW_REPEAT) {
			m_mbstates.set(e2.button);
			onMouseButtonPress.notify(e2);
		} else if (e2.action == GLFW_RELEASE) {
			m_mbstates.reset(e2.button);
			onMouseButtonRelease.notify(e2);
		}
		onMouseButton.notify(e2);
		onEvent.notify(e2);
	}

	void WindowEventProxy::dispatchMouseScrollEvent(const mouse_scroll_event &e) {
		mouse_scroll_event e2(e);
		e2.proxy = this;
		m_mpos = e2.pos;
		onMouseScroll.notify(e2);
		onEvent.notify(e2);
	}

	void WindowEventProxy::dispatchKeyEvent(const key_event &e) {
		key_event e2(e);
		e2.proxy = this;
		if (e2.action == GLFW_PRESS || e2.action == GLFW_REPEAT) {
			m_keystates.set(e2.key);
			onKeyPress.notify(e2);
		} else if (e2.action == GLFW_RELEASE) {
			m_keystates.reset(e2.key);
			onKeyRelease.notify(e2);
		}
		onKey.notify(e2);
		onEvent.notify(e2);
	}

	void WindowEventProxy::dispatchCharEvent(const char_event &e) {
		char_event e2(e);
		e2.proxy = this;
		onChar.notify(e2);
		onEvent.notify(e2);
	}

	GlaerContext * getCurrentGlaerContext() {
		GLFWwindow *handle = glfwGetCurrentContext();
		if (!handle) {
			throw window_error("no current context");
		}
		return &(getWindowData(handle)->context);
	}

	void Window::initialize() {
		// set ALL the callbacks
		glfwSetWindowPosCallback(m_handle, callbackWindowPos);
		glfwSetWindowSizeCallback(m_handle, callbackWindowSize);
		glfwSetWindowCloseCallback(m_handle, callbackWindowClose);
		glfwSetWindowRefreshCallback(m_handle, callbackWindowRefresh);
		glfwSetWindowFocusCallback(m_handle, callbackWindowFocus);
		glfwSetWindowIconifyCallback(m_handle, callbackWindowIconify);
		glfwSetFramebufferSizeCallback(m_handle, callbackFramebufferSize);
		glfwSetMouseButtonCallback(m_handle, callbackMouseButton);
		glfwSetCursorPosCallback(m_handle, callbackCursorPos);
		glfwSetCursorEnterCallback(m_handle, callbackCursorEnter);
		glfwSetScrollCallback(m_handle, callbackScroll);
		glfwSetKeyCallback(m_handle, callbackKey);
		glfwSetCharCallback(m_handle, callbackChar);
		// create a windowdata object
		glfwSetWindowUserPointer(m_handle, new WindowData(this));
	}

	void Window::destroy() {
		// must only be called from the main thread
		delete getWindowData(m_handle);
		glfwDestroyWindow(m_handle);
	}

	void Window::makeCurrent() {
		section_guard sec("Window");
		glfwMakeContextCurrent(m_handle);
		WindowData *wd = getWindowData(m_handle);
		// init glaer
		if (!wd->init_done) {
			Log::info() << "GLAER initializing...";
			//GLenum t_err = glGetError();
			//gecom::log("Window") << "GLEW t_err: " << t_err;
			if (!glaerInitCurrentContext()) {
				Log::error() << "GLAER initialization failed";
				glfwTerminate();
				std::abort();
			}
			// clear any GL errors from init
			GLenum gl_err = glGetError();
			while (gl_err) {
				Log::info() << "GLAER initialization left GL error " << gl_err;
				gl_err = glGetError();
			}
			Log::info() << "GL_VENDOR: " << glGetString(GL_VENDOR);
			Log::info() << "GL_RENDERER " << glGetString(GL_RENDERER);
			Log::info() << "GL_VERSION: " << glGetString(GL_VERSION);
			Log::info() << "GL_SHADING_LANGUAGE_VERSION: " << glGetString(GL_SHADING_LANGUAGE_VERSION);
			Log::info().verbosity(0) << "GLAER initialized";
			wd->init_done = true;
			// enable GL_ARB_debug_output if available
			if (glfwExtensionSupported("GL_ARB_debug_output")) {
				// this allows the error location to be determined from a stacktrace
				glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
				// set the callback
				glDebugMessageCallbackARB(callbackDebugGL, this);
				glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, true);
				Log::info() << "GL debug callback installed";
			} else {
				Log::info() << "GL_ARB_debug_output not available";
			}
		}
	}
	
	Window * Window::current() {
		GLFWwindow *handle = glfwGetCurrentContext();
		if (handle == nullptr) return nullptr;
		return getWindow(handle);
	}

	// this should only be called from the main thread
	create_window_args::operator Window * () {
		section_guard sec("Window");
		Log::info().verbosity(0) << "Creating window... [title=" << m_title << "]";
		if (m_hints[GLFW_OPENGL_DEBUG_CONTEXT]) {
			Log::info().verbosity(0) << "Requesting debug GL context";
		}
		init_glfw();
		glfwDefaultWindowHints();
		//GLenum gl_err = glGetError();
		//gecom::log("Window") % 0 << "GLerror: " << gl_err;
		for (auto me : m_hints) {
			glfwWindowHint(me.first, me.second);
		}
		GLFWwindow *handle = glfwCreateWindow(m_size.w, m_size.h, m_title.c_str(), m_monitor, m_share ? m_share->handle() : nullptr);
		glfwDefaultWindowHints();
		if (!handle) {
			Log::error() << "GLFW window creation failed";
			throw window_error("GLFW window creation failed");
		}
		Log::info().verbosity(0) << "Window created [title=" << m_title << "]";
		return new Window(handle, m_share);
	}


}
