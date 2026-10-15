#include <tasker/task.h>

#include "mocks.h"
#include "../test-helpers/helpers.h"

#include <memory>
#include <mt/event.h>
#include <mt/thread.h>
#include <stdexcept>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace tasker
{
	namespace tests
	{
		begin_test_suite( ATaskTests )
			mocks::queue queues[2];

			test( RunningATaskSchedulesOneOnTheQueuePassed )
			{
				// INIT
				vector<int> called(2);

				// INIT / ACT
				auto t1 = schedule_value_task([&] {	return called[0]++, 18;	}, queues[0]);
				auto t2 = schedule_value_task([&] {	return called[1]++, 181;	}, queues[1]);

				// ASSERT
				assert_equal(1u, queues[0].tasks.size());
				assert_equal(1u, queues[1].tasks.size());

				// ACT
				queues[0].run_one();

				// ASSERT
				assert_equal(plural + 1 + 0, called);
				assert_equal(0u, queues[0].tasks.size());
				assert_equal(1u, queues[1].tasks.size());

				// ACT
				queues[1].run_one();

				// ASSERT
				assert_equal(plural + 1 + 1, called);
				assert_equal(0u, queues[0].tasks.size());
				assert_equal(0u, queues[1].tasks.size());
			}


			test( ContinuationsAreScheduledWhenAntecedentIsComplete )
			{
				// INIT
				vector<int> order;

				// INIT / ACT
				schedule_value_task([&] {
					order.push_back(1);
					return 18;
				}, queues[0]).then([&] (async_result<int> &&) {
					order.push_back(2);
					return string("lorem ipsum");
				}, queues[0]).then([&] (async_result<string> &&) {
					order.push_back(3);
					return 123.45;
				}, queues[0]);

				// ASSERT
				assert_equal(1u, queues[0].tasks.size());
				assert_is_empty(order);

				// ACT
				queues[0].run_one();

				// ASSERT
				assert_equal(1u, queues[0].tasks.size());
				assert_equal(plural + 1, order);

				// ACT
				queues[0].run_one();

				// ASSERT
				assert_equal(1u, queues[0].tasks.size());
				assert_equal(plural + 1 + 2, order);

				// ACT
				queues[0].run_one();

				// ASSERT
				assert_is_empty(queues[0].tasks);
				assert_equal(plural + 1 + 2 + 3, order);
			}


			test( RootAndContinuationRunOnTheirQueuesAndTransferValueOwnership )
			{
				// INIT
				unique_ptr<string> obtained;
				auto root_called = false;

				// INIT / ACT
				auto next = schedule_value_task([&] { root_called = true; return string("lorem"); }, queues[0])
					.then([&] (async_result<string> &&result) {
						return unique_ptr<string>(new string(std::move(*result)));
					}, queues[1])
					.then([&] (async_result<unique_ptr<string>> &&result) { obtained = std::move(*result); }, queues[0]);

				// ASSERT
				assert_is_false(root_called);
				assert_is_empty(queues[1].tasks);
				assert_equal(1u, queues[0].tasks.size());

				// ACT
				queues[0].run_one();

				// ASSERT
				assert_is_true(root_called);
				assert_equal(1u, queues[1].tasks.size());
				assert_is_empty(queues[0].tasks);

				// ACT
				queues[1].run_one();

				// ASSERT
				assert_equal(1u, queues[0].tasks.size());
				assert_is_true(!obtained);

				// ACT
				queues[0].run_one();

				// ASSERT
				assert_equal("lorem", *obtained);
				assert_is_empty(queues[0].tasks);
			}


			test( RootExceptionIsDeliveredToValueContinuation )
			{
				// INIT
				auto caught = 0;

				// INIT / ACT
				schedule_value_task([] () -> int { throw 13; }, queues[0])
					.then([&] (async_result<int> &&result) {
						try
						{ *result; }
						catch (int value)
						{ caught = value; }
					}, queues[1]);

				// ACT
				queues[0].run_one();
				queues[1].run_one();

				// ASSERT
				assert_equal(13, caught);
				assert_is_empty(queues[0].tasks);
				assert_is_empty(queues[1].tasks);
			}


			test( ContinuationRegisteredAfterCompletionIsScheduled )
			{
				// INIT
				auto node = make_shared<task_node<int>>();
				task<int> task(node);
				auto obtained = 0;

				// INIT / ACT
				node->set(31);
				std::move(task).then([&] (async_result<int> &&result) { obtained = *std::move(result); }, queues[0]);

				// ASSERT
				assert_equal(1u, queues[0].tasks.size());

				// ACT
				queues[0].run_one();

				// ASSERT
				assert_equal(31, obtained);
				assert_is_empty(queues[0].tasks);
			}


			test( ConcurrentCompletionAndContinuationRegistrationScheduleOnce )
			{
				// INIT
				auto node = make_shared<task_node<int>>();
				task<int> task(node);
				mt::event start(false, false);
				auto obtained = 0;

				// ACT
				mt::thread setting([&] { start.wait(); node->set(47); });
				mt::thread registering([&] {
					start.wait();
					std::move(task).then([&] (async_result<int> &&result) { obtained = *std::move(result); }, queues[0]);
				});
				start.set();
				setting.join();
				registering.join();

				// ASSERT
				assert_equal(1u, queues[0].tasks.size());

				// ACT
				queues[0].run_one();

				// ASSERT
				assert_equal(47, obtained);
				assert_is_empty(queues[0].tasks);
			}


			test( VoidTasksAndContinuationExceptionsPropagateThroughValueChain )
			{
				// INIT
				auto step = 0;

				// INIT / ACT
				schedule_value_task([&] { step = 17; }, queues[0])
					.then([&] (async_result<void> &&result) { *std::move(result); step = 11; return 17; }, queues[1])
					.then([] (async_result<int> &&) -> string { throw 19; }, queues[0])
					.then([&] (async_result<string> &&result) {
						try
						{ *std::move(result); }
						catch (int value)
						{ step = value; }
					}, queues[1]);

				// ACT
				queues[0].run_till_end();
				queues[1].run_till_end();
				queues[0].run_till_end();
				queues[1].run_till_end();

				// ASSERT
				assert_equal(19, step);
				assert_is_empty(queues[0].tasks);
				assert_is_empty(queues[1].tasks);
			}


			test( OnlyOneContinuationCanConsumeAResult )
			{
				// INIT
				auto node = make_shared<task_node<int>>();
				task<int> first(node);
				task<int> second(node);

				// ACT
				std::move(first).then([] (async_result<int> &&) { }, queues[0]);
				try
				{ std::move(second).then([] (async_result<int> &&) { }, queues[0]); }
				catch (const logic_error &)
				{
					// ASSERT
					assert_is_true(true);
					return;
				}

				// ASSERT
				assert_is_true(false);
			}


			test( UnwrappingANestedTaskCreatesATaskTriggeredWhenNestedIsComplete )
			{
				// INIT
				vector<int> results1;
				auto called2 = 0;
				auto s1 = make_shared< task_node<int> >();
				auto s2 = make_shared< task_node<void> >();

				// ACT
				schedule_value_task([&] {	return task<int>(s1);	}, queues[0])
					.unwrap()
					.then([&] (async_result<int> &&r) {	results1.push_back(*r);	}, queues[1]);
				schedule_value_task([&] {	return task<void>(s2);	}, queues[0])
					.unwrap()
					.then([&] (async_result<void> &&)  {	called2++;	}, queues[1]);

				// ASSERT
				assert_equal(2u, queues[0].tasks.size());
				assert_is_empty(queues[1].tasks);

				// ACT
				queues[0].run_one();
				queues[0].run_one();

				// ASSERT
				assert_equal(0u, queues[0].tasks.size());
				assert_is_empty(queues[1].tasks);

				// ACT
				s1->set(171321);

				// ASSERT
				assert_is_empty(queues[0].tasks);
				assert_equal(1u, queues[1].tasks.size());
				assert_is_empty(results1);
				assert_equal(0, called2);

				// ACT
				s2->set();

				// ASSERT
				assert_is_empty(queues[0].tasks);
				assert_equal(2u, queues[1].tasks.size());
				assert_is_empty(results1);
				assert_equal(0, called2);

				// ACT
				queues[1].run_one();

				// ASSERT
				assert_equal(1u, queues[1].tasks.size());
				assert_equal(plural + 171321, results1);
				assert_equal(0, called2);

				// ACT
				queues[1].run_one();

				// ASSERT
				assert_is_empty(queues[0].tasks);
				assert_is_empty(queues[1].tasks);
				assert_equal(plural + 171321, results1);
				assert_equal(1, called2);
			}

		end_test_suite
	}
}