#include "helpers.h"

#include <windows.h>

using namespace std;

namespace tasker
{
	namespace tests
	{
		class message_loop_win32 : public message_loop
		{
		public:
			message_loop_win32()
				: _thread_id(::GetCurrentThreadId())
			{	}

			virtual void run() override
			{
				MSG msg = {};

				while (::GetMessage(&msg, NULL, 0, 0) && WM_QUIT != msg.message)
					::DispatchMessage(&msg);
			}

			virtual void exit() override
			{	::PostThreadMessage(_thread_id, WM_QUIT, 0, 0);	}

		private:
			unsigned _thread_id;
		};


		mt::milliseconds clock()
		{
			LARGE_INTEGER v;
			static const auto period = (::QueryPerformanceFrequency(&v), 1000.0 / v.QuadPart);
			static const auto initial = (::QueryPerformanceCounter(&v), v.QuadPart);

			::QueryPerformanceCounter(&v);
			return mt::milliseconds(static_cast<long long>(period * (v.QuadPart - initial)));
		}

		function<mt::milliseconds ()> create_stopwatch()
		{
			const auto previous = make_shared<mt::milliseconds>(clock());

			return [previous] () -> mt::milliseconds {
				const auto now = clock();
				const auto d = now - *previous;

				return *previous = now, d;
			};
		}

		shared_ptr<message_loop> message_loop::create()
		{	return make_shared<message_loop_win32>();	}

		function<mt::milliseconds()> get_clock()
		{	return &clock;	}
	}
}
