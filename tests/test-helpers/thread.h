#pragma once

#include <memory>
#include <mt/event.h>

namespace tasker
{
	namespace tests
	{
		namespace this_thread
		{
			std::shared_ptr<mt::event> open();
			unsigned int get_native_id();
			mt::milliseconds get_cpu_time();
		}
	}
}
