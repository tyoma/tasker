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

#include "forward_result.h"
#include "scheduler.h"
#include "task_node.h"
#include "type_traits.h"

namespace tasker
{
	template <typename T>
	class task : task_node<T>::ptr
	{
	public:
		explicit task(typename task_node<T>::ptr &&node);

		template <typename F>
		task<detail::invoke_result_t<F, const async_result<T> &>> then(F &&continuation_callback, queue &continue_on) const;

		task<detail::unwrapped_result_t<T>> unwrap() const;
	};



	template <typename T>
	inline task<T>::task(typename task_node<T>::ptr &&node)
		: task_node<T>::ptr(std::forward<typename task_node<T>::ptr>(node))
	{	}

	template <typename T>
	template <typename F>
	inline task<detail::invoke_result_t<F, const async_result<T> &>> task<T>::then(F &&continuation_callback,
		queue &continue_on) const
	{
		struct task_continuation : task_node<detail::invoke_result_t<F, const async_result<T> &>>,
			continuation<const async_result<T>>
		{
			task_continuation(F &&from, queue &continue_on)
				: _callback(std::forward<F>(from)), _continue_on(continue_on)
			{	}

			virtual void begin(const std::shared_ptr< const async_result<T> > &antecedent_result) override
			{
				auto self = std::static_pointer_cast<task_continuation>(this->shared_from_this());

				_continue_on.schedule([antecedent_result, self] {
					forward_result<tasker::task_node>(*self, self->_callback, *antecedent_result);
				});
			}

		private:
			F _callback;
			queue &_continue_on;
		};

		auto c = std::make_shared<task_continuation>(std::forward<F>(continuation_callback), continue_on);

		(*this)->then(c);
		return task<detail::invoke_result_t<F, const async_result<T> &>>(std::move(c));
	}

	template <typename T>
	inline task<detail::unwrapped_result_t<T>> task<T>::unwrap() const
	{
		using U = detail::unwrapped_result_t<T>;

		auto c = std::make_shared<task_node<U>>();

		then([c] (const async_result<T> &rr) {
			(*rr).then([c] (const async_result<U> &r) {
				forward_result<tasker::task_node>(*c, [] (const async_result<U> &result) {
					return *result;
				}, r);
			}, immediate);
		}, immediate);
		return task<U>(std::move(c));
	}


	template <typename F>
	inline task<detail::invoke_result_t<F>> schedule_task(F &&task_callback, queue &run_on)
	{
		typedef detail::invoke_result_t<F> task_type;

		struct task_root : task_node<task_type>
		{
			task_root(F &&from)
				: callback(std::forward<F>(from))
			{	}

			typename std::remove_reference<F>::type callback;
		};

		auto r = std::make_shared<task_root>(std::forward<F>(task_callback));

		run_on.schedule([r] {	forward_result<tasker::task_node>(*r, r->callback);	});
		return task<task_type>(std::move(r));
	}
}
