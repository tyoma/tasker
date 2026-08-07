#include <tasker/object_pool.h>

#include <functional>
#include <string>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace tasker
{
	namespace tests
	{
		namespace
		{
			class my_object
			{
			public:
				~my_object()
				{
					if (on_destroy)
						on_destroy();
				}

				function<void ()> on_destroy;
			};
		}

		begin_test_suite( ObjectPoolTests )
			test( PoolConstructsNewObjectsUntilLimitIsReached )
			{
				// INIT / ACT
				auto p1 = object_pool<string>::construct(2);
				auto p2 = object_pool<string>::construct(3);

				// ASSERT
				assert_not_null(p1);
				assert_not_null(p2);

				// ACT
				auto s1 = p1->allocate();
				auto s2 = p1->allocate();
				auto s3 = p2->allocate();
				auto s4 = p2->allocate();
				auto s5 = p2->allocate();

				// INIT / ACT (contained objects are strings)
				string &s1_ref = *s1;

				// ASSER
				assert_not_null(s1);
				assert_not_null(s2);
				assert_not_null(s3);
				assert_not_null(s4);
				assert_not_null(s5);
				assert_is_empty(*s1);
				assert_is_empty(*s1);

				// ACT / ASSERT
				assert_null(p1->allocate());
				assert_null(p2->allocate());
			}


			test( ReturningAnObjectToPoolMakesItAvailableForAllocationAgain )
			{
				// INIT / ACT
				auto p = object_pool<string>::construct(2);
				auto o1 = p->allocate();
				auto o2 = p->allocate();

				*o1 = "lorem ipsum amet dolor ---";

				// ACT
				o1 = nullptr;

				// ASSERT
				assert_null(o1);

				// ACT
				o1 = p->allocate();

				// ASSERT
				assert_not_null(o1);
				assert_equal("lorem ipsum amet dolor ---", *o1); // pool does not guarantee object clearing

				// ACT / ASSERT (the limit is still honored)
				assert_null(p->allocate());

				// ACT
				o1 = nullptr;
				o2 = nullptr;

				// ACT / ASSERT
				assert_not_null(o1 = p->allocate());
				assert_not_null(o2 = p->allocate());
				assert_null(p->allocate());
			}


			test( NewObjectsAreNotConstructedUntilPooledAreExhausted )
			{
				// INIT / ACT
				auto p = object_pool<string>::construct(3);
				auto o = p->allocate();

				*o = "lorem ipsum amet dolor 123";

				// ACT
				o = nullptr;
				o = p->allocate();

				// ASSERT
				assert_equal("lorem ipsum amet dolor 123", *o);
			}


			test( PoolIsNotDestroyedUntilLastPooledObjectIsReleased )
			{
				// INIT
				auto p = object_pool<my_object>::construct(2);
				weak_ptr<object_pool<my_object>> weak = p;
				pool_ptr<my_object> o1 = p->allocate();
				pool_ptr<my_object> o2 = p->allocate();

				// ACT
				p.reset();

				// ASSERT
				assert_is_false(weak.expired());

				// ACT
				o1.reset();

				// ASSERT
				assert_is_false(weak.expired());

				// ACT
				o2 = nullptr;

				// ASSERT
				assert_is_true(weak.expired());
			}


			test( PooledObjectsAreNotDestroyedUntilThePoolIsDestroyed )
			{
				// INIT
				auto p = object_pool<my_object>::construct(2);
				pool_ptr<my_object> o1 = p->allocate();
				pool_ptr<my_object> o2 = p->allocate();
				auto destroyed = 0;

				o1->on_destroy = [&] { destroyed++; };
				o2->on_destroy = [&] { destroyed++; };

				// ACT
				o1.reset();
				o2.reset();

				// ASSERT
				assert_equal(0, destroyed);

				// ACT
				p.reset();

				// ASSERT
				assert_equal(2, destroyed);
			}


			test( PooledObjectsAreNotDestroyedUntilTheLastPooledObjectIsReleased )
			{
				// INIT
				auto p = object_pool<my_object>::construct(4);
				auto o1 = p->allocate();
				auto o2 = p->allocate();
				auto o3 = p->allocate();
				auto destroyed = 0;

				o1->on_destroy = [&] { destroyed++; };
				o2->on_destroy = [&] { destroyed++; };
				o3->on_destroy = [&] { destroyed++; };

				p = nullptr;

				// ACT
				o1 = nullptr;
				o2 = nullptr;

				// ASSERT
				assert_equal(0, destroyed);

				// ACT
				o3 = nullptr;

				// ASSERT
				assert_equal(3, destroyed);
			}
		end_test_suite
	}
}
