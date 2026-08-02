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

#include "task.h"
#include "type_traits.h"

#include <mt/atomic.h>
#include <tuple>

namespace tasker
{
	namespace detail
	{
		template <typename ResultT, typename CallbackT, typename... T>
		class when_all_completion : public task_node<ResultT>
		{
		public:
			when_all_completion(CallbackT &&callback, queue &continue_on)
				: _callback(std::forward<CallbackT>(callback)), _continue_on(continue_on),
					_remaining(static_cast<int>(sizeof...(T)))
			{	}

			template <std::size_t I, typename T2>
			void set_result(const async_result<T2> &result)
			{
				std::get<I>(_result) = result;
				if (1 == _remaining.fetch_add(-1, mt::memory_order_acq_rel))
				{
					auto self = std::static_pointer_cast<when_all_completion>(this->shared_from_this());

					_continue_on.schedule([self] { self->complete(); });
				}
			}

		private:
			void complete()
			{
				auto callback = [this] {
					return invoke(typename make_index_sequence<sizeof...(T)>::type());
				};

				tasker::set_result(static_cast< task_node<ResultT> & >(*this), callback);
			}

			template <std::size_t... I>
			ResultT invoke(detail::index_sequence<I...>)
			{	return _callback(std::get<I>(_result)...);	}

		private:
			CallbackT _callback;
			queue &_continue_on;
			std::tuple<async_result<T>...> _result;
			mt::atomic<int> _remaining;
		};

		template <typename StateT, typename... T, std::size_t... I>
		inline void subscribe_when_all(const std::shared_ptr<StateT> &state, detail::index_sequence<I...>,
			const task<T> &...tasks)
		{
			int unused[] = { 0, (tasks.then([state] (const async_result<T> &result) {
				state->template set_result<I>(result);
			}, immediate), 0)... };

			(void)unused;
		}

		template <typename BodyT>
		inline void loop(const task_node<void>::ptr &completion, const BodyT &body, queue &queue_)
		{
			schedule_task(body, queue_)
				.unwrap()
				.then([completion, body, &queue_] (const async_result<bool> &result) {
					try
					{
						if (*result)
							loop(completion, body, queue_);
						else
							completion->set();
					}
					catch (...)
					{
						completion->fail(std::current_exception());
					}
				}, queue_);
		}
	}

	template <typename BodyT>
	inline task<void> loop(const BodyT &body, queue &queue_)
	{
		auto loop_completion = std::make_shared< task_node<void> >();

		detail::loop(loop_completion, body, queue_);
		return task<void>(std::move(loop_completion));
	}

	template <typename F, typename... T>
	inline task<typename detail::invoke_result_t<F, T...>> when_all(F &&continuation_callback, queue &queue_,
		const task<T> &...tasks)
	{
		typedef detail::invoke_result_t<F, T...> result_type;
		typedef detail::when_all_completion<result_type, F, T...> state_type;

		auto completion = std::make_shared<state_type>(std::forward<F>(continuation_callback), queue_);

		detail::subscribe_when_all(completion, typename detail::make_index_sequence<sizeof...(T)>::type(), tasks...);
		return task<result_type>(std::move(completion));
	}
}
