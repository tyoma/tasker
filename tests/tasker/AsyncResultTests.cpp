#include <tasker/async_result.h>

#include <memory>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace tasker
{
	namespace tests
	{
		namespace
		{
			struct no_default_constructor
			{
				no_default_constructor(int value_)
					: value(value_)
				{	}

				int value;
			};

			template <typename T>
			exception_ptr make_exception(const T &e)
			{
				try
				{	throw e;	}
				catch (...)
				{	return current_exception();	}
			}
		}

		begin_test_suite( ValuedAsyncResultTests )
			test( DestroyingUnsetResultIsOK )
			{
				{
				// INIT / ACT
					async_result<int> r1;
					async_result<string> r2;

				// ACT / ASSERT
					assert_equal(async_in_progress, r1.state());
					assert_equal(async_in_progress, r2.state());

				// ASSERT (no crash at destroy)
				}
			}


			test( ProvidedValueIsAvailableOnDereference )
			{
				// INIT / ACT
				async_result<int> r1, r2;
				async_result<string> r3, r4;

				// ACT
				r1.set(150);
				r2.set(13);
				r3.set(string("lorem"));
				r4.set(string("ipsum"));

				// ASSERT
				assert_equal(async_completed, r1.state());
				assert_equal(150, *r1);
				assert_equal(13, *r2);
				assert_equal(async_completed, r3.state());
				assert_equal("lorem", *r3);
				assert_equal("ipsum", *r4);
			}


			test( NoDefaultInstanceIsCreatedByAsyncResult )
			{
				// INIT
				async_result<no_default_constructor> r;

				// ACT
				r.set(no_default_constructor(11911));

				// ASSERT
				assert_equal(11911, (*r).value);
			}


			test( ValueHeldIsDestroyedOnAsyncResultDestruction )
			{
				// INIT
				const auto f = make_shared<bool>();
				unique_ptr< async_result< shared_ptr<void> > > r(new async_result< shared_ptr<void> >());

				// ACT
				r->set(shared_ptr<bool>(f));

				// ASSERT
				assert_is_true(f.use_count() > 1);

				// ACT
				r.reset();

				// ASSERT
				assert_equal(1, f.use_count());
			}


			test( ProvidedExceptionIsThrownOnDereference )
			{
				// INIT
				async_result<int> r1, r2, r3, r4, r5;
				auto e2 = make_shared<int>(1192);
				auto e3 = make_shared<string>("abc");

				r1.fail(make_exception(1928));
				r2.fail(make_exception(e2));
				r3.fail(make_exception(e3));
				r4.fail(13131);
				r5.fail(string("loremIpsum"));

				// ACT
				try
				{
					*r1;
					assert_is_false(true);
				}
				catch (int value)
				{

				// ASSERT
					assert_equal(1928, value);
				}
				assert_equal(async_faulted, r1.state());

				// ACT
				try
				{
					*r2;
					assert_is_false(true);
				}
				catch (shared_ptr<int> value)
				{

				// ASSERT
					assert_equal(e2, value);
				}

				// ACT
				try
				{
					*r3;
					assert_is_false(true);
				}
				catch (shared_ptr<string> value)
				{

				// ASSERT
					assert_equal(e3, value);
				}

				// ACT
				try
				{
					*r4;
					assert_is_false(true);
				}
				catch (int value)
				{

				// ASSERT
					assert_equal(13131, value);
				}

				// ACT
				try
				{
					*r5;
					assert_is_false(true);
				}
				catch (const string &value)
				{

				// ASSERT
					assert_equal("loremIpsum", value);
				}
			}


			test( ExceptionHeldIsDestroyedOnAsyncResultDestruction )
			{
				// INIT
				const auto f = make_shared<bool>();
				unique_ptr< async_result<int> > r1(new async_result<int>());
				unique_ptr< async_result<int> > r2(new async_result<int>());

				// ACT
				r1->fail(make_exception(f));
				r2->fail(f);

				// ASSERT
				assert_is_true(f.use_count() > 2);

				// ACT
				r1.reset();

				// ASSERT
				assert_equal(2, f.use_count());

				// ACT
				r2.reset();

				// ASSERT
				assert_equal(1, f.use_count());
			}


			test( SettingNonEmptyResultFails )
			{
				// INIT
				async_result<int> with_value, with_exception;

				with_value.set(12345);
				with_exception.fail(make_exception(111));

				// ACT / ASSERT
				assert_throws(with_value.set(1), logic_error);
				assert_throws(with_exception.set(2), logic_error);
				assert_throws(with_value.fail(make_exception("bbb")), logic_error);
				assert_throws(with_exception.fail(make_exception("ccc")), logic_error);
				assert_throws(with_exception.fail("ccc"), logic_error);
			}


			test( AssigningAsyncResultCopiesItsState )
			{
				// INIT
				async_result<int> r1, r2, r3, cr1, cr2, cr3;
				async_result<string> r4, cr4;
				async_result<void> r5, cr5;
				async_result<void> er6, cr6;

				r1.set(17), r2.set(1911), r3.fail(make_exception(string("failure")));
				r4.set(string("lorem ipsum"));
				r5.set();

				// ACT
				cr1 = r1, cr2 = r2, cr3 = r3;
				cr4 = r4;
				cr5 = r5;
				cr6 = er6;

				// ASSERT
				assert_equal(17, *cr1);
				assert_equal(1911, *cr2);
				try {
					*cr3;
					assert_is_false(true);
				} catch (const string &e) {
					assert_equal("failure", e);
				}
				assert_equal("lorem ipsum", *cr4);
				*cr5; // no exception
				assert_equal(async_in_progress, er6.state());
			}


			test( AttemptingToAssignToANonEmptyResultFails )
			{
				// INIT
				async_result<int> r1, r2;

				r1.set(17), r2.set(1911);

				// ACT / ASSERT
				assert_throws(r1 = r2, logic_error);
			}
		end_test_suite


		begin_test_suite( VoidAsyncResultTests )
			test( DestroyingUnsetResultIsOK )
			{
				{
				// INIT / ACT
					async_result<void> r;

				// ASSERT
					assert_equal(async_in_progress, r.state());

				// ASSERT (no crash at destroy)
				}
			}


			test( ProvidedValueIsAvailableOnDereference )
			{
				// INIT / ACT
				async_result<void> r;

				// ACT
				r.set();

				// ACT / ASSERT (no exception)
				*r;

				// ASSERT
				assert_equal(async_completed, r.state());
			}


			test( DereferencingUnsetResultFails )
			{
				// INIT / ACT
				async_result<void> r;

				// ACT / ASSERT
				assert_throws(*r, logic_error);
			}


			test( ProvidedExceptionIsThrownOnDereference )
			{
				// INIT
				async_result<void> r1, r2, r3;
				auto e2 = make_shared<int>(1192);
				auto e3 = make_shared<string>("abc");

				r1.fail(make_exception(1928));
				r2.fail(make_exception(e2));
				r3.fail(make_exception(e3));

				// ACT
				try
				{
					*r1;
					assert_is_false(true);
				}
				catch (int value)
				{

				// ASSERT
					assert_equal(1928, value);
				}
				assert_equal(async_faulted, r1.state());

				// ACT
				try
				{
					*r2;
					assert_is_false(true);
				}
				catch (shared_ptr<int> value)
				{

				// ASSERT
					assert_equal(e2, value);
				}

				// ACT
				try
				{
					*r3;
					assert_is_false(true);
				}
				catch (shared_ptr<string> value)
				{

				// ASSERT
					assert_equal(e3, value);
				}
			}


			test( ExceptionHeldIsDestroyedOnAsyncResultDestruction )
			{
				// INIT
				const auto f = make_shared<bool>();
				unique_ptr< async_result<int> > r(new async_result<int>());

				// ACT
				r->fail(make_exception(f));

				// ASSERT
				assert_is_true(f.use_count() > 1);

				// ACT
				r.reset();

				// ASSERT
				assert_equal(1, f.use_count());
			}


			test( SettingNonEmptyResultFails )
			{
				// INIT
				async_result<void> with_value, with_exception;

				with_value.set();
				with_exception.fail(make_exception(111));

				// ACT / ASSERT
				assert_throws(with_value.set(), logic_error);
				assert_throws(with_exception.set(), logic_error);
				assert_throws(with_value.fail(make_exception("bbb")), logic_error);
				assert_throws(with_exception.fail(make_exception("ccc")), logic_error);
				assert_throws(with_exception.fail("ccc"), logic_error);
			}
		end_test_suite
	}
}
