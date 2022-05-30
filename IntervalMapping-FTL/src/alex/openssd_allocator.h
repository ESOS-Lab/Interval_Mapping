/*
 * openssd_allocator.h
 *
 *  Created on: 2021. 2. 17.
 *      Author: Minsu Jang
 */

#ifndef SRC_ALEX_OPENSSD_ALLOCATOR_H_
#define SRC_ALEX_OPENSSD_ALLOCATOR_H_

#include <xil_printf.h>
#include "../smalloc/smalloc.h"

extern const void* allocator_start_addr;
extern const void* allocator_end_addr;
extern void* memAddr;

template<class T>
class OpenSSDAllocator {
public:
	typedef T value_type;

	constexpr OpenSSDAllocator() noexcept {
	}

	constexpr OpenSSDAllocator(const OpenSSDAllocator &) noexcept = default;
	template<class _Other>
	constexpr OpenSSDAllocator(const OpenSSDAllocator<_Other> &) noexcept {
	}

	T *allocate(std::size_t n) {
//		if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
//			throw std::bad_alloc();

		// if (!smalloc_curr_pool.pool) {
		// 	size_t size = (size_t)allocator_end_addr - (size_t)allocator_start_addr;
		// 	sm_set_default_pool((void*)allocator_start_addr, size, 0, 0);

		// }

		// xil_printf("trying to allocate size=%p\n", n * sizeof(T));
		if ((unsigned int)memAddr % 4 > 0) {
			memAddr = (void*)((unsigned int) memAddr - (unsigned int)memAddr % 4 + 4);
		}
		T* p = static_cast<T *>(memAddr);
		memAddr += n * sizeof(T);
		// T* p = static_cast<T *>(sm_malloc(n * sizeof(T)));

		// xil_printf("allocate %p, size=%p\n", p, n * sizeof(T));
//		 report(p, n, 1);

		if (memAddr >= allocator_end_addr)
			throw std::bad_alloc();
		return p;
	}

	void deallocate(T *p, std::size_t n) noexcept
	{
		//  xil_printf("deallocate %p, size=%p\n", p, n * sizeof(T));
		//  sm_free(p);
//		 report(p, n, 0);
//		std::free(p);
	}

	void destroy(T *p) {
		p->~T();
	}

	template<class _Other>
	struct rebind {
		typedef OpenSSDAllocator<_Other> other;
	};

private:
//	 void report(T *p, std::size_t n, bool alloc = true) const
//	 {
//	     std::cout << (alloc ? "Alloc: " : "Dealloc: ") << sizeof(T) * n
//	               << " bytes at " << std::hex << std::showbase
//	               << reinterpret_cast<void *>(p) << std::dec << '\n';
//	 }
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
