#pragma once

#include "async_result.h"
#include "continuation.h"
#include "forward_result.h"

#include <mt/atomic.h>
#include <stdexcept>

namespace tasker
{
	template <typename T>
	class value_task_node : public std::enable_shared_from_this< value_task_node<T> >
	{
	public:
		typedef async_result<T> result_type;
		typedef typename continuation<result_type>::ptr continuation_ptr;
		typedef std::shared_ptr<value_task_node> ptr;

	public:
		value_task_node();

		template <typename U>
		void set(U &&result);
		void set();

		template <typename E>
		void fail(E &&exception);
		void fail(std::exception_ptr &&exception);

		void then(const continuation_ptr &continuation_);

	protected:
		template <typename F>
		void set_result(F &&result_setter);

	private:
		typedef unsigned int state_type;

		enum : state_type
		{
			callback_claimed_increment = 0x00000001u,
			callback_claimed_mask = 0x00007fffu,
			result_claimed_increment = 0x00008000u,
			result_claimed_mask = 0x3fff8000u,
			callback_ready = 0x40000000u,
			result_ready = 0x80000000u
		};

	private:
		void claim(state_type increment, state_type mask, const char *error_message);
		void dispatch(state_type set, state_type check);

	private:
		mt::atomic<state_type> _state;
		result_type _result;
		continuation_ptr _continuation;
	};



	template <typename T>
	inline value_task_node<T>::value_task_node()
		: _state(0)
	{	}

	template <typename T>
	template <typename U>
	inline void value_task_node<T>::set(U &&result)
	{	set_result([&] (result_type &r) {	r.set(std::forward<U>(result));	});	}

	template <typename T>
	inline void value_task_node<T>::set()
	{	set_result([] (result_type &r) {	r.set();	});	}

	template <typename T>
	template <typename E>
	inline void value_task_node<T>::fail(E &&exception)
	{	set_result([&] (result_type &r) {	r.fail(std::make_exception_ptr(std::forward<E>(exception)));	});	}

	template <typename T>
	inline void value_task_node<T>::fail(std::exception_ptr &&exception)
	{	set_result([&] (result_type &r) {	r.fail(std::forward<std::exception_ptr>(exception));	});	}

	template <typename T>
	inline void value_task_node<T>::then(const continuation_ptr &continuation_)
	{
		claim(callback_claimed_increment, callback_claimed_mask, "a value continuation has already been registered");
		_continuation = continuation_;
		dispatch(callback_ready, result_ready);
	}

	template <typename T>
	template <typename F>
	inline void value_task_node<T>::set_result(F &&result_setter)
	{
		claim(result_claimed_increment, result_claimed_mask, "a value or exception has already been set");
		trap_exception(_result, [&] {	result_setter(_result);	});
		dispatch(result_ready, callback_ready);
	}

	template <typename T>
	inline void value_task_node<T>::claim(state_type increment, state_type mask, const char *error_message)
	{
		if (_state.fetch_add(increment, mt::memory_order_relaxed) & mask)
			throw std::logic_error(error_message);
	}

	template <typename T>
	inline void value_task_node<T>::dispatch(state_type set, state_type check)
	{
		if (_state.fetch_add(set, mt::memory_order_acq_rel) & check)
		{
			auto c = std::move(_continuation);
			c->begin(std::shared_ptr<result_type>(this->shared_from_this(), &_result));
		}
	}
}
