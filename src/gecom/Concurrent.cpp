
#include <vector>
#include <unordered_map>

#include "Concurrent.hpp"
#include "Log.hpp"
#include "Util.hpp"

using namespace std;
using namespace gecom;

namespace gecom {

	namespace async {

		namespace {

			class Task;
			using task_ptr = shared_ptr<Task>;

			class Worker;
			using worker_ptr = shared_ptr<Worker>;

			// 
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

				void run();

				void resume();

				void suspend();

				~Task();
			};

			struct task_compare {
				bool operator()(const task_ptr &a, const task_ptr &b) {
					return a->deadline() > b->deadline();
				}
			};

			class Worker : public enable_shared_from_this<Worker> {
			private:
				static thread_local Worker *current_worker;
				static mutex pool_mutex;
				static vector<worker_ptr> pool;

				mutex m_mutex;
				condition_variable m_resume_cond;
				task_ptr m_task;
				atomic<bool> m_should_yield { true };
				thread m_thread;

				static void run(Worker *);

			public:
				Worker() {
					section_guard sec("async");
					Log::info() << "worker starting...";
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
					section_guard sec("async");
					// wake up the thread so it can exit
					m_resume_cond.notify_all();
					m_thread.join();
					Log::info() << "worker terminated";
				}

				static Worker * current() noexcept {
					return current_worker;
				}

				static worker_ptr acquire() {
					unique_lock<mutex> lock(pool_mutex);
					if (pool.empty()) {
						return make_shared<Worker>();
					} else {
						auto r = pool.back();
						pool.pop_back();
						return r;
					}
				}

				static void release(worker_ptr p) {
					unique_lock<mutex> lock(pool_mutex);
					pool.push_back(move(p));
				}
			};

			
			// background task global state
			mutex bg_task_mutex;
			util::priority_queue<task_ptr, task_compare> bg_pending_tasks;
			vector<task_ptr> bg_active_tasks;
			size_t bg_max_active_tasks = 1;
			atomic<bool> should_exit { false };

			// thread-affinitized task global state
			mutex ta_task_mutex;
			unordered_map<thread::id, util::priority_queue<task_ptr, task_compare>> ta_pending_tasks;
			
			void Task::run() {
				m_runnable->run();
			}

			void Task::resume() {
				if (m_worker) {
					m_worker->resume();
				} else {
					m_worker = Worker::acquire();
					m_worker->employ(shared_from_this());
				}
			}

			void Task::suspend() {
				if (m_worker) {
					m_worker->suspend();
				}
			}

			Task::~Task() {
				if (m_worker) {
					m_worker->suspend();
					Worker::release(std::move(m_worker));
				}
			}

			void rescheduleBackground() {
				// bg_task_mutex assumed owned
				// move currently active tasks to possibly active task set
				util::priority_queue<task_ptr, task_compare> possibly_active_tasks(
					std::make_move_iterator(bg_active_tasks.begin()),
					std::make_move_iterator(bg_active_tasks.end())
				);
				bg_active_tasks.clear();
				// grab more tasks from pending task queue as needed
				while (possibly_active_tasks.size() < bg_max_active_tasks && !bg_pending_tasks.empty()) {
					possibly_active_tasks.push(bg_pending_tasks.pop());
				}
				// fill active task set
				while (bg_active_tasks.size() < bg_max_active_tasks && !possibly_active_tasks.empty()) {
					bg_active_tasks.push_back(possibly_active_tasks.pop());
				}
				// throw remaining tasks back into pending task queue after suspending them
				while (!possibly_active_tasks.empty()) {
					possibly_active_tasks.top()->suspend();
					bg_pending_tasks.push(possibly_active_tasks.pop());
				}
				// resume all active tasks
				for (auto &task : bg_active_tasks) {
					task->resume();
				}
			}

			void invokeBackground(task_ptr task) {
				std::unique_lock<std::mutex> lock(bg_task_mutex);
				// add task to active set (force reschedule to consider it)
				bg_active_tasks.push_back(std::move(task));
				// ensure highest priority tasks are running
				rescheduleBackground();
			}

			void finishBackground(task_ptr task) {
				std::unique_lock<std::mutex> lock(bg_task_mutex);
				// remove task from active list
				auto it = std::find(bg_active_tasks.begin(), bg_active_tasks.end(), task);
				if (it != bg_active_tasks.end()) bg_active_tasks.erase(it);
				// 'delete' task so the worker is released
				task = nullptr;
				// ensure highest priority tasks are running
				rescheduleBackground();
			}
			
			void Worker::run(Worker *this_) {
				section_guard sec("async");

				Log::info() << "worker started";

				// allow yield to find this worker
				current_worker = this_;

				while (true) {
					task_ptr task;

					do {
						// check for shutdown
						if (should_exit) return;
						// try grab next task
						unique_lock<mutex> lock(this_->m_mutex);
						swap(task, this_->m_task);
						if (!task) {
							// no task, block
							this_->m_resume_cond.wait(lock);
						}
					} while (!task);

					try {
						task->run();
					} catch (exception &e) {
						Log::error().verbosity(2) << "task exceptioned; what(): " << e.what();
					} catch (...) {
						Log::error().verbosity(2) << "task exceptioned";
					}

					// cleanup after task (possibly schedule another task to this worker)
					finishBackground(move(task));

				}

			}

			void invokeAffinitized(thread::id affinity, task_ptr task) {
				unique_lock<mutex> lock(ta_task_mutex);
				ta_pending_tasks[affinity].push(task);
			}

			// Worker statics
			thread_local Worker *Worker::current_worker { nullptr };
			mutex Worker::pool_mutex;
			vector<worker_ptr> Worker::pool;

			class AsyncInit {
			public:
				AsyncInit() {
					// start a worker
					async::invoke(chrono::seconds(0), []{});
				}

				~AsyncInit() {
					// signal workers should exit; worker dtor does the rest
					should_exit = true;
				}
			};

			AsyncInit async_init_obj;

			// id of main thread
			thread::id main_thread_id { this_thread::get_id() };
		}


		void detail::invoke(std::thread::id affinity, clock::time_point deadline, detail::runnable_ptr taskfun) {
			if (affinity == thread::id()) {
				invokeBackground(make_shared<Task>(move(deadline), move(taskfun)));
			} else {
				invokeAffinitized(affinity, make_shared<Task>(move(deadline), move(taskfun)));
			}
		}

		thread::id mainThreadId() noexcept {
			return main_thread_id;
		}

		void concurrency(size_t x) {
			bg_max_active_tasks = x ? x : 1;
		}

		size_t concurrency() noexcept {
			return bg_max_active_tasks;
		}

		void yield() noexcept {
			if (Worker *worker = Worker::current()) {
				worker->yield();
			}
		}

		size_t execute(clock::duration timebudget) noexcept {
			size_t count = 0;
			const auto time1 = clock::now() + timebudget;
			while (clock::now() < time1) {
				task_ptr task;
				{
					// lock the mutex only long enough to grab a task
					unique_lock<mutex> lock(ta_task_mutex);
					auto &q = ta_pending_tasks[this_thread::get_id()];
					// quit if no tasks
					if (q.empty()) break;
					task = q.pop();
				}
				// execute task
				try {
					task->run();
					count++;
				} catch (exception &e) {
					Log::error().verbosity(2) << "task exceptioned; what(): " << e.what();
				} catch (...) {
					Log::error().verbosity(2) << "task exceptioned";
				}
			}
			return count;
		}

		
	}

}
