#include <tasker/task_algorithm.h>

#include "mocks.h"

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace tasker
{
	namespace tests
	{
		namespace
		{
			template <typename T>
			using result_ref = const async_result<T> &;
		}

		begin_test_suite( TaskAlgorithmLoopTests )
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
					t->set(std::move(false));
					return task<bool>(std::move(t));
				}, q).then([&] (result_ref<void> r) {
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

					t->set(std::move(--times > 0u));
					return task<bool>(std::move(t));
				}, q).then([&] (result_ref<void> r) {
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
					return task<bool>(std::move(t));
				}, q).then([&] (result_ref<void> r) {
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

		begin_test_suite( TaskAlgorithmWhenAllTests )
			template <typename T>
			static task<T> from_node(const shared_ptr<task_node<T>> &from)
			{	return task<T>(typename task_node<T>::ptr(from));	}

			test( ContinuationIsScheduledImmediatellyForSingleAntecedent )
			{
				// INIT
				mocks::queue q;
				const auto t_void = make_shared<task_node<void>>();
				const auto t_int = make_shared<task_node<int>>();
				const auto t_string = make_shared<task_node<string>>();
				auto invoked = false;

				// INIT / ACT
				task<string> t1 = when_all([&] (result_ref<void> /*r*/) {
					invoked = true;
					return string();
				}, q, from_node(t_void));
				task<void> t2 = when_all([&] (result_ref<int> /*r*/) {
					invoked = true;
				}, q, from_node(t_int));
				task<int> t3 = when_all([&] (result_ref<string> /*r*/) {
					invoked = true;
					return 123;
				}, q, from_node(t_string));

				// ACT
				t_void->set();

				// ASSERT
				assert_equal(1u, q.tasks.size());

				// ACT
				t_int->set(3141);
				t_string->set("lorem ipsum");

				// ASSERT
				assert_equal(3u, q.tasks.size());
				assert_is_false(invoked);
			}


			test( ContinuationIsExecutedWithExpectedResult )
			{
				// INIT
				mocks::queue q;
				const auto t_int = make_shared<task_node<int>>();
				const auto t_string = make_shared<task_node<string>>();
				int iresult = 0;
				string sresult;

				auto t1 = when_all([&] (result_ref<int> r) {
					iresult = *r;
				}, q, from_node(t_int));
				auto t2 = when_all([&] (result_ref<string> r) {
					sresult = *r;
					return 123;
				}, q, from_node(t_string));
				t_int->set(314159);
				t_string->set("lorem ipsum");

				// ACT
				q.run_one();
				q.run_one();

				// ASSERT
				assert_equal(314159, iresult);
				assert_equal("lorem ipsum", sresult);
				assert_equal(0u, q.tasks.size());

				// ACT
				t2.then([&] (const async_result<int> &r) {	iresult = *r;	}, immediate);

				// ASSERT
				assert_equal(123, iresult);
			}


			test( ContinuationIsOnlyScheduledWhenAllAntecedentTasksAreComplete )
			{
				// INIT
				mocks::queue q;
				const auto t_void = make_shared<task_node<void>>();
				const auto t_int = make_shared<task_node<int>>();
				const auto t_string = make_shared<task_node<string>>();
				auto invoked = false;
				int iresult = 0;
				string sresult;

				// INIT / ACT
				task<string> t1 = when_all([&] (result_ref<void> r1, result_ref<int> r2, result_ref<string> r3) {
					invoked = true;
					*r1;
					iresult = *r2;
					sresult = *r3;
					return string("lorem ipsum amet dolor");
				}, q, from_node(t_void), from_node(t_int), from_node(t_string));

				// ACT
				t_void->set();
				t_string->set("Lorem ipsum");

				// ASSERT
				assert_equal(0u, q.tasks.size());

				// ACT
				t_int->set(314);

				// ASSERT
				assert_equal(1u, q.tasks.size());
				assert_is_false(invoked);

				// ACT
				q.run_one();

				// ASSERT
				assert_is_empty(q.tasks);
				assert_is_true(invoked);
				assert_equal(314, iresult);
				assert_equal("Lorem ipsum", sresult);
			}
		end_test_suite
	}
}
