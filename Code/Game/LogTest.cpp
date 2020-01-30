#include "Engine/Develop/UnitTest.hpp"
#include "Engine/Develop/Log.hpp"
#include "Engine/Develop/Profile.hpp"
#define LOG_MESSAGES_PER_THREAD_TEST   (512)

static void LogTest()
{
	//PROFILE_SCOPE(__FUNCTION__);
	std::thread::id this_id = std::this_thread::get_id();
	size_t hash_id = std::hash<std::thread::id>{}(this_id);
	char const* format = "Thread[%I64u]: Printing Message %u";

	for (unsigned int i = 0; i < LOG_MESSAGES_PER_THREAD_TEST; ++i) {
		if (rand() % 2) {
			LogFilterEnable("debug");
		} else {
			LogFilterDisable("debug");
		}
		Log("debug", format, hash_id, i);
	}
}

UNIT_TEST(LogThreadTest, "System", 0)
{
	// leave one thread free (main thread)
	unsigned int core_count = std::thread::hardware_concurrency() - 2;
	LogFilterDisableAll();
	for (unsigned i = 0; i < core_count; ++i) {
		std::thread test_thread(LogTest);
		test_thread.detach();
	}
	return true;
}