/*
 *
 * GECom Window Management Header
 *
 */

// define GECOM_GL_NO_EXCEPTIONS globally
// to prevent GL debug callbacks from throwing exceptions

#ifndef GECOM_WINDOW_HPP
#define GECOM_WINDOW_HPP

#include <cstdint>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <memory>
#include <utility>
#include <bitset>
#include <array>

#include "GL.hpp"
#include "Log.hpp"
#include "Concurrent.hpp"
#include "Uncopyable.hpp"

namespace gecom {

	class WindowInit {
	private:
		static size_t refcount;
	public:
		WindowInit();
		~WindowInit();
	};

	inline void checkGL() {
		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			Log::error("GL") << "GL error: " << err;
			throw std::runtime_error("BOOM!");
		}
	}
	
	inline void checkFB() {
		GLenum err = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
		if (err != GL_FRAMEBUFFER_COMPLETE) {
			Log::error("GL") << "Framebuffer status: " << err;
			Log::error("GL") << "YOU BROKE THE FRAMEBUFFER!";
			throw std::runtime_error("OH NOES! THE FRAMEBUFFER IS BROKED");
		}
	}

	inline void checkExtension(const std::string &ext_name) {
		if (glfwExtensionSupported(ext_name.c_str())) {
			Log::info("GL") << "Extension " << ext_name << " detected.";
		} else {
			Log::error("GL") << "Extension " << ext_name << " not supported.";
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

	// joystick input state
	struct joystick_state {
		int token = -1;
		std::string name;
		std::array<float, 16> axes;
		std::bitset<64> buttons;

		joystick_state() {
			axes.fill(0.f);
		}

		joystick_state(const joystick_state &) = default;
		joystick_state(joystick_state &&) = default;
		joystick_state & operator=(const joystick_state &) = default;
		joystick_state & operator=(joystick_state &&) = default;

		float axis(unsigned i) const {
			return i < axes.size() ? axes[i] : 0.f;
		}

		bool testButton(unsigned b) const {
			return b < buttons.size() ? buttons[b] : false;
		}

		bool resetButton(unsigned b) {
			if (b >= buttons.size()) return false;
			bool r = buttons[b];
			buttons[b] = false;
			return r;
		}
	};

	// window forward declaration
	class Window;

	// window event proxy forward declaration
	class WindowEventProxy;

	// window event forward declarations
	struct window_event;
	struct window_refresh_event;
	struct window_close_event;
	struct window_pos_event;
	struct window_size_event;
	struct framebuffer_size_event;
	struct window_focus_event;
	struct window_icon_event;
	struct mouse_event;
	struct mouse_button_event;
	struct mouse_scroll_event;
	struct key_event;
	struct char_event;
	struct joystick_event;
	struct joystick_presence_event;
	struct joystick_button_event;

	// virtual event dispatch
	class WindowEventDispatcher {
	public:
		virtual void dispatchWindowRefreshEvent(const window_refresh_event &) { }
		virtual void dispatchWindowCloseEvent(const window_close_event &) { }
		virtual void dispatchWindowPosEvent(const window_pos_event &) { }
		virtual void dispatchWindowSizeEvent(const window_size_event &) { }
		virtual void dispatchFramebufferSizeEvent(const framebuffer_size_event &) { }
		virtual void dispatchWindowFocusEvent(const window_focus_event &) { }
		virtual void dispatchWindowIconEvent(const window_icon_event &) { }
		virtual void dispatchMouseEvent(const mouse_event &) { }
		virtual void dispatchMouseButtonEvent(const mouse_button_event &) { }
		virtual void dispatchMouseScrollEvent(const mouse_scroll_event &) { }
		virtual void dispatchKeyEvent(const key_event &) { }
		virtual void dispatchCharEvent(const char_event &) { }
		virtual void dispatchJoystickEvent(const joystick_event &) { }
		virtual void dispatchJoystickPresenceEvent(const joystick_presence_event &) { }
		virtual void dispatchJoystickButtonEvent(const joystick_button_event &) { }
		virtual ~WindowEventDispatcher() { }
	};

	// monotonically increasing event uid.
	// used to prevent duplicate dispatch through complex proxy configurations.
	// we'll just hope the counter never overflows.
	// at 100 events/frame, 60 frames/second:
	//  - 32-bit counter = ~198 hours
	//  - 64-bit counter = ~97,000,000 years
	// before overflow.
	uintmax_t makeWindowEventUID();

	// base window event
	struct window_event {
		// the window this event originated from; may be null
		Window *window = nullptr;

		// the proxy that sent this event; should not be null when received by an event observer
		// proxies set this automatically when an event is dispatched to them
		WindowEventProxy *proxy = nullptr;

		// event uid
		uintmax_t euid = makeWindowEventUID();

		// dispatch to virtual event dispatcher
		virtual void dispatch(WindowEventDispatcher &wed) const = 0;

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

	// window framebuffer size changed
	struct framebuffer_size_event : public window_event {
		size2i size;

		virtual void dispatch(WindowEventDispatcher &wed) const override {
			wed.dispatchFramebufferSizeEvent(*this);
		}
	};

	// window focused / unfocused
	struct window_focus_event : public window_event {
		bool focused = false;

		virtual void dispatch(WindowEventDispatcher &wed) const override {
			wed.dispatchWindowFocusEvent(*this);
		}
	};

	// window iconified (minimized) / deiconified (restored)
	struct window_icon_event : public window_event {
		bool iconified = false;

		virtual void dispatch(WindowEventDispatcher &wed) const override {
			wed.dispatchWindowIconEvent(*this);
		}
	};

	// mouse moved
	struct mouse_event : public window_event {
		point2d pos;
		bool entered = false;
		bool exited = false;

		virtual void dispatch(WindowEventDispatcher &wed) const override {
			wed.dispatchMouseEvent(*this);
		}
	};

	// mouse button pressed / released
	struct mouse_button_event : public mouse_event {
		int button = 0;
		int action = 0;
		int mods = 0;

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
		int key = 0;
		int scancode = 0;
		int action = 0;
		int mods = 0;

		virtual void dispatch(WindowEventDispatcher &wed) const override {
			wed.dispatchKeyEvent(*this);
		}
	};

	// unicode character typed
	struct char_event : public window_event {
		unsigned codepoint = 0;

		virtual void dispatch(WindowEventDispatcher &wed) const override {
			wed.dispatchCharEvent(*this);
		}
	};
	
	// joystick state updated
	struct joystick_event : public window_event {
		joystick_state state;

		virtual void dispatch(WindowEventDispatcher &wed) const override {
			wed.dispatchJoystickEvent(*this);
		}
	};

	struct joystick_presence_event : public joystick_event {
		bool present = false;

		virtual void dispatch(WindowEventDispatcher &wed) const override {
			wed.dispatchJoystickPresenceEvent(*this);
		}
	};

	struct joystick_button_event : public joystick_event {
		int button = 0;
		int action = 0;

		virtual void dispatch(WindowEventDispatcher &wed) const override {
			wed.dispatchJoystickButtonEvent(*this);
		}
	};

	// handles dispatched events and forwards them to subscribers
	// not actually thread-safe at the moment, so keep event dispatch to the main thread.
	class WindowEventProxy : public WindowEventDispatcher, private Uncopyable {
	protected:
		std::bitset<GLFW_KEY_LAST + 1> m_keystates;
		std::bitset<GLFW_MOUSE_BUTTON_LAST + 1> m_mbstates;
		joystick_state m_joystates[GLFW_JOYSTICK_LAST + 1];
		std::unordered_map<const Window *, point2d> m_mpos;
		const Window *m_last_key_win = nullptr;
		const Window *m_last_mouse_win = nullptr;
		uintmax_t m_last_euid = 0;

		void setKeyState(int key, bool state) {
			if (size_t(key) > GLFW_KEY_LAST) return;
			m_keystates[size_t(key)] = state;
		}

		void setMouseButtonState(int button, bool state) {
			if (size_t(button) > GLFW_MOUSE_BUTTON_LAST) return;
			m_mbstates[size_t(button)] = state;
		}

	public:
		Event<window_event> onEvent;
		Event<window_pos_event> onMove;
		Event<window_size_event> onResize;
		Event<framebuffer_size_event> onFramebufferResize;
		Event<window_refresh_event> onRefresh;
		Event<window_close_event> onClose;
		Event<window_focus_event> onFocus;
		Event<window_focus_event> onFocusGain;
		Event<window_focus_event> onFocusLose;
		Event<window_icon_event> onIcon;
		Event<window_icon_event> onIconMinimize;
		Event<window_icon_event> onIconRestore;
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
		Event<joystick_event> onJoystick;
		Event<joystick_presence_event> onJoystickPresence;
		Event<joystick_presence_event> onJoystickPresenceGain;
		Event<joystick_presence_event> onJoystickPresenceLose;
		Event<joystick_button_event> onJoystickButton;
		Event<joystick_button_event> onJoystickButtonPress;
		Event<joystick_button_event> onJoystickButtonRelease;

		// helper method; subscribes an event dispatcher to onEvent
		subscription_ptr subscribeEventDispatcher(std::shared_ptr<WindowEventDispatcher> wed) {
			return onEvent.subscribe([wed = std::move(wed)](const window_event &e) {
				e.dispatch(*wed);
				return false;
			});
		}

		virtual void dispatchWindowRefreshEvent(const window_refresh_event &) override;
		virtual void dispatchWindowCloseEvent(const window_close_event &) override;
		virtual void dispatchWindowPosEvent(const window_pos_event &) override;
		virtual void dispatchWindowSizeEvent(const window_size_event &) override;
		virtual void dispatchFramebufferSizeEvent(const framebuffer_size_event &) override;
		virtual void dispatchWindowFocusEvent(const window_focus_event &) override;
		virtual void dispatchWindowIconEvent(const window_icon_event &) override;
		virtual void dispatchMouseEvent(const mouse_event &) override;
		virtual void dispatchMouseButtonEvent(const mouse_button_event &) override;
		virtual void dispatchMouseScrollEvent(const mouse_scroll_event &) override;
		virtual void dispatchKeyEvent(const key_event &) override;
		virtual void dispatchCharEvent(const char_event &) override;
		virtual void dispatchJoystickEvent(const joystick_event &) override;
		virtual void dispatchJoystickPresenceEvent(const joystick_presence_event &) override;
		virtual void dispatchJoystickButtonEvent(const joystick_button_event &) override;

		void reset() {
			m_keystates.reset();
			m_mbstates.reset();
			m_mpos.clear();
		}

		bool testKey(unsigned k) const {
			return m_keystates.test(k);
		}

		bool resetKey(unsigned k) {
			bool r = m_keystates.test(k);
			m_keystates.reset(k);
			return r;
		}

		bool testMouseButton(unsigned b) const {
			return m_mbstates.test(b);
		}

		bool resetMouseButton(unsigned b) {
			bool r = m_mbstates.test(b);
			m_mbstates.reset(b);
			return r;
		}

		const Window * lastKeyWindow() const {
			return m_last_key_win;
		}

		const Window * lastMouseWindow() const {
			return m_last_mouse_win;
		}

		point2d mousePosition(const Window *win) const {
			auto it = m_mpos.find(win);
			if (it == m_mpos.end()) return point2d(-1, -1);
			return it->second;
		}

		point2d mousePosition() const {
			return mousePosition(lastMouseWindow());
		}

		bool joystickPresent(int token) const {
			return m_joystates[token].token == token;
		}

		const joystick_state & joystick(int token) const {
			return m_joystates[token];
		}

		joystick_state & joystick(int token) {
			return m_joystates[token];
		}
	};


	class window_error : public std::runtime_error {
	public:
		explicit window_error(const std::string &what_ = "Window Error") : runtime_error(what_) { }
	};

	// Thin wrapper around GLFW windowing.
	// Each window can be current on any one thread at once (GL),
	// but most functions can only be called on the main thread.
	class Window : public WindowEventProxy {
	private:
		// the wrapped window
		GLFWwindow* m_handle;

		// subscription to global event dispatch mechanism
		subscription_ptr m_global_event_sub;

		void initialize();
		void destroy();

	public:
		// ctor: takes ownership of a GLFW window handle
		Window(GLFWwindow *handle_, const Window *share = nullptr) : m_handle(handle_) {
			if (m_handle == nullptr) throw window_error("GLFW window handle is null");
			(void) share;
			initialize();
			m_last_key_win = this;
			m_last_mouse_win = this;
		}
		
		GLFWwindow * handle() const {
			return m_handle;
		}

		void pos(int x, int y) {
			assertMainThread();
			glfwSetWindowPos(m_handle, x, y);
		}

		void pos(const point2i &p) {
			assertMainThread();
			glfwSetWindowPos(m_handle, p.x, p.y);
		}

		point2i pos() const {
			assertMainThread();
			point2i p;
			glfwGetWindowPos(m_handle, &p.x, &p.y);
			return p;
		}

		void size(int w, int h) {
			assertMainThread();
			glfwSetWindowSize(m_handle, w, h);
		}

		void size(const size2i &s) {
			assertMainThread();
			glfwSetWindowSize(m_handle, s.w, s.h);
		}

		size2i size() const {
			assertMainThread();
			size2i s;
			glfwGetWindowSize(m_handle, &s.w, &s.h);
			return s;
		}

		size2i framebufferSize() const {
			assertMainThread();
			size2i s;
			glfwGetFramebufferSize(m_handle, &s.w, &s.h);
		}

		void width(int w) {
			size2i s = size();
			s.w = w;
			size(s);
		}

		int width() const {
			assertMainThread();
			int w, h;
			glfwGetWindowSize(m_handle, &w, &h);
			return w;
		}

		int framebufferWidth() const {
			assertMainThread();
			int w, h;
			glfwGetFramebufferSize(m_handle, &w, &h);
			return w;
		}

		void height(int h) {
			size2i s = size();
			s.h = h;
			size(s);
		}

		int height() const {
			assertMainThread();
			int w, h;
			glfwGetWindowSize(m_handle, &w, &h);
			return h;
		}

		int framebufferHeight() const {
			assertMainThread();
			int w, h;
			glfwGetFramebufferSize(m_handle, &w, &h);
			return h;
		}

		void title(const std::string &s) {
			assertMainThread();
			glfwSetWindowTitle(m_handle, s.c_str());
		}

		void visible(bool b) {
			assertMainThread();
			if (b) {
				glfwShowWindow(m_handle);
			} else {
				glfwHideWindow(m_handle);
			}
		}

		// can be called from any thread
		bool shouldClose() const {
			return glfwWindowShouldClose(m_handle);
		}

		// can be called from any thread
		void makeCurrent();

		// can be called from any thread
		void swapBuffers() const {
			glfwSwapBuffers(m_handle);
		}

		int attrib(int a) const {
			assertMainThread();
			return glfwGetWindowAttrib(m_handle, a);
		}

		~Window() {
			destroy();
		}

		// can be called from any thread
		static Window * current();

		// main thread only (at the moment)
		static void dispatchGlobalEvent(const window_event &);

		// main thread only
		static void pollEvents();
	};

	class create_window_args {
	private:
		size2i m_size = size2i(512, 512);
		std::string m_title = "";
		GLFWmonitor *m_monitor = nullptr;
		const Window *m_share = nullptr;
		std::unordered_map<int, int> m_hints;

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
			if (major * 100 + minor < 302) m_hints[GLFW_OPENGL_PROFILE] = GLFW_OPENGL_ANY_PROFILE;
			// set context version hints
			m_hints[GLFW_CONTEXT_VERSION_MAJOR] = major;
			m_hints[GLFW_CONTEXT_VERSION_MINOR] = minor;
			return *this;
		}

		// this should only be called from the main thread
		operator Window * ();

	};

	inline create_window_args createWindow() {
		return create_window_args();
	}
}

namespace {
	gecom::WindowInit window_init_obj;
}

#endif
