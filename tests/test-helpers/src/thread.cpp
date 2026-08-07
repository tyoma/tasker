#include "../thread.h"

#include <mt/event.h>
#include <mt/thread_callbacks.h>

using namespace std;

namespace tasker
{
	namespace tests
	{
		shared_ptr<mt::event> this_thread::open()
		{
			auto exited = make_shared<mt::event>(false, false);

			mt::get_thread_callbacks().at_thread_exit([exited] {	exited->set();	});
			return exited;
		}
	}
}
