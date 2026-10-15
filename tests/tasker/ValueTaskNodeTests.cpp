#include <tasker/value_task_node.h>

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace tasker
{
	namespace tests
	{
		namespace mocks
		{
			template <typename T>
			struct continuation : tasker::continuation<async_result<T>>
			{
				continuation(const function<void (async_result<T> &&result)> &acceptor_ = [] (async_result<T> &&) {	})
					: called(0), acceptor(acceptor_)
				{	}

				virtual void begin(const std::shared_ptr<async_result<T>> &antecedent) override
				{
					acceptor(std::move(*antecedent));
					called++;
				}

				int called;
				const function<void (async_result<T> &&result)> acceptor;
			};

			template <>
			struct continuation<void> : tasker::continuation<async_result<void>>
			{
				continuation(const function<void (async_result<void> &&result)> &acceptor_ = [] (async_result<void> &&) {	})
					: called(0), acceptor(acceptor_)
				{	}

				virtual void begin(const std::shared_ptr<async_result<void>> &antecedent) override
				{
					acceptor(std::move(*antecedent));
					called++;
				}

				int called;
				const function<void (async_result<void> &&result)> acceptor;
			};
		}

		begin_test_suite( ValueTaskNodeTests )
			test( ContinuationAddedToPendingNodeIsNotExecuted )
			{
				// INIT
				auto n1 = make_shared<value_task_node<string>>();
				auto n2 = make_shared<value_task_node<void>>();
				auto c1 = make_shared<mocks::continuation<string>>();
				auto c2 = make_shared<mocks::continuation<void>>();

				// ACT
				n1->then(c1);
				n2->then(c2);

				// ASSERT 
				assert_equal(0, c1->called);
				assert_equal(0, c2->called);
				assert_equal(2, c1.use_count());
				assert_equal(2, c2.use_count());
			}


			test( SettingContinuationFailsOnConsequentAttempts )
			{
				// INIT
				auto n = make_shared<value_task_node<string>>();
				auto c = make_shared<mocks::continuation<string>>();

				n->then(c);

				// ACT / ASSERT
				assert_throws(n->then(c), logic_error);
				assert_equal(2, c.use_count());
			}


			test( SettingResultFailsOnConsequentAttempts )
			{
				// INIT
				auto n1 = make_shared<value_task_node<string>>();
				auto n2 = make_shared<value_task_node<void>>();

				// ACT / ASSERT (no exception)
				n1->set("first");
				n2->set();

				// ACT / ASSERT
				assert_throws(n1->set("second"), logic_error);
				assert_throws(n2->set(), logic_error);
			}


			test( SettingFailureFailsOnConsequentAttempts )
			{
				// INIT
				auto n1 = make_shared<value_task_node<string>>();
				auto n2 = make_shared<value_task_node<void>>();
				auto n3 = make_shared<value_task_node<string>>();
				auto n4 = make_shared<value_task_node<void>>();

				// ACT / ASSERT (no exception)
				n1->fail("first");
				n2->fail(123);
				n3->fail(make_exception_ptr(runtime_error("error")));
				n4->fail(make_exception_ptr(runtime_error("error")));

				// ACT / ASSERT
				assert_throws(n1->fail(make_exception_ptr(runtime_error("another error"))), logic_error);
				assert_throws(n2->fail(make_exception_ptr(runtime_error("another error"))), logic_error);
				assert_throws(n3->fail("second"), logic_error);
				assert_throws(n4->fail(456), logic_error);
			}


			test( SettingFailureAfterResultOrViceVersaFailsOnConsequentAttempts )
			{
				// INIT
				auto n1 = make_shared<value_task_node<string>>();
				auto n2 = make_shared<value_task_node<void>>();

				// ACT / ASSERT (no exception)
				n1->set("first");
				n2->fail(123);

				// ACT / ASSERT
				assert_throws(n1->fail(make_exception_ptr(runtime_error("another error"))), logic_error);
				assert_throws(n2->set(), logic_error);
			}


			test( AssigningContinuationAfterResultIsSetDeliversItToTheContinuation )
			{
				// INIT
				auto n1 = make_shared<value_task_node<string>>();
				auto n2 = make_shared<value_task_node<void>>();
				auto c1 = make_shared<mocks::continuation<string>>([] (async_result<string> &&r) {
					assert_equal("loremIPSUMamet", *r);
				});
				auto c2 = make_shared<mocks::continuation<void>>([] (async_result<void> &&r) {
					*r; // Must not throw.
				});

				n1->set("loremIPSUMamet");
				n2->set();
	
				// ACT / ASSERT
				n1->then(c1);
				n2->then(c2);

				// ASSERT
				assert_equal(1, c1->called);
				assert_equal(1, c2->called);
				assert_throws(n1->then(c1), logic_error); // State is kept.
				assert_throws(n2->then(c2), logic_error);
			}


			test( AssigningContinuationAfterFailureIsSetDeliversItToTheContinuation )
			{
				// INIT
				auto n1 = make_shared<value_task_node<string>>();
				auto n2 = make_shared<value_task_node<void>>();
				auto c1 = make_shared<mocks::continuation<string>>([] (async_result<string> &&r) {
					assert_throws(*r, invalid_argument);
				});
				auto c2 = make_shared<mocks::continuation<void>>([] (async_result<void> &&r) {
					assert_throws(*r, out_of_range);
				});

				n1->fail(invalid_argument(""));
				n2->fail(make_exception_ptr(out_of_range("")));
	
				// ACT / ASSERT
				n1->then(c1);
				n2->then(c2);

				// ASSERT
				assert_equal(1, c1->called);
				assert_equal(1, c2->called);
				assert_throws(n1->then(c1), logic_error); // State is kept.
				assert_throws(n2->then(c2), logic_error);
			}


			test( SettingResultWhenContinuationIsAssignedDeliversItToTheContinuation )
			{
				// INIT
				auto n1 = make_shared<value_task_node<string>>();
				auto n2 = make_shared<value_task_node<void>>();
				auto c1 = make_shared<mocks::continuation<string>>([] (async_result<string> &&r) {
					assert_equal("lorem ipsum amet", *r);
				});
				auto c2 = make_shared<mocks::continuation<void>>([] (async_result<void> &&r) {
					*r; // Must not throw.
				});

				n1->then(c1);
				n2->then(c2);

				// ACT / ASSERT
				n1->set("lorem ipsum amet");
				n2->set();

				// ASSERT
				assert_equal(1, c1->called);
				assert_equal(1, c2->called);
			}


			test( SettingAFailureWhenContinuationIsAssignedDeliversItToTheContinuation )
			{
				// INIT
				auto n1 = make_shared<value_task_node<string>>();
				auto n2 = make_shared<value_task_node<void>>();
				auto c1 = make_shared<mocks::continuation<string>>([] (async_result<string> &&r) {
					assert_throws(*r, invalid_argument);
				});
				auto c2 = make_shared<mocks::continuation<void>>([] (async_result<void> &&r) {
					assert_throws(*r, out_of_range);
				});

				n1->then(c1);
				n2->then(c2);

				// ACT / ASSERT
				n1->fail(invalid_argument(""));
				n2->fail(make_exception_ptr(out_of_range("")));

				// ASSERT
				assert_equal(1, c1->called);
				assert_equal(1, c2->called);
			}


			test( FailureToCopyTheResultIsDeliveredToContinuationAsAFailure )
			{
				struct failure_on_copy
				{
					failure_on_copy() = default;

					failure_on_copy(const failure_on_copy &)
					{	throw string("an error to copy");	}

					void operator =(const failure_on_copy &rhs) = delete;
				};

				// INIT
				auto n = make_shared<value_task_node<failure_on_copy>>();
				auto c = make_shared<mocks::continuation<failure_on_copy>>([] (async_result<failure_on_copy> &&r) {
					assert_throws(*r, string);
				});

				n->then(c);

				// ACT / ASSERT
				n->set(failure_on_copy());

				// ASSERT
				assert_equal(1, c->called);
			}


			test( ContinuationIsReleasedAfterInvocation )
			{
				// INIT
				auto p = make_shared<bool>();
				auto n = make_shared<value_task_node<void>>();

				n->then(make_shared<mocks::continuation<void>>([p] (async_result<void> &&) {
					assert_not_equal(1, p.use_count());
				}));

				// ASSERT
				assert_equal(2, p.use_count());

				// ACT
				n->set();
				
				// ASSERT
				assert_equal(1, p.use_count());
			}
		end_test_suite
	}
}
