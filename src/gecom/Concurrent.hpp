#ifndef GECOM_CONCURRENT_HPP
#define GECOM_CONCURRENT_HPP

#include <cassert>
#include <exception>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <utility>
#include <tuple>
#include <future>
#include <memory>
#include <type_traits>

#include "Log.hpp"
#include "Uncopyable.hpp"

namespace gecom {
	
	// asynchronous execution services
	namespace async {

		using clock = std::chrono::steady_clock;

		std::thread::id mainThreadId() noexcept;

		// set maximum number of concurrently scheduled background tasks.
		// must be at least 1.
		void concurrency(size_t);

		// get maximum number of concurrently scheduled background tasks.
		// at least 1.
		size_t concurrency() noexcept;

		// allow the current background task to be descheduled in favour of a higher priority task.
		// if not called from a background task execution thread, does nothing.
		void yield() noexcept;

		// execute tasks affinitized to the current thread until all tasks have executed or the
		// specified time budget has lapsed. returns number of tasks executed.
		size_t execute(clock::duration timebudget) noexcept;

		namespace detail {

			// run-once, no copy
			class Runnable : private Uncopyable {
			public:
				virtual void run() = 0;
				virtual ~Runnable() { }
			};

			// capture function and arguments
			template <typename FunT, typename ...ArgTR>
			class BasicRunnable : public Runnable {
			private:
				using result_t = decltype(std::declval<FunT>()(std::declval<ArgTR>()...));
				FunT m_fun;
				std::tuple<ArgTR...> m_args;
				std::promise<result_t> m_promise;

				template <typename ResultT, typename Dummy = void>
				struct call_impl {
					static void go(BasicRunnable &this_) {
						this_.m_promise.set_value(util::call(std::move(this_.m_fun), std::move(this_.m_args)));
					}
				};

				template <typename Dummy>
				struct call_impl<void, Dummy> {
					static void go(BasicRunnable &this_) {
						util::call(std::move(this_.m_fun), std::move(this_.m_args));
						this_.m_promise.set_value();
					}
				};

			public:
				template <typename ArgTupleT>
				BasicRunnable(FunT fun_, ArgTupleT &&args) : m_fun(std::move(fun_)), m_args(std::forward<ArgTupleT>(args)) { }

				auto getFuture() {
					return m_promise.get_future();
				}

				virtual void run() override {
					try {
						call_impl<result_t>::go(*this);
					} catch (...) {
						m_promise.set_exception(std::current_exception());
						throw;
					}
				}
			};

			using runnable_ptr = std::unique_ptr<Runnable>;

			template <typename FunT, typename ...ArgTR>
			auto make_runnable(FunT &&fun, ArgTR &&...args) {
				return std::make_unique<BasicRunnable<std::decay_t<FunT &&>, std::decay_t<ArgTR &&>...>>(
					std::forward<FunT>(fun),
					std::forward_as_tuple(std::forward<ArgTR>(args)...)
				);
			}
			
			void invoke(std::thread::id affinity, clock::time_point deadline, runnable_ptr taskfun);

		}

		// invoke an affinitized task (absolute deadline)
		template <typename TaskFunT, typename ...ArgTR>
		inline auto invoke(std::thread::id affinity, clock::time_point deadline, TaskFunT &&taskfun, ArgTR &&...args) -> std::future<decltype(taskfun(args...))> {
			// wrap function and args in non-template functor, forward to task engine, return future
			auto r = detail::make_runnable(std::forward<TaskFunT>(taskfun), std::forward<ArgTR>(args)...);
			auto f = r->getFuture();
			detail::invoke(std::move(affinity), std::move(deadline), std::move(r));
			return f;
		}

		// invoke an affinitized task (relative deadline)
		template <typename TaskFunT, typename ...ArgTR>
		inline auto invoke(std::thread::id affinity, clock::duration deadline, TaskFunT &&taskfun, ArgTR &&...args) -> std::future<decltype(taskfun(args...))> {
			return invoke(std::move(affinity), clock::now() + deadline, std::forward<TaskFunT>(taskfun), std::forward<ArgTR>(args)...);
		}

		// invoke a background task (absolute deadline)
		template <typename TaskFunT, typename ...ArgTR>
		inline auto invoke(clock::time_point deadline, TaskFunT &&taskfun, ArgTR &&...args) -> std::future<decltype(taskfun(args...))> {
			return invoke(std::thread::id(), std::move(deadline), std::forward<TaskFunT>(taskfun), std::forward<ArgTR>(args)...);
		}

		// invoke a background task (relative deadline)
		template <typename TaskFunT, typename ...ArgTR>
		inline auto invoke(clock::duration deadline, TaskFunT &&taskfun, ArgTR &&...args) -> std::future<decltype(taskfun(args...))> {
			return invoke(clock::now() + deadline, std::forward<TaskFunT>(taskfun), std::forward<ArgTR>(args)...);
		}

		// invoke a task affinitized to the main thread (absolute deadline)
		template <typename TaskFunT, typename ...ArgTR>
		inline auto invokeMain(clock::time_point deadline, TaskFunT &&taskfun, ArgTR &&...args) -> std::future<decltype(taskfun(args...))> {
			return invoke(mainThreadId(), std::move(deadline), std::forward<TaskFunT>(taskfun), std::forward<ArgTR>(args)...);
		}

		// invoke a task affinitized to the main thread (relative deadline)
		template <typename TaskFunT, typename ...ArgTR>
		inline auto invokeMain(clock::duration deadline, TaskFunT &&taskfun, ArgTR &&...args) -> std::future<decltype(taskfun(args...))> {
			return invokeMain(clock::now() + deadline, std::forward<TaskFunT>(taskfun), std::forward<ArgTR>(args)...);
		}
	}

	class interruption { };
	
	class Subscription : private Uncopyable {
	public:
		// test if subscription is valid (able to be manipulated)
		virtual operator bool() = 0;

		// test if subscription is enabled; returns false if invalid
		virtual bool enabled() = 0;

		// enable or disable subscription; returns true iff enabled state was set successfully
		virtual bool enable(bool b) { return !b; }

		// if subscription is valid, cancel and make invalid; return true iff cancellation succeeded
		virtual bool cancel() noexcept = 0;

		// if subscription is valid, make invalid without cancelling
		virtual void forever() = 0;

		// destroy subscription; cancels if valid
		virtual ~Subscription() { }
	};

	using subscription_ptr = std::unique_ptr<Subscription>;


	// event dispatch mechanism.
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

		struct observer_registry {
			// protects only observer attachment/detachment
			std::mutex mutex;
			// next observer attachment key
			unsigned next_key = 0;
			// observers: enabled + callback
			std::unordered_map<unsigned, std::pair<bool, observer_t>> observers;
		};

		// returned subscriptions maintain a weak_ptr to this so cancellation
		// can happen regardless of whether the event is alive
		std::shared_ptr<observer_registry> m_registry;

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

		// subscription type for this event type
		class EventSubscription : public Subscription {
		private:
			std::weak_ptr<observer_registry> m_wpreg;
			unsigned m_key;

		public:
			EventSubscription(std::weak_ptr<observer_registry> wpreg_, unsigned key_) : m_wpreg(wpreg_), m_key(key_) { }

			virtual operator bool() override {
				return bool(m_wpreg.lock());
			}

			virtual bool enabled() override {
				auto spreg = m_wpreg.lock();
				if (!spreg) return false;
				std::unique_lock<std::mutex> lock(spreg->mutex);
				auto it = spreg->observers.find(m_key);
				if (it == spreg->observers.end()) return false;
				return it->second.first;
			}

			virtual bool enable(bool b) override {
				auto spreg = m_wpreg.lock();
				if (!spreg) return false;
				std::unique_lock<std::mutex> lock(spreg->mutex);
				auto it = spreg->observers.find(m_key);
				if (it == spreg->observers.end()) return false;
				it->second.first = b;
				return true;
			}

			virtual bool cancel() noexcept override {
				auto spreg = m_wpreg.lock();
				if (!spreg) return false;
				std::unique_lock<std::mutex> lock(spreg->mutex);
				bool r = spreg->observers.erase(m_key);
				m_wpreg.reset();
				return r;
			}

			virtual void forever() override {
				m_wpreg.reset();
			}

			virtual ~EventSubscription() {
				cancel();
			}
		};

	public:
		Event() : m_registry(std::make_shared<observer_registry>()) { }

		// subscribe an observer to this event.
		// the observer will be removed when the returned subscription is destroyed or
		// has cancel() otherwise called on it, when the event is destroyed,
		// or when the observer returns true, whichever is first.
		subscription_ptr subscribe(observer_t func) {
			std::unique_lock<std::mutex> lock(m_registry->mutex);
			unsigned key = m_registry->next_key++;
			m_registry->observers[key] = std::make_pair(true, std::move(func));
			return std::make_unique<EventSubscription>(m_registry, key);
		}

		void interrupt() {
			// wake all waiting threads
			m_cond.notify_all();
		}

		// notify this event; wakes all waiting threads
		void notify(const EventArgT &e) {
			{
				// use a new scope so the condition is notified after the mutexes are unlocked
				std::unique_lock<std::mutex> lock(m_registry->mutex);
				std::unique_lock<std::mutex> lock2(m_mutex);
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
			m_cond.wait(lock);
			// if the notify count changed, the event was triggered
			return m_count != count0;
		}

		// TODO timed wait etc

		virtual ~Event() {
			section_guard sec("Event");
			// clear the observer registry to release memory
			{
				std::unique_lock<std::mutex> lock(m_registry->mutex);
				m_registry->observers = { };
			}
			// interrupt all waiting threads, then wait for them to unlock the mutex
			auto time0 = std::chrono::steady_clock::now();
			while (true) {
				// interrupt any threads waiting on this event still
				interrupt();
				std::this_thread::yield();
				std::unique_lock<std::mutex> lock(m_mutex);
				// test if we can go home yet
				if (m_waiters == 0) break;
				if (std::chrono::steady_clock::now() - time0 > std::chrono::milliseconds(100)) {
					// failed to finish within timeout
					Log::error() << "Destructor failed to finish within timeout";
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
		blocking_queue() { }

		blocking_queue(const blocking_queue &other) {
			std::unique_lock<std::mutex> lock1(m_mutex, std::defer_lock);
			std::unique_lock<std::mutex> lock2(other.m_mutex, std::defer_lock);
			std::lock(lock1, lock2);
			m_queue = other.m_queue;
		}

		blocking_queue(blocking_queue &&other) {
			std::unique_lock<std::mutex> lock1(m_mutex, std::defer_lock);
			std::unique_lock<std::mutex> lock2(other.m_mutex, std::defer_lock);
			std::lock(lock1, lock2);
			m_queue = std::move(other.m_queue);
		}

		blocking_queue & operator=(const blocking_queue &other) {
			std::unique_lock<std::mutex> lock1(m_mutex, std::defer_lock);
			std::unique_lock<std::mutex> lock2(other.m_mutex, std::defer_lock);
			std::lock(lock1, lock2);
			m_queue = other.m_queue;
			return *this;
		}

		blocking_queue & operator=(blocking_queue &&other) {
			std::unique_lock<std::mutex> lock1(m_mutex, std::defer_lock);
			std::unique_lock<std::mutex> lock2(other.m_mutex, std::defer_lock);
			std::lock(lock1, lock2);
			m_queue = std::move(other.m_queue);
			return *this;
		}

		void interrupt() {
			// wake up all waiters
			m_condition.notify_all();
		}

		void push(T const& value) {
			{
				std::unique_lock<std::mutex> lock(m_mutex);
				m_queue.push_front(value);
			}
			m_condition.notify_one();
		}

		T pop() {
			std::unique_lock<std::mutex> lock(m_mutex);
			if (m_queue.empty()) {
				// wait for push
				m_condition.wait(lock);
				// push wakes up only one waiter, so if queue is still empty waiting was interrupted
				if (m_queue.empty()) throw interruption();
			}
			T rc(std::move(m_queue.back()));
			m_queue.pop_back();
			return rc;
		}

		bool pop(T &ret) {
			std::unique_lock<std::mutex> lock(m_mutex);
			if (m_queue.empty()) return false;
			ret = std::move(m_queue.back());
			m_queue.pop_back();
			return true;
		}

		bool empty() {
			std::unique_lock<std::mutex> lock(m_mutex);
			return m_queue.empty();
		}

	};

}

#endif