#include <tasker/task_algorithm.h>

#include "mocks.h"

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace tasker
{
	namespace tests
	{
		begin_test_suite( TaskAlgorithmTests )
			test( LooperRunsTheBodyOnesThenExits )
			{
				// INIT
				mocks::queue q, q2;
				auto called = 0u;
				auto complete = false;

				// INIT / ACT
				loop([&] () -> task<bool> {
					auto t = make_shared< task_node<bool> >();

					called++;
					t->set(move(false));
					return task<bool>(move(t));
				}, q).then([&] (const async_result<void> &r) {
					*r; // no exception
					complete = true;
				}, q2);

				// ASSERT
				assert_equal(1u, q.tasks.size());
				assert_equal(0u, called);

				// ACT
				q.run_one(); // body

				// ASSERT
				assert_equal(1u, q.tasks.size());
				assert_equal(1u, called);
				assert_is_empty(q2.tasks);

				// ACT
				q.run_one(); // condition

				// ASSERT
				assert_is_empty(q.tasks);
				assert_equal(1u, called);
				assert_is_false(complete);
				assert_equal(1u, q2.tasks.size());

				// ACT
				q2.run_one();

				// ASSERT
				assert_is_true(complete);
		}


			test( LooperRunsTheBodyUntilItReturnsFalse )
			{
				// INIT
				mocks::queue q, q2;
				auto times = 3u;
				auto complete = false;

				// ACT
				loop([&] () -> task<bool> {
					auto t = make_shared< task_node<bool> >();

					t->set(move(--times > 0u));
					return task<bool>(move(t));
				}, q).then([&] (const async_result<void> &r) {
					*r; // no exception
					complete = true;
				}, q2);

				// ASSERT
				assert_equal(1u, q.tasks.size());

				// ACT
				q.run_one(); // body

				// ASSERT
				assert_equal(2u, times);
				assert_equal(1u, q.tasks.size());

				// ACT
				q.run_one(); // condition

				// ASSERT
				assert_equal(2u, times);
				assert_equal(1u, q.tasks.size());

				// ACT
				q.run_one(); // body

				// ASSERT
				assert_equal(1u, times);
				assert_equal(1u, q.tasks.size());

				// ACT
				q.run_one(); // condition

				// ASSERT
				assert_equal(1u, times);
				assert_equal(1u, q.tasks.size());

				// ACT
				q.run_one(); // body

				// ASSERT
				assert_equal(0u, times);
				assert_equal(1u, q.tasks.size());
				assert_is_empty(q2.tasks);

				// ACT
				q.run_one(); // condition

				// ASSERT
				assert_is_empty(q.tasks);
				assert_equal(1u, q2.tasks.size());
				assert_is_false(complete);

				// ACT
				q2.run_one();

				// ASSERT
				assert_is_empty(q.tasks);
				assert_is_empty(q2.tasks);
				assert_is_true(complete);
			}


			test( LoopExitsIfABodyThrowsAnException )
			{
				// INIT
				mocks::queue q;
				auto complete = false;

				// ACT
				loop([&] () -> task<bool> {
					auto t = make_shared< task_node<bool> >();

					t->fail(runtime_error(""));
					return task<bool>(move(t));
				}, q).then([&] (const async_result<void> &r) {
					try
					{
						*r;
					}
					catch (runtime_error &)
					{
						complete = true;
					}
				}, q);

				q.run_till_end();

				// ASSERT
				assert_is_true(complete);
			}

		end_test_suite
	}
}
