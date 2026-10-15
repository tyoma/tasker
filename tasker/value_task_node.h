#pragma once

#include "async_result.h"
#include "continuation.h"

#include <mt/atomic.h>
#include <stdexcept>

namespace tasker
{
	template <typename T>
	class value_task_node_base : public std::enable_shared_from_this< value_task_node_base<T> >
	{
	public:
		typedef async_result<T> result_type;

		value_task_node_base();

		void then(const typename continuation<result_type>::ptr &continuation_);

		template <typename E>
		void fail(E &&exception);
		void fail(std::exception_ptr &&exception);

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
		void dispatch();

	private:
		mt::atomic<state_type> _state;
		result_type _result;
		typename continuation<result_type>::ptr _continuation;
	};

	template <typename T>
	struct value_task_node : value_task_node_base<T>
	{
		typedef std::shared_ptr<value_task_node> ptr;
		void set(T &&result);
	};

	template <>
	struct value_task_node<void> : value_task_node_base<void>
	{
		typedef std::shared_ptr<value_task_node> ptr;
		void set();
	};


	template <typename T>
	inline value_task_node_base<T>::value_task_node_base()
		: _state(0)
	{	}

	template <typename T>
	inline void value_task_node_base<T>::then(const typename continuation<result_type>::ptr &continuation_)
	{
		claim(callback_claimed_increment, callback_claimed_mask, "a value continuation has already been registered");
		_continuation = continuation_;
		if (_state.fetch_add(callback_ready, mt::memory_order_acq_rel) & result_ready)
			dispatch();
	}

	template <typename T>
	template <typename E>
	inline void value_task_node_base<T>::fail(E &&exception)
	{	set_result([&] (result_type &r) {	r.fail(std::make_exception_ptr(std::forward<E>(exception)));	});	}

	template <typename T>
	inline void value_task_node_base<T>::fail(std::exception_ptr &&exception)
	{	set_result([&] (result_type &r) {	r.fail(std::forward<std::exception_ptr>(exception));	});	}

	template <typename T>
	template <typename F>
	inline void value_task_node_base<T>::set_result(F &&result_setter)
	{
		claim(result_claimed_increment, result_claimed_mask, "a value or exception has already been set");
		try
		{	result_setter(_result);	}
		catch (...)
		{	_result.fail(std::current_exception());	}
		if (_state.fetch_add(result_ready, mt::memory_order_acq_rel) & callback_ready)
			dispatch();
	}

	template <typename T>
	inline void value_task_node_base<T>::claim(state_type increment, state_type mask, const char *error_message)
	{
		if (_state.fetch_add(increment, mt::memory_order_acq_rel) & mask)
			throw std::logic_error(error_message);
	}

	template <typename T>
	inline void value_task_node_base<T>::dispatch()
	{
		auto continuation_ = std::move(_continuation);
		continuation_->begin(std::shared_ptr<result_type>(this->shared_from_this(), &_result));
	}


	template <typename T>
	inline void value_task_node<T>::set(T &&result)
	{	this->set_result([&] (typename value_task_node<T>::result_type &r) {	r.set(std::forward<T>(result));	});	}

	inline void value_task_node<void>::set()
	{	this->set_result([] (result_type &r) {	r.set();	});	}
}
