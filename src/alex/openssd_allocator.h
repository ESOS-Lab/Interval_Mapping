/*
 * openssd_allocator.h
 *
 *  Created on: 2021. 2. 17.
 *      Author: Minsu Jang
 */

#ifndef SRC_ALEX_OPENSSD_ALLOCATOR_H_
#define SRC_ALEX_OPENSSD_ALLOCATOR_H_

extern const void* allocator_start_addr;
extern const void* allocator_end_addr;
extern void* memAddr;

template<class T>
class OpenSSDAllocator {
	typedef T value_type;

public:

	constexpr OpenSSDAllocator() noexcept {
	}

	constexpr OpenSSDAllocator(const OpenSSDAllocator &) noexcept = default;
	template<class _Other>
	constexpr OpenSSDAllocator(const OpenSSDAllocator<_Other> &) noexcept {
	}

	[[nodiscard]]
	T *allocate(std::size_t n) {
		if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
			throw std::bad_alloc();

		T* p = static_cast<T *>(memAddr);
		memAddr += sizeof(n * sizeof(T));

		if (memAddr >= allocator_end_addr)
			throw std::bad_alloc();
		return p;
	}

	void deallocate(T *p, std::size_t n) noexcept
	{
		// report(p, n, 0);
		std::free(p);
	}

	void destroy(T *p) {
		p->~T();
	}

	template<class _Other>
	struct rebind {
		typedef OpenSSDAllocator<_Other> other;
	};

private:
	// void report(T *p, std::size_t n, bool alloc = true) const
	// {
	//     std::cout << (alloc ? "Alloc: " : "Dealloc: ") << sizeof(T) * n
	//               << " bytes at " << std::hex << std::showbase
	//               << reinterpret_cast<void *>(p) << std::dec << '\n';
	// }
};

template<class T, class U>
bool operator==(const OpenSSDAllocator<T> &, const OpenSSDAllocator<U> &) {
	return true;
}
template<class T, class U>
bool operator!=(const OpenSSDAllocator<T> &, const OpenSSDAllocator<U> &) {
	return false;
}

#endif /* SRC_ALEX_OPENSSD_ALLOCATOR_H_ */
