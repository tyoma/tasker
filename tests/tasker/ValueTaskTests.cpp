#include <tasker/value_task.h>

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
		begin_test_suite( ValueTaskTests )
			mocks::queue queues[2];

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
				auto node = make_shared<value_task_node<int>>();
				value_task<int> task((value_task_node<int>::ptr(node)));
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
				auto node = make_shared<value_task_node<int>>();
				value_task<int> task((value_task_node<int>::ptr(node)));
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
				auto node = make_shared<value_task_node<int>>();
				value_task<int> first((value_task_node<int>::ptr(node)));
				value_task<int> second((value_task_node<int>::ptr(node)));

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
		end_test_suite
	}
}