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

#include "async_result.h"
#include "continuation.h"

#include <mt/mutex.h>
#include <vector>

namespace tasker
{
	template <typename T>
	class shared_task_node : public std::enable_shared_from_this< shared_task_node<T> >
	{
	public:
		typedef typename continuation<const async_result<T>>::ptr continuation_ptr;
		typedef async_result<T> result_type;
		typedef std::shared_ptr<shared_task_node<T>> ptr;

	public:
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
		mt::mutex _mtx;
		result_type _result;
		std::vector<continuation_ptr> _continuations;
	};



	template <typename T>
	template <typename U>
	inline void shared_task_node<T>::set(U &&result)
	{	this->set_result([&] (typename shared_task_node<T>::result_type &r) {	r.set(std::forward<T>(result));	});	}


	template <typename T>
	inline void shared_task_node<T>::set()
	{	this->set_result([] (result_type &r) {	r.set();	});	}

	template <typename T>
	template <typename E>
	inline void shared_task_node<T>::fail(E &&exception)
	{	set_result([&] (result_type &r) {	r.fail(std::make_exception_ptr(std::forward<E>(exception)));	});	}

	template <typename T>
	inline void shared_task_node<T>::fail(std::exception_ptr &&exception)
	{	set_result([&] (result_type &r) {	r.fail(std::forward<std::exception_ptr>(exception));	});	}

	template <typename T>
	inline void shared_task_node<T>::then(const continuation_ptr &continuation_)
	{
		for (mt::lock_guard<mt::mutex> l(_mtx); _result.state() == async_in_progress; )
			return _continuations.push_back(continuation_);
		continuation_->begin(std::shared_ptr<const result_type>(this->shared_from_this(), &_result));
	}

	template <typename T>
	template <typename F>
	inline void shared_task_node<T>::set_result(F &&result_setter)
	{
		{
			mt::lock_guard<mt::mutex> l(_mtx);

			result_setter(_result); // State: async_in_progress -> async_completed/async_faulted.
		}

		auto result = std::shared_ptr<const result_type>(this->shared_from_this(), &_result);

		for (auto i = std::begin(_continuations); i != std::end(_continuations); ++i)
			(*i)->begin(result);
		_continuations.clear();
	}
}
