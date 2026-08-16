#include <tasker/lifetime.h>

#include <functional>
#include <stdexcept>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace tasker
{
	namespace tests
	{
		struct noop_event
		{
			void wait() {	}
			void set() {	}
		};

		struct mock_event
		{
			void wait() {	on_wait();	}
			void set() {	on_set();	}

			function<void ()> on_wait;
			function<void ()> on_set;
		};

		begin_test_suite( LifetimeBlockTests )
			test( LifetimeCanCarryAnyEvent )
			{
				// INIT / ACT
				auto l1 = make_lifetime((noop_event()));
				auto l2 = make_lifetime((mock_event()));

				// ASSERT
				assert_not_null(l1);
				assert_not_null(l2);
			}


			test( LifetimeIfAliveCallsTheFunctionPassed )
			{
				// INIT
				auto called = 0;
				auto method_called = 0;
				auto l = make_lifetime((mock_event {
					[&] {	called++;	},
					[&] {	called++;	}
				}));

				// ACT
				l->try_if_alive([&] {
					method_called++;
				});

				// ASSERT
				assert_equal(0, called);
				assert_equal(1, method_called);
			}


			test( LifetimeIfAliveDoesntCallTheFunctionIfEnded )
			{
				// INIT
				auto wait_called = 0;
				auto set_called = 0;
				auto method_called = 0;
				auto l = make_lifetime((mock_event {
					[&] {	wait_called++;	},
					[&] {	set_called++;	}
				}));

				// ACT
				l->end();
				l->try_if_alive([&] {
					method_called++;
				});
				l->try_if_alive([&] {
					method_called++;
					l->try_if_alive([&] {
						method_called++;
					});
				});

				// ASSERT
				assert_equal(0, wait_called);
				assert_equal(0, set_called);
				assert_equal(0, method_called);
			}


			test( ExceptionThrownDoesNotBreakEnd )
			{
				// INIT
				auto called = 0;
				auto method_called = 0;
				auto l = make_lifetime((mock_event {
					[&] {	called++;	},
					[&] {	called++;	}
				}));

				// ACT / ASSERT
				l->try_if_alive([&] {	method_called++;	});
				assert_throws(l->try_if_alive([&] {	throw logic_error("abc"); }), logic_error);
				l->end();

				// ASSERT
				assert_equal(0, called);
				assert_equal(1, method_called);

				// ACT
				l->try_if_alive([&] { method_called++; });
				l->try_if_alive([&] { method_called++; });
				l->try_if_alive([&] { method_called++; });

				// ASSERT
				assert_equal(0, called);
				assert_equal(1, method_called);
			}


			test( EndingEntersWaitIfCalledWhileExecutionIsOnAndScopeUnlockCallsSet )
			{
				// INIT
				auto set_called = 0;
				auto l = make_lifetime((mock_event {
					[&] {	throw 0;	},
					[&] {	set_called++;	}
				}));

				// ACT
				l->try_if_alive([&] {
					assert_throws(l->end(), int); // simulate blocking by breaking end() execution at wait() call.

				// ASSERT
					assert_equal(set_called, 0);
				});
				assert_equal(set_called, 1);
			}


			test( ConcurrentIfAliveCallIsSkippedIfAlreadyMarkedAsDead )
			{
				// INIT
				auto set_called = 0;
				auto l = make_lifetime((mock_event {
					[&] {	throw 0;	},
					[&] {	set_called++;	}
				}));

				l->try_if_alive([&] {
					assert_throws(l->end(), int);

				// ACT / ASSERT
					l->try_if_alive([&] {
						assert_is_false(true);
					});
					assert_equal(0, set_called);
				});

				// ASSERT
				assert_equal(1, set_called);
			}


			test( EventSpecializedLifetimeBehavesTheSame )
			{
				// INIT
				auto called = 0;
				auto l = make_lifetime();

				// ACT
				l->try_if_alive([&] {
					called++;
					l->try_if_alive([&] {
						called++;
					});
				});

				// ASSERT
				assert_equal(2, called);

				// ACT
				l->end();
				l->try_if_alive([&] {
					called++;
					l->try_if_alive([&] {
						called++;
					});
				});

				// ASSERT
				assert_equal(2, called);
			}


			test( IfAliveWithExceptionFactoryReturnsResults )
			{
				// INIT
				auto called = 0;
				auto l = make_lifetime(mock_event());

				// ACT / ASSERT
				assert_equal(3.14159, l->if_alive([] {	return 3.14159;	}, [] {	return 0;	}));
				assert_equal("lorem", l->if_alive([] {	return string("lorem");	}, [] {	return logic_error("");	}));
				l->if_alive([&] {	called++;	}, [] {	return logic_error("");	});

				// ASSERT
				assert_equal(1, called);
			}


			test( LifetimeIfAliveThrowsAnExceptionIfEnded )
			{
				// INIT
				auto wait_called = 0;
				auto set_called = 0;
				auto l = make_lifetime((mock_event {
					[&] {	wait_called++;	},
					[&] {	set_called++;	}
				}));

				// ACT
				l->if_alive([&] {
					return 5;
				}, [] {	return invalid_argument("this won't be thrown");	});
				l->end();

				// ASSERT
				assert_equal(0, wait_called);

				// ACT / ASSERT
				assert_throws(l->if_alive([&] {
					return 5;
				}, [] {	return invalid_argument("this will be thrown");	}), invalid_argument);

				// ASSERT
				assert_equal(0, set_called);

				// ACT / ASSERT
				assert_throws(l->if_alive([&] {
					return "abc";
				}, [] {	return logic_error("xxx");	}), logic_error);

				// ASSERT
				assert_equal(0, set_called);
			}


			test( TryIfAliveForwardsArgumentsToProtectedFunction )
			{
				// INIT
				auto called = 0;
				int a = 0;
				string b;
				double c = 0.0;
				auto l = make_lifetime(mock_event());

				// ACT
				l->try_if_alive([&] (int x) {
					a = x;
					++called;
				}, 11);
				l->try_if_alive([&] (int x, const string &y) {
					a = x;
					b = y;
					++called;
				}, 22, string("ipsum"));
				l->try_if_alive([&] (int x, const string &y, double z) {
					a = x;
					b = y;
					c = z;
					++called;
				}, 33, string("dolor"), 2.5);

				// ASSERT
				assert_equal(3, called);
				assert_equal(33, a);
				assert_equal("dolor", b);
				assert_equal(2.5, c);
			}


			test( IfAliveForwardsArgumentsToProtectedFunction )
			{
				// INIT
				auto called = 0;
				auto l = make_lifetime(mock_event());

				// ACT / ASSERT
				assert_equal(11, l->if_alive([] (int x) {
					return x;
				}, [] {	return logic_error("");	}, 11));
				assert_equal("ipsum", l->if_alive([] (int, const string &y) {
					return y;
				}, [] {	return logic_error("");	}, 22, string("ipsum")));
				assert_equal(2.5, l->if_alive([&] (int x, const string &y, double z) {
					++called;
					assert_equal(33, x);
					assert_equal("dolor", y);
					return z;
				}, [] {	return logic_error("");	}, 33, string("dolor"), 2.5));

				// ASSERT
				assert_equal(1, called);
			}

		end_test_suite
	}
}
