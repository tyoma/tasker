#pragma once

#include "forward_result.h"
#include "scheduler.h"
#include "task_node.h"
#include "type_traits.h"

#include <utility>

namespace tasker
{
	template <typename T>
	class task : task_node<T>::ptr
	{
	public:
		template <typename PtrT>
		task(PtrT &&node_ptr);
		task(task &&other) = default;

		task &operator =(task &&) = default;

		template <typename F>
		task<detail::invoke_result_t<F, async_result<T> &&>> then(F &&continuation_cb, queue &continue_on) &&;

		task<detail::unwrapped_result_t<T>> unwrap() &&;
	};



	template <typename T>
	template <typename PtrT>
	inline task<T>::task(PtrT &&node_ptr)
		: task_node<T>::ptr(std::forward<PtrT>(node_ptr))
	{	}

	template <typename T>
	template <typename F>
	inline task<detail::invoke_result_t<F, async_result<T> &&>> task<T>::then(F &&continuation_cb, queue &continue_on) &&
	{
		struct task_continuation : task_node<detail::invoke_result_t<F, async_result<T> &&>>,
			continuation<async_result<T>>
		{
			task_continuation(F &&from, queue &continue_on)
				: _callback(std::forward<F>(from)), _continue_on(continue_on)
			{	}

			virtual void begin(const std::shared_ptr<async_result<T>> &antecedent_result) override
			{
				auto self = std::static_pointer_cast<task_continuation>(this->shared_from_this());

				_continue_on.schedule([antecedent_result, self] {
					forward_result<tasker::task_node>(*self, self->_callback, std::move(*antecedent_result));
				});
			}

		private:
			F _callback;
			queue &_continue_on;
		};

		auto c = std::make_shared<task_continuation>(std::forward<F>(continuation_cb), continue_on);
		auto antecedent = std::move(*this);

		antecedent->then(c);
		return std::move(c);
	}

	template <typename T>
	inline task<detail::unwrapped_result_t<T>> task<T>::unwrap() &&
	{
		using U = detail::unwrapped_result_t<T>;

		auto c = std::make_shared<task_node<U>>();

		std::move(*this).then([c] (async_result<T> &&rr) {
			std::move(*rr).then([c] (async_result<U> &&r) {
				forward_result<tasker::task_node>(*c, [&] {	return *std::move(r);	});
			}, immediate);
		}, immediate);
		return c;
	}

	template <typename F>
	inline task<detail::invoke_result_t<F>> schedule_value_task(F &&task_callback, queue &run_on)
	{
		using task_type = detail::invoke_result_t<F>;

		struct value_task_root : task_node<task_type>
		{
			value_task_root(F &&from)
				: callback(std::forward<F>(from))
			{	}

			typename std::remove_reference<F>::type callback;
		};

		auto r = std::make_shared<value_task_root>(std::forward<F>(task_callback));

		run_on.schedule([r] {	forward_result<tasker::task_node>(*r, r->callback);	});
		return task<task_type>(std::move(r));
	}
}
