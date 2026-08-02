//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#pragma once

#include "scheduler.h"
#include "task_node.h"
#include "type_traits.h"

namespace tasker
{
	struct cancelled : std::exception
	{
	};

	template <typename T>
	class task : task_node<T>::ptr
	{
	public:
		explicit task(typename task_node<T>::ptr &&node);

		template <typename F>
		task<detail::invoke_result_t<F, T>> then(F &&continuation_callback, queue &continue_on) const;

		task<detail::unwrapped_result_t<T>> unwrap();

	private:
		template <typename T2>
		friend struct task_unwrap;
	};

	template <typename T, typename F>
	struct task_root : task_node<T>
	{
		task_root(F &&from);

		typename std::remove_reference<F>::type callback;
	};

	template <typename F, typename ArgT>
	struct task_continuation : task_node<detail::invoke_result_t<F, ArgT>>, continuation<ArgT>
	{
		task_continuation(F &&from, queue &continue_on);

		virtual void begin(const std::shared_ptr< const async_result<ArgT> > &antecedent_result) override;

	private:
		F _callback;
		queue &_continue_on;
	};

	template <typename T>
	struct task_unwrap : task_node<T>, continuation< task<T> >, continuation<T>
	{
		virtual void begin(const std::shared_ptr< const async_result< task<T> > > &antecedent_result) override
		{	(**antecedent_result)->then(std::static_pointer_cast<task_unwrap>(this->shared_from_this()));	}

		virtual void begin(const std::shared_ptr< const async_result<T> > &antecedent_result) override
		{	this->set_result([&] (async_result<T> &r) {	r = *antecedent_result;	});	}
	};



	template <typename T, typename F>
	inline void handle_exception(task_node<T> &target, F &&f)
	{
		try
		{	f();	}
		catch (...)
		{	target.fail(std::current_exception());	}
	}

	template <typename T, typename CallbackT>
	inline void set_result(task_node<T> &target, CallbackT &callback)
	{	handle_exception(target, [&] {	target.set(callback());	});	}

	template <typename CallbackT>
	inline void set_result(task_node<void> &target, CallbackT &callback)
	{	handle_exception(target, [&] {	callback(), target.set();	});	}

	template <typename T, typename CallbackT, typename A>
	inline void set_result(task_node<T> &target, CallbackT &callback, A &antecedent)
	{	handle_exception(target, [&] {	target.set(callback(antecedent));	});	}

	template <typename CallbackT, typename A>
	inline void set_result(task_node<void> &target, CallbackT &callback, A &antecedent)
	{	handle_exception(target, [&] {	callback(antecedent), target.set();	});	}


	template <typename T>
	inline task<T>::task(typename task_node<T>::ptr &&node)
		: task_node<T>::ptr(std::forward<typename task_node<T>::ptr>(node))
	{	}

	template <typename T>
	template <typename F>
	inline task<detail::invoke_result_t<F, T>> task<T>::then(F &&continuation_callback,
		queue &continue_on) const
	{
		auto c = std::make_shared< task_continuation<F, T> >(std::forward<F>(continuation_callback), continue_on);

		(*this)->then(c);
		return task<detail::invoke_result_t<F, T>>(std::move(c));
	}

	template <typename T>
	inline task<detail::unwrapped_result_t<T>> task<T>::unwrap()
	{
		auto c = std::make_shared< task_unwrap<detail::unwrapped_result_t<T>> >();

		(*this)->then(c);
		return task<detail::unwrapped_result_t<T>>(std::move(c));
	}


	template <typename T, typename F>
	inline task_root<T, F>::task_root(F &&from)
		: callback(std::forward<F>(from))
	{	}


	template <typename F, typename ArgT>
	inline task_continuation<F, ArgT>::task_continuation(F &&from, queue &continue_on)
		: _callback(std::forward<F>(from)), _continue_on(continue_on)
	{	}

	template <typename F, typename ArgT>
	inline void task_continuation<F, ArgT>::begin(const std::shared_ptr< const async_result<ArgT> > &result)
	{
		auto self = std::static_pointer_cast<task_continuation>(this->shared_from_this());

		_continue_on.schedule([result, self] {	tasker::set_result(*self, self->_callback, *result);	});
	}


	template <typename F>
	inline task<detail::invoke_result_t<F>> schedule_task(F &&task_callback, queue &run_on)
	{
		typedef detail::invoke_result_t<F> task_type;

		auto r = std::make_shared< task_root<task_type, F> >(std::forward<F>(task_callback));

		run_on.schedule([r] {	tasker::set_result(*r, r->callback);	});
		return task<task_type>(std::move(r));
	}
}
