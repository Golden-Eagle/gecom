
#include <vector>
#include <unordered_map>
#include <cstdio>

#include "Concurrent.hpp"
#include "Log.hpp"
#include "Util.hpp"
#include "Terminal.hpp"

using namespace std;
using namespace gecom;

namespace gecom {

	thread::id mainThreadId() noexcept {
		static thread::id mid { this_thread::get_id() };
		return mid;
	}

	namespace async {

		namespace {

			class Task;
			using task_ptr = shared_ptr<Task>;

			class Worker;
			using worker_ptr = shared_ptr<Worker>;

			class Task : public enable_shared_from_this<Task> {
			private:
				clock::time_point m_deadline;
				detail::runnable_ptr m_runnable;
				worker_ptr m_worker;

			public:
				Task(clock::time_point deadline_, detail::runnable_ptr runnable_) : m_deadline(move(deadline_)), m_runnable(move(runnable_)) { }

				const clock::time_point & deadline() const {
					return m_deadline;
				}

				void run() const {
					m_runnable->run();
				}

				bool started() const {
					return bool(m_worker);
				}

				void start(worker_ptr);

				worker_ptr finish();

				void resume();

				void suspend();

				~Task() {
					assert(!m_worker);
				}
			};

			struct task_compare {
				bool operator()(const task_ptr &a, const task_ptr &b) {
					return a->deadline() > b->deadline();
				}
			};

			class Worker : public enable_shared_from_this<Worker> {
			private:
				mutex m_mutex;
				condition_variable m_resume_cond;
				task_ptr m_task;
				atomic<bool> m_should_yield { true };
				thread m_thread;

				static void run(Worker *);

			public:
				Worker() {
					section_guard sec("Async");
					Log::info() << "Worker starting...";
					// must start thread after constructing other fields
					m_thread = thread(run, this);
				}

				void employ(task_ptr task) {
					unique_lock<mutex> lock(m_mutex);
					assert(!m_task);
					m_task = move(task);
					resume();
				}

				void resume() noexcept {
					m_should_yield = false;
					m_resume_cond.notify_all();
				}

				void suspend() noexcept {
					m_should_yield = true;
				}

				void yield() {
					while (m_should_yield) {
						unique_lock<mutex> lock(m_mutex);
						m_resume_cond.wait(lock);
					}
				}

				~Worker() {
					section_guard sec("Async");
					// wake up the thread so it can exit
					m_resume_cond.notify_all();
					m_thread.join();
					Log::info() << "Worker terminated";
				}
			};

			auto & currentWorker() {
				static thread_local Worker *cw { nullptr };
				return cw;
			}

			void Task::start(worker_ptr p) {
				assert(!m_worker);
				m_worker = move(p);
				m_worker->employ(shared_from_this());
				resume();
			}

			worker_ptr Task::finish() {
				assert(m_worker);
				suspend();
				return move(m_worker);
			}

			void Task::resume() {
				assert(m_worker);
				m_worker->resume();
			}

			void Task::suspend() {
				if (m_worker) {
					m_worker->suspend();
				}
			}

			class BackgroundScheduler {
			private:
				struct Statics {
					// current worker pointer must be valid until all workers have finished
					bool ensure_init_current_worker { (currentWorker(), true) };
					atomic<bool> should_exit { false };
					atomic<size_t> max_active_tasks { 1 };
					mutex pool_mutex;
					condition_variable pool_cond;
					vector<worker_ptr> pool;
					size_t pool_acquire_count = 0;
					mutex task_mutex;
					vector<task_ptr> active_tasks;
					util::priority_queue<task_ptr, task_compare> pending_tasks;

					Statics() {
						Log::info("Async") << "Background task scheduler initialized";
					}

					~Statics() {
						section_guard sec("Async");
						Log::info() << "Background task scheduler deinitializing...";
						// signal workers should exit
						should_exit = true;
						// release our references to all tasks; only tasks associated with a worker thread survive
						pending_tasks.clear();
						active_tasks.clear();
						// wait for all acquired workers to be released as their tasks complete
						unique_lock<mutex> lock(pool_mutex);
						while (pool_acquire_count > 0) {
							pool_cond.wait(lock);
						}
						// clear worker pool, waits for idle workers to terminate
						pool.clear();
						Log::info() << "Background task scheduler deinitialized";
					}
				};

				static auto & statics() {
					static Statics s;
					return s;
				}

				static worker_ptr acquireWorker() {
					//Log::info("Async") << "Worker acquire";
					unique_lock<mutex> lock(statics().pool_mutex);
					if (statics().pool.empty()) {
						auto r = make_shared<Worker>();
						statics().pool_acquire_count++;
						return r;
					} else {
						auto r = statics().pool.back();
						statics().pool.pop_back();
						statics().pool_acquire_count++;
						return r;
					}
				}

				static void releaseWorker(worker_ptr p) {
					//Log::info("Async") << "Worker release";
					{
						unique_lock<mutex> lock(statics().pool_mutex);
						statics().pool.push_back(move(p));
						statics().pool_acquire_count--;
					}
					statics().pool_cond.notify_all();
				}

				static void reschedule() {
					if (shouldExit()) return;
					// task_mutex assumed owned
					// move currently active tasks to possibly active task set
					util::priority_queue<task_ptr, task_compare> possibly_active_tasks(
						std::make_move_iterator(statics().active_tasks.begin()),
						std::make_move_iterator(statics().active_tasks.end())
					);
					statics().active_tasks.clear();
					// grab more tasks from pending task queue as needed
					while (possibly_active_tasks.size() < statics().max_active_tasks && !statics().pending_tasks.empty()) {
						possibly_active_tasks.push(statics().pending_tasks.pop());
					}
					// fill active task set
					while (statics().active_tasks.size() < statics().max_active_tasks && !possibly_active_tasks.empty()) {
						statics().active_tasks.push_back(possibly_active_tasks.pop());
					}
					// throw remaining tasks back into pending task queue after suspending them
					while (!possibly_active_tasks.empty()) {
						possibly_active_tasks.top()->suspend();
						statics().pending_tasks.push(possibly_active_tasks.pop());
					}
					// start or resume all active tasks
					for (auto &task : statics().active_tasks) {
						if (task->started()) {
							task->resume();
						} else {
							task->start(acquireWorker());
						}
					}
				}

			public:
				static void invoke(task_ptr task) {
					std::unique_lock<std::mutex> lock(statics().task_mutex);
					// add task to active set (force reschedule to consider it)
					statics().active_tasks.push_back(std::move(task));
					// ensure highest priority tasks are running
					reschedule();
				}

				static void finish(task_ptr task) {
					std::unique_lock<std::mutex> lock(statics().task_mutex);
					// remove task from active list
					auto it = std::find(statics().active_tasks.begin(), statics().active_tasks.end(), task);
					if (it != statics().active_tasks.end()) statics().active_tasks.erase(it);
					// reclaim worker
					releaseWorker(task->finish());
					// ensure highest priority tasks are running
					reschedule();
				}

				static bool shouldExit() {
					return statics().should_exit;
				}

				static size_t concurrency() {
					return statics().max_active_tasks;
				}

				static void concurrency(size_t x) {
					statics().max_active_tasks = x ? x : 1;
				}
			};
			
			void Worker::run(Worker *this_) {
				section_guard sec("AsyncWorker");

				Log::info() << "Worker started";

				// allow yield to find this worker
				currentWorker() = this_;

				while (true) {
					task_ptr task;

					do {
						// check for shutdown
						if (BackgroundScheduler::shouldExit()) {
							Log::info() << "Worker shutdown signaled";
							return;
						}
						// try grab next task
						unique_lock<mutex> lock(this_->m_mutex);
						swap(task, this_->m_task);
						if (!task) {
							// no task, block
							this_->m_resume_cond.wait(lock);
						}
					} while (!task);

					try {
						section_guard sec("Task");
						task->run();
					} catch (exception &e) {
						Log::error().verbosity(2) << "Task exceptioned; what(): " << e.what();
					} catch (...) {
						Log::error().verbosity(2) << "Task exceptioned";
					}

					// cleanup after task (possibly schedule another task to this worker)
					BackgroundScheduler::finish(move(task));

				}

			}

			class AffinitizedScheduler {
			private:
				struct Statics {
					mutex task_mutex;
					unordered_map<thread::id, util::priority_queue<task_ptr, task_compare>> pending_tasks;

					Statics() {
						Log::info("Async") << "Affinitized task scheduler initialized";
					}

					~Statics() {
						Log::info("Async") << "Affinitized task scheduler deinitialized";
					}
				};

				static auto & statics() {
					static Statics s;
					return s;
				}

			public:
				static void invoke(thread::id affinity, task_ptr task) {
					unique_lock<mutex> lock(statics().task_mutex);
					statics().pending_tasks[affinity].push(task);
				}

				static size_t execute(clock::duration timebudget) noexcept {
					size_t count = 0;
					const auto time1 = clock::now() + timebudget;
					while (clock::now() < time1) {
						task_ptr task;
						{
							// lock the mutex only long enough to grab a task
							unique_lock<mutex> lock(statics().task_mutex);
							auto &q = statics().pending_tasks[this_thread::get_id()];
							// quit if no tasks
							if (q.empty()) break;
							task = q.pop();
						}
						// execute task
						try {
							task->run();
							count++;
						} catch (exception &e) {
							Log::error().verbosity(2) << "Task exceptioned; what(): " << e.what();
						} catch (...) {
							Log::error().verbosity(2) << "Task exceptioned";
						}
					}
					return count;
				}
			};

		}

		void detail::invoke(std::thread::id affinity, clock::time_point deadline, detail::runnable_ptr taskfun) {
			if (affinity == thread::id()) {
				BackgroundScheduler::invoke(make_shared<Task>(move(deadline), move(taskfun)));
			} else {
				AffinitizedScheduler::invoke(affinity, make_shared<Task>(move(deadline), move(taskfun)));
			}
		}

		void concurrency(size_t x) {
			BackgroundScheduler::concurrency(x);
		}

		size_t concurrency() noexcept {
			return BackgroundScheduler::concurrency();
		}

		void yield() noexcept {
			if (Worker *worker = currentWorker()) {
				worker->yield();
			}
		}

		size_t execute(clock::duration timebudget) noexcept {
			return AffinitizedScheduler::execute(move(timebudget));
		}

	}

	size_t ConcurrentInit::refcount = 0;

	ConcurrentInit::ConcurrentInit() {
		if (refcount++ == 0) {
			// init main thread id
			mainThreadId();
			// init background scheduler
			async::invoke(chrono::seconds(0), []{});
			// init affinitized scheduler
			async::invoke(this_thread::get_id(), chrono::seconds(0), []{});
			async::execute(chrono::seconds(1));
		}
	}

	ConcurrentInit::~ConcurrentInit() {
		if (--refcount == 0) {
			// async statics about to be destroyed
			// nothing to do
		}
	}

}
