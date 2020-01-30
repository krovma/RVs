#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Develop/UnitTest.hpp"
#include "Engine/Core/AsyncQueue.hpp"
#include "Game/EngineBuildPreferences.hpp"
#include <atomic>
#include <chrono>

#include "Engine/Develop/Memory.hpp"
#include "Engine/Develop/Profile.hpp"

using uint = unsigned int;
using byte = unsigned char;
#define MEM_TRACKING_UNIT_TEST
#if defined(MEM_TRACKING) && defined(MEM_TRACKING_UNIT_TEST)
#define MEMTEST_ITER_PER_THREAD 1'000'000
#define MEMTEST_ALLOC_BYTE_SIZE 128

bool RandomCoinFlip() {
	return (rand() % 2) != 0;
}

static void AllocTest(AsyncQueue<void*>& mem_queue, std::atomic<uint>& running_count)
{
	for (uint i = 0; i < MEMTEST_ITER_PER_THREAD; ++i) {
		// (Random01() > .5f) or however your random functions look
		if (RandomCoinFlip()) {
			byte* ptr = (byte*)TrackedAlloc(MEMTEST_ALLOC_BYTE_SIZE);

			// just doing this to slow it down
			// (and later, to confirm memory didn't get currupted)
			for (uint j = 0; j < MEMTEST_ALLOC_BYTE_SIZE; ++j) {
				ptr[j] = (byte)j;
			}

			mem_queue.Push(ptr);
		}
		else {
			void* ptr;
			if (mem_queue.Pop(&ptr)) {
				TrackedFree(ptr);
			}
		}
	}

	// we're done; 
	--running_count;
}

// This test will only work if memory tracking is enabled
// otherwise the memory tracking just return 0;

UNIT_TEST(memoryTest, "memory", 1)
{
	// unittest assumes 
	size_t pre_allocations = GetLiveAllocationCount();

	{
		PROFILE_SCOPE("A02 Test");
		// scope so queue goes out of scope and we
		// get those allocations back; 
		AsyncQueue<void*> mem_queue;
		uint core_count = std::thread::hardware_concurrency();
		std::atomic<uint> live_count = core_count;

		// wpin up that many threads; 
		for (uint i = 0; i < core_count; ++i) {
			std::thread test_thread(AllocTest, std::ref(mem_queue), std::ref(live_count));
			test_thread.detach();
		}

		while (live_count.load() > 0) {
			// "ms" is a C++ custom literal equivalent 
			// for std::chrono::milliseconds(100)
			// https://en.cppreference.com/w/cpp/chrono/operator%22%22ms
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		void* ptr;
		while (mem_queue.Pop(&ptr)) {
			TrackedFree(ptr);
		}
	}

	// check we're back to where we started; 
	size_t post_allocations = GetLiveAllocationCount();

	// if done right, allocations at the start
	// should be allocations at the end; 
	return (pre_allocations == post_allocations);
}
#endif
