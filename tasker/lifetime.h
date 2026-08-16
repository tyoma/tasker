#pragma once

#include "type_traits.h"

#include <memory>
#include <mt/atomic.h>
#include <mt/event.h>
#include <type_traits>

namespace tasker
{
	template <typename E>
	class basic_lifetime;

	using lifetime = basic_lifetime<mt::event>;

	template <typename E>
	inline std::shared_ptr<basic_lifetime<E>> make_lifetime(E &&event);

	inline std::shared_ptr<lifetime> make_lifetime();

	template <typename E>
	class basic_lifetime
	{
	private:
		basic_lifetime(E &&event);

	public:
		template <typename F, typename... T>
		void try_if_alive(const F &method, T&& ...args);
		template <typename F, typename EF, typename... T>
		detail::invoke_result_t<F, T ...> if_alive(const F &method, const EF &exception_factory, T&& ...args);
		void end();

	private:
		enum {	dead = -1000000	};
		struct scope_lock;

	private:
		void decrement_notify_last() noexcept;

	private:
		E _event;
		mt::atomic<int> _alive, _set_called;

	private:
		friend std::shared_ptr<basic_lifetime<E>> make_lifetime<E>(E &&event);
		friend std::shared_ptr<lifetime> make_lifetime();
	};

	template <typename E>
	struct basic_lifetime<E>::scope_lock
	{
		~scope_lock()
		{	owner.decrement_notify_last();	}

		bool enter()
		{	return owner._alive.fetch_add(1) >= 0;	}

		basic_lifetime<E> &owner;
	};



	template <typename E>
	inline std::shared_ptr<basic_lifetime<E>> make_lifetime(E &&event)
	{	return std::shared_ptr<basic_lifetime<E>>(new basic_lifetime<E>(std::forward<E>(event)));	}

	inline std::shared_ptr<lifetime> make_lifetime()
	{	return std::shared_ptr<lifetime>(new lifetime(std::move(mt::event())));	}


	template <typename E>
	inline basic_lifetime<E>::basic_lifetime(E&& event)
		: _event(std::forward<E>(event)), _alive(0), _set_called(0)
	{	}

	template <typename E>
	template <typename F, typename... T>
	inline void basic_lifetime<E>::try_if_alive(const F &method, T&&... args)
	{
		for (scope_lock lock = {	*this	}; lock.enter(); )
			return method(std::forward<T>(args)...);
	}

	template <typename E>
	template <typename F, typename EF, typename... T>
	inline detail::invoke_result_t<F, T ...> basic_lifetime<E>::if_alive(const F &method, const EF &exception_factory, T&&... args)
	{
		for (scope_lock lock = {	*this	}; lock.enter(); )
			return method(std::forward<T>(args)...);
		throw exception_factory();
	}

	template <typename E>
	inline void basic_lifetime<E>::end()
	{
		if (_alive.fetch_add(dead) > 0)
			_event.wait();
		_set_called.fetch_add(1); // Avoid unnecessary set() calls when entering if_alive() past the end.
	}

	template <typename E>
	inline void basic_lifetime<E>::decrement_notify_last() noexcept
	{
		if (_alive.fetch_add(-1) == dead + 1 && _set_called.fetch_add(1) == 0)
			_event.set(); // Expected not to throw.
	}
}
