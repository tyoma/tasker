#include "helpers.h"

using namespace std;

namespace tasker
{
	namespace tests
	{
		mt::milliseconds clock()
		{	return mt::milliseconds(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());	}

		function<mt::milliseconds ()> create_stopwatch()
		{
			const auto previous = make_shared<mt::milliseconds>(clock());

			return [previous] () -> mt::milliseconds {
				const auto now = clock();
				const auto d = now - *previous;

				return *previous = now, d;
			};
		}
	}
}
