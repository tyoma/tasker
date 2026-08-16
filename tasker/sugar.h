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

#include "exception.h"
#include "lifetime.h"
#include "task.h"
#include "task_algorithm.h"
#include "type_traits.h"

#include <tuple>

namespace tasker
{
	enum go_t { go };
	enum unwrap_t { unwrap };

	template <typename F, typename E>
	struct protected_call
	{
		template <typename... T>
		detail::invoke_result_t<F, T &&...> operator ()(T &&...args)
		{
			return lifetime->if_alive(underlying, [] {
				return callback_dead_exception();
			}, std::forward<T>(args)...);
		}

		F underlying;
		std::shared_ptr<basic_lifetime<E>> lifetime;
	};

	template <typename C>
	inline std::tuple<C &&, queue &> next(C &&callback, queue &execute_on)
	{	return std::tuple<C&&, queue&>(std::forward<C>(callback), execute_on);	}

	template <typename C>
	inline std::tuple<C &&, queue &> operator /(C &&callback, queue &execute_on)
	{	return next(std::forward<C>(callback), execute_on);	}

	template <typename C, typename E>
	inline protected_call<C, E> operator /(C &&callback, const std::shared_ptr<basic_lifetime<E>> &lifetime_)
	{	return protected_call<C, E> {	std::forward<C>(callback), lifetime_ };	}

	template <typename T, typename C>
	inline task<detail::invoke_result_t<C, const async_result<T> &>> operator >>(const task<T> &lhs,
		std::tuple<C &&, queue &> &&continuation)
	{	return lhs.then(std::forward<C>(std::get<0>(continuation)), std::get<1>(continuation));	}

	template <typename T>
	inline task<T> operator >>(task<task<T>> &&lhs, unwrap_t)
	{	return lhs.unwrap();	}

	namespace detail
	{
		template <typename C, typename... T, std::size_t... I>
		inline task<invoke_result_t<C, const async_result<T> &...>> when_all_from_tuple(const std::tuple<task<T>...> &tasks,
			std::tuple<C &&, queue &> &&continuation, index_sequence<I...>)
		{
			return when_all(std::forward<C>(std::get<0>(continuation)), std::get<1>(continuation),
				std::get<I>(tasks)...);
		}

		template <typename T1, typename... T2, std::size_t... I>
		inline std::tuple<task<T1>, task<T2>...> prepend_task(task<T1> &&lhs, std::tuple<task<T2>...> &&rhs,
			index_sequence<I...>)
		{	return std::tuple<task<T1>, task<T2>...>(std::move(lhs), std::get<I>(std::move(rhs))...);	}

		template <typename T2, typename... T1, std::size_t... I>
		inline std::tuple<task<T1>..., task<T2>> append_task(std::tuple<task<T1>...> &&lhs, task<T2> &&rhs,
			index_sequence<I...>)
		{	return std::tuple<task<T1>..., task<T2>>(std::get<I>(std::move(lhs))..., std::move(rhs));	}
	}

	template <typename C, typename... T>
	inline task<detail::invoke_result_t<C, const async_result<T> &...>> operator >>(const std::tuple<task<T>...> &lhs,
		std::tuple<C &&, queue &> &&continuation)
	{
		return detail::when_all_from_tuple(lhs, std::move(continuation),
			typename detail::make_index_sequence<sizeof...(T)>::type());
	}

	template <typename C>
	inline task<detail::invoke_result_t<C>> operator >>(go_t, std::tuple<C &&, queue &> &&continuation)
	{	return schedule_task(std::forward<C>(std::get<0>(continuation)), std::get<1>(continuation));	}

	template <typename T1, typename T2>
	inline std::tuple<task<T1>, task<T2>> operator &(task<T1> &&lhs, task<T2> &&rhs)
	{	return { std::move(lhs), std::move(rhs) };	}

	template <typename T1, typename... T2>
	inline std::tuple<task<T1>, task<T2>...> operator &(task<T1> &&lhs, std::tuple<task<T2>...> &&rhs)
	{
		return detail::prepend_task(std::move(lhs), std::move(rhs),
			typename detail::make_index_sequence<sizeof...(T2)>::type());
	}

	template <typename T2, typename... T1>
	inline std::tuple<task<T1>..., task<T2>> operator &(std::tuple<task<T1>...> &&lhs, task<T2> &&rhs)
	{
		return detail::append_task(std::move(lhs), std::move(rhs),
			typename detail::make_index_sequence<sizeof...(T1)>::type());
	}
}
