/*
 *
 * GECom Window Management Header
 *
 */

// define GECOM_GL_NO_EXCEPTIONS globally
// to prevent GL debug callbacks from throwing exceptions

#ifndef GECOM_WINDOW_HPP
#define GECOM_WINDOW_HPP

#include <string>
#include <stdexcept>
#include <map>
#include <memory>
#include <utility>

#include "GL.hpp"
#include "Shader.hpp"
#include "Log.hpp"
#include "Concurrent.hpp"

namespace gecom {

	inline void checkGL() {
		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			gecom::log("GL").error() << "GL error: " << err;
			throw std::runtime_error("BOOM!");
		}
	}
	
	inline void checkFB() {
		GLenum err = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
		if (err != GL_FRAMEBUFFER_COMPLETE) {
			gecom::log("GL").error() << "Framebuffer status: " << err;
			gecom::log("GL").error() << "YOU BROKE THE FRAMEBUFFER!";
			throw std::runtime_error("OH NOES! THE FRAMEBUFFER IS BROKED");
		}
	}

	inline void checkExtension(const std::string &ext_name) {
		if (glfwExtensionSupported(ext_name.c_str())) {
			gecom::log("GL") << "Extension " << ext_name << " detected.";
		} else {
			gecom::log("GL").error() << "Extension " << ext_name << " not supported.";
			throw std::runtime_error("unsupported extension");
		}
	}

	//
	// Point2
	//
	template <typename T>
	struct point2 {
		T x, y;
		point2(T x_, T y_) : x(x_), y(y_) { }
		point2(std::pair<T, T> p) : x(p.first), y(p.second) { }
		point2() : x(0), y(0) { }

		const T & operator[](int i) const {
			return *(&x + i);
		}

		T & operator[](int i) {
			return *(&x + i);
		}

		operator std::pair<T, T>() const {
			return std::make_pair(x, y);
		}
	};

	using point2i = point2<int>;
	using point2f = point2<float>;
	using point2d = point2<double>;

	//
	// Size2
	//
	template <typename T>
	struct size2 {
		T w, h;
		size2(T w_, T h_) : w(w_), h(h_) { }
		size2(std::pair<T, T> p) : w(p.first), h(p.second) { }
		size2() : w(0), h(0) { }

		inline double ratio() const {
			return double(w) / double(h);
		}

		const T & operator[](int i) const {
			return *(&w + i);
		}

		T & operator[](int i) {
			return *(&h + i);
		}

		operator std::pair<T, T>() const {
			return std::make_pair(w, h);
		}
	};

	using size2i = size2<int>;
	using size2f = size2<float>;
	using size2d = size2<double>;

	template <typename T>
	inline bool operator==(const size2<T> &lhs, const size2<T> &rhs) {
		return lhs.w == rhs.w && lhs.h == rhs.h;
	}

	template <typename T>
	inline bool operator!=(const size2<T> &lhs, const size2<T> &rhs) {
		return !(lhs == rhs);
	}

	template <typename T>
	inline bool operator==(const point2<T> &lhs, const point2<T> &rhs) {
		return lhs.x == rhs.x && lhs.y == rhs.y;
	}

	template <typename T>
	inline bool operator!=(const point2<T> &lhs, const point2<T> &rhs) {
		return !(lhs == rhs);
	}

	template <typename T>
	inline point2<T> operator*(const point2<T> &lhs, const T &rhs) {
		point2<T> r;
		r.x = lhs.x * rhs;
		r.y = lhs.y * rhs;
		return r;
	}

	template <typename T>
	inline point2<T> operator/(const point2<T> &lhs, const T &rhs) {
		point2<T> r;
		r.x = lhs.x / rhs;
		r.y = lhs.y / rhs;
		return r;
	}

	template <typename T>
	inline size2<T> operator*(const size2<T> &lhs, const T &rhs) {
		size2<T> r;
		r.w = lhs.w * rhs;
		r.h = lhs.h * rhs;
		return r;
	}

	template <typename T>
	inline size2<T> operator/(const size2<T> &lhs, const T &rhs) {
		size2<T> r;
		r.w = lhs.w / rhs;
		r.h = lhs.h / rhs;
		return r;
	}

	template <typename T>
	inline size2<T> operator*(const size2<T> &lhs, const size2<T> &rhs) {
		size2<T> r;
		r.w = lhs.w * rhs.w;
		r.h = lhs.h * rhs.h;
		return r;
	}

	template <typename T>
	inline size2<T> operator/(const size2<T> &lhs, const size2<T> &rhs) {
		size2<T> r;
		r.w = lhs.w / rhs.w;
		r.h = lhs.h / rhs.h;
		return r;
	}

	template <typename T>
	inline point2<T> operator+(const point2<T> &lhs, const size2<T> &rhs) {
		point2<T> r;
		r.x = lhs.x + rhs.w;
		r.y = lhs.y + rhs.h;
		return r;
	}

	template <typename T>
	inline point2<T> operator-(const point2<T> &lhs, const size2<T> &rhs) {
		point2<T> r;
		r.x = lhs.x - rhs.w;
		r.y = lhs.y - rhs.h;
		return r;
	}

	template <typename T>
	inline size2<T> operator-(const point2<T> &lhs, const point2<T> &rhs) {
		size2<T> r;
		r.w = lhs.x - rhs.x;
		r.h = lhs.y - rhs.y;
		return r;
	}

	template <typename T>
	inline size2<T> operator+(const size2<T> &lhs, const size2<T> &rhs) {
		size2<T> r;
		r.w = lhs.w + rhs.w;
		r.h = lhs.h + rhs.h;
		return r;
	}

	template <typename T>
	inline size2<T> operator-(const size2<T> &lhs, const size2<T> &rhs) {
		size2<T> r;
		r.w = lhs.w - rhs.w;
		r.h = lhs.h - rhs.h;
		return r;
	}

	// window forward declaration
	class Window;

	// window event forward declarations
	struct window_event;
	struct window_refresh_event;
	struct window_close_event;
	struct window_pos_event;
	struct window_size_event; 
	struct window_focus_event;
	struct window_icon_event;
	struct mouse_event;
	struct mouse_button_event;
	struct mouse_scroll_event;
	struct key_event;
	struct char_event;

	// virtual event dispatch
	class WindowEventDispatcher {
	public:
		virtual void dispatchWindowRefreshEvent(const window_refresh_event &) { }
		virtual void dispatchWindowCloseEvent(const window_close_event &) { }
		virtual void dispatchWindowPosEvent(const window_pos_event &) { }
		virtual void dispatchWindowSizeEvent(const window_size_event &) { }
		virtual void dispatchWindowFocusEvent(const window_focus_event &) { }
		virtual void dispatchWindowIconEvent(const window_icon_event &) { }
		virtual void dispatchMouseEvent(const mouse_event &) { }
		virtual void dispatchMouseButtonEvent(const mouse_button_event &) { }
		virtual void dispatchMouseScrollEvent(const mouse_scroll_event &) { }
		virtual void dispatchKeyEvent(const key_event &) { }
		virtual void dispatchCharEvent(const char_event &) { }
		virtual ~WindowEventDispatcher() { }
	};

	// base window event
	struct window_event {
		Window *window = nullptr;

		// dispatch to virtual event dispatcher
		virtual void dispatch(WindowEventDispatcher &wed) const = 0;

		// dispatch to origin window
		void dispatchOrigin() const;

		virtual ~window_event() { }
	};

	// ???
	struct window_refresh_event : public window_event {
		virtual void dispatch(WindowEventDispatcher &wed) const override {
			wed.dispatchWindowRefreshEvent(*this);
		}
	};

	// window about to be closed
	struct window_close_event : public window_event {
		virtual void dispatch(WindowEventDispatcher &wed) const override {
			wed.dispatchWindowCloseEvent(*this);
		}
	};

	// window position changed
	struct window_pos_event : public window_event {
		point2i pos;

		virtual void dispatch(WindowEventDispatcher &wed) const override {
			wed.dispatchWindowPosEvent(*this);
		}
	};

	// window size changed
	struct window_size_event : public window_event {
		size2i size;

		virtual void dispatch(WindowEventDispatcher &wed) const override {
			wed.dispatchWindowSizeEvent(*this);
		}
	};

	// window focused / unfocused
	struct window_focus_event : public window_event {
		bool focused;

		virtual void dispatch(WindowEventDispatcher &wed) const override {
			wed.dispatchWindowFocusEvent(*this);
		}
	};

	// window iconified (minimized) / deiconified (restored)
	struct window_icon_event : public window_event {
		bool iconified;

		virtual void dispatch(WindowEventDispatcher &wed) const override {
			wed.dispatchWindowIconEvent(*this);
		}
	};

	// mouse moved
	struct mouse_event : public window_event {
		point2d pos;
		bool entered;
		bool exited;

		virtual void dispatch(WindowEventDispatcher &wed) const override {
			wed.dispatchMouseEvent(*this);
		}
	};

	// mouse button pressed / released
	struct mouse_button_event : public mouse_event {
		int button;
		int action;
		int mods;

		virtual void dispatch(WindowEventDispatcher &wed) const override {
			wed.dispatchMouseButtonEvent(*this);
		}
	};

	// mouse (wheel) scrolled
	struct mouse_scroll_event : public mouse_event {
		size2d offset;

		virtual void dispatch(WindowEventDispatcher &wed) const override {
			wed.dispatchMouseScrollEvent(*this);
		}
	};

	// key pressed / released
	struct key_event : public window_event {
		int key;
		int scancode;
		int action;
		int mods;

		virtual void dispatch(WindowEventDispatcher &wed) const override {
			wed.dispatchKeyEvent(*this);
		}
	};

	// unicode character typed
	struct char_event : public window_event {
		unsigned codepoint;

		virtual void dispatch(WindowEventDispatcher &wed) const override {
			wed.dispatchCharEvent(*this);
		}
	};

	// handles dispatched events and forwards them to subscribers
	class WindowEventProxy : public WindowEventDispatcher, private Uncopyable {
	public:
		Event<window_event> onEvent;
		Event<window_pos_event> onMove;
		Event<window_size_event> onResize;
		Event<window_refresh_event> onRefresh;
		Event<window_close_event> onClose;
		Event<window_focus_event> onFocus;
		Event<window_focus_event> onFocusGain;
		Event<window_focus_event> onFocusLose;
		Event<window_icon_event> onIcon;
		Event<window_icon_event> onMinimize;
		Event<window_icon_event> onRestore;
		Event<mouse_button_event> onMouseButton;
		Event<mouse_button_event> onMouseButtonPress;
		Event<mouse_button_event> onMouseButtonRelease;
		Event<mouse_event> onMouseMove;
		Event<mouse_event> onMouseEnter;
		Event<mouse_event> onMouseExit;
		Event<mouse_scroll_event> onMouseScroll;
		Event<key_event> onKey;
		Event<key_event> onKeyPress;
		Event<key_event> onKeyRelease;
		Event<char_event> onChar;

		// helper method; subscribes an event dispatcher to onEvent
		subscription subscribeEventDispatcher(std::shared_ptr<WindowEventDispatcher> proxy) {
			return onEvent.subscribe([=](const window_event &e) {
				e.dispatch(*proxy);
				return false;
			});
		}

		virtual void dispatchWindowRefreshEvent(const window_refresh_event &) override;
		virtual void dispatchWindowCloseEvent(const window_close_event &) override;
		virtual void dispatchWindowPosEvent(const window_pos_event &) override;
		virtual void dispatchWindowSizeEvent(const window_size_event &) override;
		virtual void dispatchWindowFocusEvent(const window_focus_event &) override;
		virtual void dispatchWindowIconEvent(const window_icon_event &) override;
		virtual void dispatchMouseEvent(const mouse_event &) override;
		virtual void dispatchMouseButtonEvent(const mouse_button_event &) override;
		virtual void dispatchMouseScrollEvent(const mouse_scroll_event &) override;
		virtual void dispatchKeyEvent(const key_event &) override;
		virtual void dispatchCharEvent(const char_event &) override;
	};


	class window_error : public std::runtime_error {
	public:
		explicit window_error(const std::string &what_ = "Window Error") : runtime_error(what_) { }
	};

	// Thin wrapper around GLFW windowing.
	// Each window can only be used on one thread at once.
	class Window : public WindowEventProxy {
	private:
		// the wrapped window
		GLFWwindow* m_handle;

		// shader manager, shared (potentially) with other windows
		std::shared_ptr<ShaderManager> m_shaderman;

		void initialize();
		void destroy();

	public:
		// ctor: takes ownership of a GLFW window handle
		Window(GLFWwindow *handle_, const Window *share = nullptr) : m_handle(handle_) {
			if (m_handle == nullptr) throw window_error("GLFW window handle is null");
			if (share) {
				m_shaderman = share->shaderManager();
			} else {
				m_shaderman = std::make_shared<ShaderManager>(".");
			}
			initialize();
		}
		
		GLFWwindow * handle() const {
			return m_handle;
		}

		void pos(int x, int y) {
			glfwSetWindowPos(m_handle, x, y);
		}

		void pos(const point2i &p) {
			glfwSetWindowPos(m_handle, p.x, p.y);
		}

		point2i pos() const {
			point2i p;
			glfwGetWindowPos(m_handle, &p.x, &p.y);
			return p;
		}

		void size(int w, int h) {
			glfwSetWindowSize(m_handle, w, h);
		}

		void size(const size2i &s) {
			glfwSetWindowSize(m_handle, s.w, s.h);
		}

		size2i size() const {
			size2i s;
			glfwGetWindowSize(m_handle, &s.w, &s.h);
			return s;
		}

		void width(int w) {
			size2i s = size();
			s.w = w;
			size(s);
		}

		int width() const {
			int w, h;
			glfwGetWindowSize(m_handle, &w, &h);
			return w;
		}

		void height(int h) {
			size2i s = size();
			s.h = h;
			size(s);
		}

		int height() const {
			int w, h;
			glfwGetWindowSize(m_handle, &w, &h);
			return h;
		}

		void title(const std::string &s) {
			glfwSetWindowTitle(m_handle, s.c_str());
		}

		void visible(bool b) {
			if (b) {
				glfwShowWindow(m_handle);
			} else {
				glfwHideWindow(m_handle);
			}
		}

		bool shouldClose() const {
			return glfwWindowShouldClose(m_handle);
		}

		void makeContextCurrent();

		void swapBuffers() const {
			glfwSwapBuffers(m_handle);
		}

		int attrib(int a) const {
			return glfwGetWindowAttrib(m_handle, a);
		}

		// the returned shader manager has the process's cwd as its first source directory.
		// it is shared with all other windows whose contexts share GL object namespaces.
		// as such, adding a source directory can affect code obtaining shaders from sharing windows.
		const std::shared_ptr<ShaderManager> & shaderManager() const {
			return m_shaderman;
		}

		// get current state of a key
		bool getKey(int key);

		// get current state of a key, then clear it
		bool pollKey(int key);

		// get the current state of a mouse button
		bool getMouseButton(int button);

		// get the current state of a mouse button, then clear it
		bool pollMouseButton(int button);

		// windows must only be destroyed from the main thread
		~Window() {
			destroy();
		}

		static Window * currentContext();
	};

	inline void window_event::dispatchOrigin() const {
		// inheritance of Window from WindowEventDispatcher is not known until after definition on Window
		dispatch(*window);
	}

	class create_window_args {
	private:
		size2i m_size = size2i(512, 512);
		std::string m_title = "";
		GLFWmonitor *m_monitor = nullptr;
		const Window *m_share = nullptr;
		std::map<int, int> m_hints;

	public:
		create_window_args() {
			m_hints[GLFW_CONTEXT_VERSION_MAJOR] = 3;
			m_hints[GLFW_CONTEXT_VERSION_MINOR] = 3;
			m_hints[GLFW_OPENGL_PROFILE] = GLFW_OPENGL_CORE_PROFILE;
			m_hints[GLFW_OPENGL_FORWARD_COMPAT] = true;
			m_hints[GLFW_SAMPLES] = 0;
			m_hints[GLFW_VISIBLE] = false;
#ifndef NDEBUG
			// hint for debug context in debug build
			m_hints[GLFW_OPENGL_DEBUG_CONTEXT] = true;
#endif
		}

		create_window_args & width(int w) { m_size.w = w; return *this; }
		create_window_args & height(int h) { m_size.h = h; return *this; }
		create_window_args & size(int w, int h) { m_size.w = w; m_size.h = h; return *this; }
		create_window_args & size(size2i s) { m_size = s; return *this; }
		create_window_args & title(const std::string &title) { m_title = title; return *this; }
		create_window_args & monitor(GLFWmonitor *mon) { m_monitor = mon; return *this; }
		create_window_args & visible(bool b) { m_hints[GLFW_VISIBLE] = b; return *this; }
		create_window_args & resizable(bool b) { m_hints[GLFW_RESIZABLE] = b; return *this; }
		create_window_args & debug(bool b) { m_hints[GLFW_OPENGL_DEBUG_CONTEXT] = b; return *this; }
		create_window_args & share(const Window *win) { m_share = win; return *this; }
		create_window_args & hint(int target, int hint) { m_hints[target] = hint; return *this; }

		create_window_args & contextVersion(unsigned major, unsigned minor) {
			// forward compat only works for 3.0+
			if (major < 3) m_hints.erase(GLFW_OPENGL_FORWARD_COMPAT);
			// core/compat profiles only work for 3.2+
			if (major * 100 + minor <= 302) m_hints[GLFW_OPENGL_PROFILE] = GLFW_OPENGL_ANY_PROFILE;
			return *this;
		}

		// this should only be called from the main thread
		operator Window * ();

	};

	inline create_window_args createWindow() {
		return create_window_args();
	}
}

#endif
