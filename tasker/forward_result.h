#pragma once

#include <exception>

namespace tasker
{
	template <template <typename> class TaskNodeT, typename T, typename F>
	inline void call_set_exception(TaskNodeT<T> &target, F &&f)
	{
		try
		{ f(); }
		catch (...)
		{ target.fail(std::current_exception()); }
	}

	template <template <typename> class TaskNodeT, typename T, typename CallbackT, typename A>
	inline void forward_result(TaskNodeT<T> &target, CallbackT &&callback, A &&antecedent)
	{	call_set_exception<TaskNodeT>(target, [&] { target.set(callback(std::forward<A>(antecedent))); });	}

	template <template <typename> class TaskNodeT, typename T, typename CallbackT>
	inline void forward_result(TaskNodeT<T> &target, CallbackT &&callback)
	{	call_set_exception<TaskNodeT>(target, [&] { target.set(callback()); });	}

	template <template <typename> class TaskNodeT, typename CallbackT, typename A>
	inline void forward_result(TaskNodeT<void> &target, CallbackT &&callback, A &&antecedent)
	{	call_set_exception<TaskNodeT>(target, [&] { callback(std::forward<A>(antecedent)), target.set(); });	}

	template <template <typename> class TaskNodeT, typename CallbackT>
	inline void forward_result(TaskNodeT<void> &target, CallbackT &&callback)
	{	call_set_exception<TaskNodeT>(target, [&] { callback(), target.set(); });	}
}
