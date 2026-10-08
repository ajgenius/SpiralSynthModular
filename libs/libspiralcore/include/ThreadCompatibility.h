// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SSM_THREAD_COMPATIBILITY_H
#define SSM_THREAD_COMPATIBILITY_H
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stdexcept>
#if __cplusplus >= 201103L
#include <atomic>
#endif

namespace SSMCompat
{
#if __cplusplus >= 201103L
	typedef std::memory_order MemoryOrder;
	const MemoryOrder Relaxed = std::memory_order_relaxed;
	const MemoryOrder Acquire = std::memory_order_acquire;
	const MemoryOrder Release = std::memory_order_release;
	const MemoryOrder Sequential = std::memory_order_seq_cst;
#else
	enum MemoryOrder
	{
		Relaxed,
		Acquire,
		Release,
		Sequential
	};
#endif
	// The GCC 4.2 fallback uses full-barrier atomic operations, never a mutex
	// on the realtime path. New compilers retain the lighter ordered operations.
	template <class T> class Atomic
	{
#if __cplusplus >= 201103L
		std::atomic<T> value;
#else
		mutable T value;
#endif
		Atomic(const Atomic &);
		Atomic &operator=(const Atomic &);

	public:
		explicit Atomic(T initial = T()):
		    value(initial)
		{
		}

		T load(MemoryOrder order = Sequential) const
		{
#if __cplusplus >= 201103L
			return value.load(order);
#else
			return __sync_val_compare_and_swap(&value, T(), T());
#endif
		}

		void store(T next, MemoryOrder order = Sequential)
		{
#if __cplusplus >= 201103L
			value.store(next, order);
#else
			exchange(next);
#endif
		}

		T exchange(T next, MemoryOrder order = Sequential)
		{
#if __cplusplus >= 201103L
			return value.exchange(next, order);
#else
			T previous = load();
			for (;;)
			{
				T observed = __sync_val_compare_and_swap(&value, previous, next);
				if (observed == previous)
					return previous;
				previous = observed;
			}
#endif
		}

		T fetch_add(T amount, MemoryOrder order = Sequential)
		{
#if __cplusplus >= 201103L
			return value.fetch_add(amount, order);
#else
			return __sync_fetch_and_add(&value, amount);
#endif
		}
	};

	class Mutex
	{
		pthread_mutex_t mutex;
		Mutex(const Mutex &);
		Mutex &operator=(const Mutex &);

	public:
		Mutex()
		{
			if (pthread_mutex_init(&mutex, NULL))
				throw std::runtime_error("Cannot create mutex");
		}

		~Mutex()
		{
			pthread_mutex_destroy(&mutex);
		}

		void Lock()
		{
			pthread_mutex_lock(&mutex);
		}

		void Unlock()
		{
			pthread_mutex_unlock(&mutex);
		}
	};

	class Lock
	{
		Mutex &mutex;
		Lock(const Lock &);
		Lock &operator=(const Lock &);

	public:
		explicit Lock(Mutex &m):
		    mutex(m)
		{
			mutex.Lock();
		}

		~Lock()
		{
			mutex.Unlock();
		}
	};

	inline void SleepMilliseconds(unsigned milliseconds)
	{
		struct timespec delay;
		delay.tv_sec = milliseconds / 1000;
		delay.tv_nsec = (milliseconds % 1000) * 1000000;
		while (nanosleep(&delay, &delay) != 0 && errno == EINTR)
		{
		}
	}
}
#endif
