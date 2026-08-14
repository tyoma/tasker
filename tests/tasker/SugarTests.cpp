#include <tasker/sugar.h>

#include "../test-helpers/helpers.h"
#include "mocks.h"

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace no_a_tasker // Made intentionally to verify Koenig lookup is working.
{
	namespace tests
	{
		using tasker::async_result;
		using tasker::immediate;
		using tasker::next;
		using tasker::task;
		using tasker::task_node;

		using tasker::tests::plural;

		namespace mocks
		{
			using tasker::tests::mocks::queue;
		}

		template <typename T>
		inline vector<T> operator +(vector<T> lhs, const T& rhs)
		{	return lhs.push_back(rhs), lhs;	}

		begin_test_suite( SugarTests )
			test( RightShiftInvokesThenAndReturnsItsTaskImmediately )
			{
				// INIT
				auto called = 0;
				auto completion_v = make_shared<task_node<void>>();

				completion_v->set();
				task<void> completed_v_task(std::move(completion_v));

				// ACT
				task<void> t1 = completed_v_task >> next([&] (const async_result<void> &) {
					++called;
				}, immediate);

				// ASSERT
				assert_equal(1, called);

				// ACT
				task<double> t2 = completed_v_task >> next([&](const async_result<void> &) {
					++called;
					return 1.14;
				}, immediate);

				// ASSERT
				auto v = 0.0;

				assert_equal(2, called);

				t2.then([&](const async_result<double>& r) {	v = *r;	}, immediate);
				assert_equal(1.14, v);
			}


			test( RightShiftInvokesThenAndReturnsItsTaskWithQueue )
			{
				// INIT
				auto called = 0;
				auto completion_v = make_shared<task_node<void>>();
				mocks::queue q;

				completion_v->set();
				task<void> completed_v_task(std::move(completion_v));

				// ACT
				completed_v_task >> next([&] (const async_result<void> &) {
					++called;
				}, q);

				// ASSERT
				assert_equal(1u, q.tasks.size());
				assert_equal(0, called);

				// ACT
				q.run_one();

				// ASSERT
				assert_equal(1, called);
			}


			test( DivideOperatorConstructsContinuationPair )
			{
				// INIT
				auto called_int = 0;
				string called_str;
				mocks::queue q;

				// INIT / ACT
				get<0>([&] (int v) {	called_int = v;	} / immediate)(3141);
				get<0>([&] (const char *v) {	called_str = v;	} / immediate)("lorem ips");

				// ASSERT
				assert_equal(3141, called_int);
				assert_equal("lorem ips", called_str);

				// ACT / ASSERT
				assert_equal(&immediate, &get<1>([] {} / immediate));
				assert_equal(&q, &get<1>([] {} / q));
			}


			test( VerifyTheChainIsExecutedAsSpecified )
			{
				// INIT
				mocks::queue q1, q2;
				vector<int> log;
				auto completion_v = make_shared<task_node<void>>();

				completion_v->set();
				task<void> completed_v_task(std::move(completion_v));

				// ACT
				completed_v_task
					>> [&] (const async_result<void> &) {
						log.push_back(1);
						return 71;
					} / q1
					>> [&] (const async_result<int> &r) {
						log.push_back(*r);
					} / q2
					>> [&] (const async_result<void> &) {
						log.push_back(3);
						return 91.1;
					} / immediate
					>> [&] (const async_result<double> &r) {
						log.push_back(static_cast<int>(*r));
					} / q1;

				// ASSERT
				assert_equal(1u, q1.tasks.size());
				assert_equal(0u, q2.tasks.size());

				// ACT
				q1.run_one();

				// ASSERT
				assert_equal(plural + 1, log);
				assert_equal(0u, q1.tasks.size());
				assert_equal(1u, q2.tasks.size());

				// ACT
				q2.run_one();

				// ASSERT
				assert_equal(plural + 1 + 71 + 3, log);
				assert_equal(1u, q1.tasks.size());
				assert_equal(0u, q2.tasks.size());

				// ACT
				q1.run_one();

				// ASSERT
				assert_equal(plural + 1 + 71 + 3 + 91, log);
				assert_equal(0u, q1.tasks.size());
				assert_equal(0u, q2.tasks.size());
			}


			//test( BareCallbacksAreInvokedAsImmediateContinuations )
			//{
			//	// INIT
			//	auto completion_v = make_shared<task_node<void>>();
			//	auto completion_str = make_shared<task_node<string>>();
			//	auto c_v = completion_v;
			//	auto c_str = completion_str;
			//	auto called = 0;
			//	string called_str;

			//	// ACT
			//	task<void> t1 = task<void>(std::move(completion_v)) >> [&] (const async_result<void> &) {
			//		called++;
			//	};
			//	task<int> t2 = task<string>(std::move(completion_str)) >> [&] (const async_result<string> &r) {
			//		called_str = *r;
			//		return 123;
			//	};

			//	// ASSERT
			//	assert_equal(0, called);
			//	assert_equal("", called_str);

			//	// ACT
			//	c_v->set();
			//	c_str->set("lorem ipsum dolor amet");

			//	// ASSERT
			//	assert_equal(1, called);
			//	assert_equal("lorem ipsum dolor amet", called_str);
			//}


			test( GoStartsTaskForLambdaAndProducesTaskOfItsResult )
			{
				// INIT
				auto called = 0;
				auto value = 0;

				// ACT
				task<void> t_void = tasker::go >> [&] {
					++called;
				} / immediate;

				// ASSERT
				assert_equal(1, called);

				// ACT
				task<int> t_int = tasker::go >> [&] {
					++called;
					return 42;
				} / immediate;

				// ASSERT
				t_int.then([&] (const async_result<int> &r) {
					value = *r;
				}, immediate);
				assert_equal(2, called);
				assert_equal(42, value);
			}


			test( GoSchedulesLambdaAndProducesTaskOfItsResult )
			{
				// INIT
				auto called = 0;
				mocks::queue q;

				// ACT
				tasker::go >> [&] {
					++called;
				} / q;

				// ASSERT
				assert_equal(0, called);
				assert_equal(1u, q.tasks.size());

				// ACT
				q.run_one();

				// ASSERT
				assert_equal(1, called);
				assert_equal(0u, q.tasks.size());
			}


			test( OperatorPlusCombinesTasksIntoATuple )
			{
				// INIT / ACT / ASSERT
				tuple<task<void>, task<string>> group1
					= tasker::go >> [] {} / immediate & tasker::go >> [] { return string(); } / immediate;
				tuple<task<void>, task<double>> group2
					= tasker::go >> [] {} / immediate & tasker::go >> [] { return 3.14; } / immediate;
				tuple<task<string>, task<void>, task<double>> group3
					= tasker::go >> [] { return string(); } / immediate & (tasker::go >> [] {} / immediate & tasker::go >> [] { return 3.14; } / immediate);
				tuple<task<string>, task<void>, task<double>> group4
					= (tasker::go >> [] { return string(); } / immediate & tasker::go >> [] {} / immediate) & tasker::go >> [] { return 3.14; } / immediate;
			}


			test( ContinuingATupleOfTasksHasWhenAllSemantics )
			{
				// INIT
				mocks::queue q;
				auto completion_v = make_shared<task_node<void>>();
				auto completion_str = make_shared<task_node<string>>();
				auto c_v = completion_v;
				auto c_str = completion_str;
				auto called = 0;
				string called_str;

				// ACT
				task<double> t = (task<void>(std::move(completion_v)) & task<string>(std::move(completion_str)))
					>> [&](const async_result<void> &, const async_result<string> &r) {
						called_str = *r;
						++called;
						return 1.17;
					} / q;

				// ASSERT
				assert_equal(0, called);
				assert_equal(0u, q.tasks.size());

				// ACT
				c_str->set("lorem ipsum amet");

				// ASSERT
				assert_equal(0, called);
				assert_equal(0u, q.tasks.size());

				// ACT
				c_v->set();

				// ASSERT
				assert_equal(0, called);
				assert_equal(1u, q.tasks.size());

				// ACT
				q.run_one();

				// ASSERT
				assert_equal(1, called);
				assert_equal(0u, q.tasks.size());
				assert_equal("lorem ipsum amet", called_str);
			}


			test( ContinuingAThreeTupleOfTasksInvokesWhenAllCallback )
			{
				// INIT
				auto called = 0;
				auto a = 0;
				string b;
				auto c = 0.0;

				// ACT
				(tasker::go >> [] { return 11; } / immediate
					& tasker::go >> [] { return string("lorem"); } / immediate
					& tasker::go >> [] { return 2.5; } / immediate)
					>> [&] (const async_result<int> &r1, const async_result<string> &r2, const async_result<double> &r3) {
						a = *r1;
						b = *r2;
						c = *r3;
						++called;
					} / immediate;

				// ASSERT
				assert_equal(1, called);
				assert_equal(11, a);
				assert_equal("lorem", b);
				assert_equal(2.5, c);
			}


			test( UnwrapModifierUnwrapsNestedTask )
			{
				// INIT
				auto value = 0;
				auto inner = make_shared<task_node<int>>();
				auto inner_copy = inner;

				// ACT
				tasker::go
					>> [&] {	return task<int>(std::move(inner));	} / immediate
					>> tasker::unwrap
					>> [&] (const async_result<int> &r) {	value = *r;	} / immediate;

				// ASSERT
				assert_equal(0, value);

				// ACT
				inner_copy->set(314);

				// ASSERT
				assert_equal(314, value);
			}

		end_test_suite
	}
}
