#pragma once

#include "forward_result.h"
#include "scheduler.h"
#include "type_traits.h"
#include "value_task_node.h"

#include <utility>

namespace tasker
{
	template <typename T>
	class value_task : value_task_node<T>::ptr
	{
	public:
		explicit value_task(typename value_task_node<T>::ptr &&node);
		value_task(value_task &&) = default;

		value_task &operator =(value_task &&) = default;

		template <typename F>
		value_task<detail::invoke_result_t<F, async_result<T> &&>> then(F &&continuation_callback, queue &continue_on) &&;

	private:
		value_task(const value_task &other);
		void operator =(const value_task &rhs);
	};



	template <typename T>
	inline value_task<T>::value_task(typename value_task_node<T>::ptr &&node)
		: value_task_node<T>::ptr(std::forward<typename value_task_node<T>::ptr>(node))
	{ }

	template <typename T>
	template <typename F>
	inline value_task<detail::invoke_result_t<F, async_result<T> &&>> value_task<T>::then(F &&continuation_callback,
		queue &continue_on) &&
	{
		struct task_continuation : value_task_node<detail::invoke_result_t<F, async_result<T> &&>>,
			continuation<async_result<T>>
		{
			task_continuation(F &&from, queue &continue_on)
				: _callback(std::forward<F>(from)), _continue_on(continue_on)
			{	}

			virtual void begin(const std::shared_ptr<async_result<T>> &antecedent_result) override
			{
				auto self = std::static_pointer_cast<task_continuation>(this->shared_from_this());

				_continue_on.schedule([antecedent_result, self] {
					forward_result<tasker::value_task_node>(*self, self->_callback, std::move(*antecedent_result));
				});
			}

		private:
			F _callback;
			queue &_continue_on;
		};

		auto antecedent = std::move(*this);
		auto c = std::make_shared<task_continuation>(std::forward<F>(continuation_callback), continue_on);

		antecedent->then(c);
		return value_task<detail::invoke_result_t<F, async_result<T> &&>>(std::move(c));
	}


	template <typename F>
	inline value_task<detail::invoke_result_t<F>> schedule_value_task(F &&task_callback, queue &run_on)
	{
		using task_type = detail::invoke_result_t<F>;

		struct value_task_root : value_task_node<task_type>
		{
			value_task_root(F &&from)
				: callback(std::forward<F>(from))
			{	}

			typename std::remove_reference<F>::type callback;
		};

		auto r = std::make_shared<value_task_root>(std::forward<F>(task_callback));

		run_on.schedule([r] {	forward_result<tasker::value_task_node>(*r, r->callback);	});
		return value_task<task_type>(std::move(r));
	}
}
