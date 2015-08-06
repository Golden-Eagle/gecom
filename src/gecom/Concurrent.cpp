#include <vector>

#include "Concurrent.hpp"
#include "Log.hpp"

using namespace std;

namespace gecom {

	void async::detail::invoke(std::thread::id affinity, clock::time_point deadline, std::function<void()> taskfun) {

	}

	void async::concurrency(size_t) {

	}

	size_t async::concurrency() {
		return 0;
	}

	void async::yield() {

	}

	void async::execute(clock::duration timebudget) {

	}

}
