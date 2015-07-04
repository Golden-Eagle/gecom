#ifndef GECOM_CONCURRENT_HPP
#define GECOM_CONCURRENT_HPP

#include <cassert>
#include <stdexcept>
#include <map>
#include <unordered_map>
#include <vector>
#include <deque>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <utility>

#include "GECom.hpp"
#include "Log.hpp"

namespace gecom {
	
	class interruption  { };

	// High-level mechanism for providing interruption of condition variable waiting.
	// I dont know how well this actually performs, but it seems to work at least.
	// Only threads that are waiting using this class can be interrupted using this class.
	class InterruptManager {
	private:
		struct thread_data_t {
			std::condition_variable *condition;
			std::mutex *mutex;
			bool interrupt;
		};

		static std::mutex m_mutex;
		static std::map<std::thread::id, thread_data_t> m_thread_data;

	public:
		// wait on a condition variable.
		// lock should already be locked.
		static void wait(std::condition_variable &cond, std::unique_lock<std::mutex> &lock);

		// interrupt a thread waiting on a condition variable.
		// if thread is not waiting, it will be interrupted when next it does.
		static void interrupt(const std::thread::id &id);

		// interrupt all threads waiting on a condition variable.
		// the mutex this condition variable is waiting with is assumed to be locked already.
		static void interrupt(std::condition_variable &cond);

	};

	// Generic subscription class. Wraps a cancellation function, and automatically
	// calls it when destroyed. Individual subscription objects should not be shared
	// between threads, but cancellation itself is as threadsafe as the supplied function.
	// In the case of Event classes, cancellation is threadsafe, and does not require
	// the event to not have been deleted. Also wraps an (optional) enabler function,
	// that can be used to temporarily enable / disable the subscription.
	class subscription {
	public:
		using cancel_t = std::function<bool()>;
		using enable_t = std::function<bool(bool)>;

	private:
		cancel_t m_cancel;
		enable_t m_enable;

	public:
		// default ctor: has no cancellation function or enabler function
		subscription() { }

		// ctor: provide cancellation function and optionally enabler function
		subscription(cancel_t cancel_, enable_t enable_ = enable_t()) :
			m_cancel(std::move(cancel_)), m_enable(std::move(enable_))
		{ }

		// not copyable
		subscription(const subscription &other) = delete;
		subscription & operator=(const subscription &other) = delete;

		// move ctor: take over cancellation responsibility
		subscription(subscription &&other) :
			m_cancel(std::move(other.m_cancel)),
			m_enable(std::move(other.m_enable))
		{ }

		// move assign: cancel current subscription (if any) and then take over cancellation responsibility
		subscription & operator=(subscription &&other) {
			cancel();
			m_cancel = std::move(other.m_cancel);
			m_enable = std::move(other.m_enable);
			return *this;
		}

		// explicitly cancel this subscription.
		// returns true iff cancellation actually happened.
		bool cancel() {
			if (m_cancel) {
				bool r = m_cancel();
				// after cancelling, subscription cannot be cancelled or enabled / disabled
				m_cancel = cancel_t();
				m_enable = enable_t();
				return r;
			}
			return false;
		}

		// enable or disable subscription.
		// returns true if enable state was changed.
		bool enable(bool b) {
			if (m_enable) {
				return m_enable(b);
			}
			return false;
		}

		// remove the cancellation function (if any) without calling it.
		// in the case of Event subscriptions, the associated observer can no longer
		// be detached from 'outside'; it will live as long as the event does, unless
		// the observer function itself requests to be detached by returning true.
		// this does not prevent the subscription from being enabled / disabled.
		void forever() {
			m_cancel = cancel_t();
		}

		// dtor: cancel current subscription (if any)
		~subscription() {
			cancel();
		}
	};

	// event dispatch mechanism.
	// thread safe, but beware destroying an event while another thread is trying to use it.
	// use shared_ptr for things like that (did i even need to say that? it should be obvious).
	template <typename EventArgT>
	class Event : private Uncopyable {
	public:
		// return true to detach
		using observer_t = std::function<bool(const EventArgT &)>;

	private:
		// notify count, determines if wakeup was intended
		unsigned m_count = 0;
		// currently-waiting-threads count
		unsigned m_waiters = 0;
		// this cannot be a recursive mutex because of the condition variable
		// protects the event as a whole, and used for waiting
		std::mutex m_mutex;
		std::condition_variable m_cond;

		struct observer_registry_t {
			// protects only observer attachment/detachment
			std::mutex mutex;
			// next observer attachment key
			unsigned next_key = 0;
			// observers: enabled + callback
			std::unordered_map<unsigned, std::pair<bool, observer_t>> observers;

			 observer_registry_t() { }
		};

		// returned subscriptions maintain a weak_ptr to this so cancellation
		// can happen regardless of whether the event is alive
		std::shared_ptr<observer_registry_t> m_registry;

		// ensures that the wait-count is maintained in an exception-safe manner
		class waiter_guard {
		private:
			unsigned *m_waiters;

		public:
			waiter_guard(unsigned *waiters_) : m_waiters(waiters_) {
				(*m_waiters)++;
			}

			~waiter_guard() {
				(*m_waiters)--;
			}
		};

	public:
		inline Event() : m_registry(std::make_shared<observer_registry_t>()) { }

		// subscribe an observer to this event.
		// the observer will be removed when the returned subscription is destroyed or
		// has cancel() otherwise called on it, or when the event is destroyed, whichever is first.
		subscription subscribe(const observer_t &func) {
			std::lock_guard<std::mutex> lock(m_registry->mutex);
			unsigned key = m_registry->next_key++;
			m_registry->observers[key] = std::make_pair(true, func);
			// subscription functions keeps a weak_ptr to the observer registry
			auto wpreg = std::weak_ptr<observer_registry_t>(m_registry);
			return subscription(
				// cancellation function
				[=]() {
					auto spreg = wpreg.lock();
					// if we can't lock the pointer, event has gone byebye
					if (spreg) {
						// got a shared_ptr to the registry, so we can manipulate it
						// without worrying about it being deleted
						std::lock_guard<std::mutex> lock2(spreg->mutex);
						return bool(spreg->observers.erase(key));
					}
					return false;
				},
				// enabler function
				[=](bool b) {
					auto spreg = wpreg.lock();
					if (spreg) {
						std::lock_guard<std::mutex> lock2(spreg->mutex);
						auto it = spreg->observers.find(key);
						if (it != spreg->observers.end()) {
							// key is in registry (this is good, its supposed to be)
							if (b != it->second.first) {
								it->second.first = b;
								// return true iff enable state changed
								return true;
							}
						}
					}
					return false;
				}
			);
		}

		// notify this event; wakes all waiting threads
		void notify(const EventArgT &e) {
			{
				// use a new scope so the condition is notified are the mutexes are unlocked
				std::lock_guard<std::mutex> lock(m_registry->mutex);
				std::lock_guard<std::mutex> lock2(m_mutex);
				// increment notify count to signal that wakeups are valid
				m_count++;
				// do we have observers?
				if (!m_registry->observers.empty()) {
					// keep a list of observers requesting detachment while calling them
					std::vector<unsigned> detach_keys;
					for (const auto &entry : m_registry->observers) {
						// skip if disabled
						if (!entry.second.first) continue;
						// call observer
						if (entry.second.second(e)) {
							detach_keys.push_back(entry.first);
						}
					}
					// perform detachments
					for (auto key : detach_keys) {
						m_registry->observers.erase(key);
					}
				}
			}
			// wake waiting threads
			m_cond.notify_all();
		}

		// wait on this event; returns true if the event was fired
		bool wait() {
			// no need to lock the observer registry
			std::unique_lock<std::mutex> lock(m_mutex);
			waiter_guard waiter(&m_waiters);
			// record the notify count at start of waiting
			unsigned count0 = m_count;
			// if this thread was interrupted while waiting, this will throw
			InterruptManager::wait(m_cond, lock);
			// if the notify count changed, the event was triggered
			return m_count != count0;
		}

		// TODO timed wait etc

		virtual ~Event() {
			// no need to lock the observer registry
			// interrupt all waiting threads, then wait for them to unlock the mutex
			auto time0 = std::chrono::steady_clock::now();
			while (true) {
				std::this_thread::yield();
				std::lock_guard<std::mutex> lock(m_mutex);
				// test if we can go home yet
				if (m_waiters == 0) break;
				// interrupt any threads waiting on this event still
				InterruptManager::interrupt(m_cond);
				if (std::chrono::steady_clock::now() - time0 > std::chrono::milliseconds(100)) {
					// failed to finish within timeout
					log("Event").error() << "Destructor failed to finish within timeout";
					std::abort();
				}
			}
		}
	};

	// simple blocking queue
	template <typename T>
	class blocking_queue {
	private:
		std::mutex m_mutex;
		std::condition_variable m_condition;
		std::deque<T> m_queue;

	public:
		inline blocking_queue() { }

		inline blocking_queue(const blocking_queue &other) {
			std::unique_lock<std::mutex> lock1(m_mutex, std::defer_lock);
			std::unique_lock<std::mutex> lock2(other.m_mutex, std::defer_lock);
			std::lock(lock1, lock2);
			m_queue = other.m_queue;
		}

		inline blocking_queue(blocking_queue &&other) {
			std::unique_lock<std::mutex> lock1(m_mutex, std::defer_lock);
			std::unique_lock<std::mutex> lock2(other.m_mutex, std::defer_lock);
			std::lock(lock1, lock2);
			m_queue = std::move(other.m_queue);
		}

		inline blocking_queue & operator=(const blocking_queue &other) {
			std::unique_lock<std::mutex> lock1(m_mutex, std::defer_lock);
			std::unique_lock<std::mutex> lock2(other.m_mutex, std::defer_lock);
			std::lock(lock1, lock2);
			m_queue = other.m_queue;
			return *this;
		}

		inline blocking_queue & operator=(blocking_queue &&other) {
			std::unique_lock<std::mutex> lock1(m_mutex, std::defer_lock);
			std::unique_lock<std::mutex> lock2(other.m_mutex, std::defer_lock);
			std::lock(lock1, lock2);
			m_queue = std::move(other.m_queue);
			return *this;
		}

		inline void push(T const& value) {
			{
				std::unique_lock<std::mutex> lock(m_mutex);
				m_queue.push_front(value);
			}
			m_condition.notify_one();
		}

		inline T pop() {
			std::unique_lock<std::mutex> lock(m_mutex);
			while (m_queue.empty()) {
				// if this thread is interrupted while waiting, this will throw
				InterruptManager::wait(m_condition, lock);
			}
			T rc(std::move(m_queue.back()));
			m_queue.pop_back();
			return rc;
		}

		inline bool pop(T &ret) {
			std::unique_lock<std::mutex> lock(m_mutex);
			if (m_queue.empty()) return false;
			ret = std::move(m_queue.back());
			m_queue.pop_back();
			return true;
		}

		inline bool empty() {
			std::unique_lock<std::mutex> lock(m_mutex);
			return m_queue.empty();
		}

	};

	// mechanism for asynchronous execution of arbitrary tasks
	// yes i know std::async exists
	class AsyncExecutor {
	public:
		using task_t = std::function<void(void)>;

	private:
		static bool m_started;
		static blocking_queue<task_t> m_fast_queue, m_slow_queue;
		static std::thread m_fast_thread, m_slow_thread;
		static std::thread::id m_main_id;
		static std::mutex m_exec_mutex;
		static std::map<std::thread::id, blocking_queue<task_t>> m_exec_queues;

	public:
		// start the background threads.
		// must be called from the main thread.
		static inline void start() {
			if (!m_started) {
				log("AsyncExec").information(0) << "Starting...";
				m_main_id = std::this_thread::get_id();
				m_fast_thread = std::thread([] {
					log("AsyncExec:fast").information(0) << "Background thread started";
					while (true) {
						task_t task;
						try {
							task = m_fast_queue.pop();
						} catch (interruption &) {
							// thread needs to quit
							log("AsyncExec:fast") << "Interrupted, exiting";
							break;
						}
						try {
							task();
						} catch (std::exception &e) {
							log("AsyncExec:fast").error() << "Uncaught exception; what(): " << e.what();
						} catch (...) {
							log("AsyncExec:fast").error() << "Uncaught exception (not derived from std::exception)";
						}
					}
				});
				m_slow_thread = std::thread([] {
					log("AsyncExec:slow").information(0) << "Background thread started";
					while (true) {
						task_t task;
						try {
							task = m_slow_queue.pop();
						} catch (interruption &) {
							// thread needs to quit
							log("AsyncExec:slow") << "Interrupted, exiting";
							break;
						}
						try {
							task();
						} catch (std::exception &e) {
							log("AsyncExec:slow").error() << "Uncaught exception; what(): " << e.what();
						} catch (...) {
							log("AsyncExec:slow").error() << "Uncaught exception (not derived from std::exception)";
						}
					}
				});
				m_started = true;
			}
		}

		// stop the background threads.
		// must be called from the main thread before exit() to ensure nice application shutdown.
		// cannot be registered with atexit() due to MSVC stdlib bug
		// https://connect.microsoft.com/VisualStudio/feedback/details/747145/std-thread-join-hangs-if-called-after-main-exits-when-using-vs2012-rc
		static inline void stop() {
			if (m_started) {
				log("AsyncExec").information(0) << "Stopping background threads...";
				// give the last log message time to show up
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				InterruptManager::interrupt(m_fast_thread.get_id());
				InterruptManager::interrupt(m_slow_thread.get_id());
				m_fast_thread.join();
				m_slow_thread.join();
			}
		}

		// add a high-priority background task with expected duration < ~50ms.
		// this always goes to the same background thread.
		static inline void enqueueFast(const task_t &f) {
			m_fast_queue.push(f);
		}

		// add a low-priority or slow (but still non-blocking) background task
		// this always goes to the same background thread.
		static inline void enqueueSlow(const task_t &f) {
			m_slow_queue.push(f);
		}

		// add a task to a specific thread
		static inline void enqueue(const std::thread::id &tid, const task_t &f) {
			std::lock_guard<std::mutex> lock(m_exec_mutex);
			auto it = m_exec_queues.find(tid);
			if (it == m_exec_queues.end()) {
				// create a new queue
				blocking_queue<task_t> q;
				q.push(f);
				m_exec_queues[tid] = std::move(q);
			} else {
				it->second.push(f);
			}
		}

		// execute tasks for the current thread up to some time limit
		template <typename RepT, typename Period>
		static inline void execute(const std::chrono::duration<RepT, Period> &dur) {
			blocking_queue<task_t> *q = nullptr;
			{
				std::lock_guard<std::mutex> lock(m_exec_mutex);
				auto it = m_exec_queues.find(std::this_thread::get_id());
				if (it != m_exec_queues.end()) q = &it->second;
				// safe to release this lock because the queues never get destroyed
			}
			if (q) {
				// there is a queue for this thread
				auto time1 = std::chrono::steady_clock::now() + dur;
				do {
					task_t task;
					if (!q->pop(task)) return;
					try {
						task();
					} catch (std::exception &e) {
						log("AsyncExec").error() << "Uncaught exception on thread " << std::this_thread::get_id() << "; what(): " << e.what();
					} catch (...) {
						log("AsyncExec").error() << "Uncaught exception on thread " << std::this_thread::get_id() << " (not derived from std::exception)";
					}
				} while (std::chrono::steady_clock::now() < time1);
			}
		}

		// get the id of the main thread.
		// start() must have completed before calling.
		static inline std::thread::id mainThreadID() {
			assert(m_started && "AsyncExecutor not started");
			return m_main_id;
		}

		// add a task to the 'main' thread
		static inline void enqueueMain(const task_t &f) {
			enqueue(mainThreadID(), f);
		}

	};

}

#endif