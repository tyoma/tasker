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

namespace tasker
{
	namespace detail
	{
		template <typename BodyT>
		inline void loop(const std::shared_ptr< task_node<void> > &completion, const BodyT &body, queue &queue_)
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
}
