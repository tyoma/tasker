#include "../thread.h"

#include <cstdint>
#include <windows.h>

using namespace std;

namespace tasker
{
	namespace tests
	{
		namespace
		{
			mt::milliseconds to_milliseconds(const FILETIME &ftime)
			{	return mt::milliseconds(((static_cast<uint64_t>(ftime.dwHighDateTime) << 32) + ftime.dwLowDateTime) / 10000);	}
		}

		unsigned int this_thread::get_native_id()
		{	return ::GetCurrentThreadId();	}

		mt::milliseconds this_thread::get_cpu_time()
		{
			FILETIME dummy, user = {}, kernel = {};

			::GetThreadTimes(::GetCurrentThread(), &dummy, &dummy, &kernel, &user);
			return to_milliseconds(user);
		}
	}
}
