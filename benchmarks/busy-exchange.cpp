#include <tasker/scheduler.h>
#include <tasker/thread_queue.h>
#include <iterator>
#include <iostream>
#include <mt/event.h>
#include <mt/mutex.h>
#include <string>
#include <time.h>

using namespace std;
using namespace tasker;

int main()
{
	thread_queue q1([] {	return mt::milliseconds(0); }), q2([] {	return mt::milliseconds(0); });
	vector<int> results(1000000);
	auto p = begin(results);
	mt::event complete;
	string s;

	cout << "Press any key to start!" << endl;
	cin >> s;

	auto start = clock();

	for (auto n = results.size(); n--; )
	{
		q1.schedule([&] {
			auto u = 1031294;
			auto v = 1322;

			q2.schedule([&, u, v] {
				auto q = u / v;

				q1.schedule([&, q] {
					*p++ = q;
					q2.schedule([] {}, mt::milliseconds(0));
					if (end(results) == p)
						complete.set();
				}, mt::milliseconds(0));
			}, mt::milliseconds(0));
		}, mt::milliseconds(0));
	}

	auto scheduled_at = clock();

	mt::mutex mtx;

	complete.wait();

	auto complete_at = clock();

	cout << "Scheduling per item: " << 1.0 * (scheduled_at - start) / CLOCKS_PER_SEC / results.size() << endl;
	cout << "Execution per item: " << 1.0 * (complete_at - scheduled_at) / CLOCKS_PER_SEC / results.size() << endl;

	return 0;
}
