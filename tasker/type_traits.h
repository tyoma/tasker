#pragma once

#include <cstddef>

namespace tasker
{
	template <typename T>
	class task;

	namespace detail
	{
		template <typename T>
		struct unwrapped_result
		{	typedef void type;	};

		template <typename T>
		struct unwrapped_result< task<T> >
		{	typedef T type;	};

		template<class T>
		using unwrapped_result_t = typename unwrapped_result<T>::type;


		template <std::size_t... I>
		struct index_sequence
		{	};

		template <std::size_t N, std::size_t... I>
		struct make_index_sequence : make_index_sequence<N - 1, N - 1, I...>
		{	};

		template <std::size_t... I>
		struct make_index_sequence<0, I...>
		{	typedef index_sequence<I...> type;	};


		template <typename F, typename... ArgT>
		struct invoke_result
		{
			template <typename U>
			static async_result<U> value_arg();
			static F value_f();

			typedef decltype(value_f()(value_arg<ArgT>()...)) type;
		};

		template< class F, class... ArgTypes >
		using invoke_result_t = typename invoke_result<F, ArgTypes...>::type;
	}
}
