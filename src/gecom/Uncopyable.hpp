#ifndef GECOM_UNCOPYABLE_HPP
#define GECOM_UNCOPYABLE_HPP

namespace gecom {

	// inherit (privately) from this to default-delete copy and move ops
	class Uncopyable {
	private:
		Uncopyable(const Uncopyable &rhs) = delete;
		Uncopyable & operator=(const Uncopyable &rhs) = delete;
	protected:
		Uncopyable() { }
	};

}

#endif
