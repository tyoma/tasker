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

#include <tasker/private_queue.h>

#include <tasker/scheduler.h>

using namespace std;

namespace tasker
{
	private_queue::private_queue(queue &apartment_queue)
		: _apartment_queue(apartment_queue), _lifetime(make_lifetime())
	{	}

	private_queue::~private_queue()
	{	_lifetime->end();	}

	void private_queue::schedule(function<void ()> &&task, mt::milliseconds defer_by)
	{
		auto l = _lifetime;
		function<void ()> captured([task, l] {
			l->try_if_alive(task);
		});

		task = function<void ()>();
		_apartment_queue.schedule(std::move(captured), defer_by);
	}


	private_worker_queue::private_worker_queue(queue &worker_queue, queue &apartment_queue)
		: _worker_queue(worker_queue), _apartment_queue(apartment_queue), _lifetime(make_lifetime())
	{	}

	private_worker_queue::~private_worker_queue()
	{	_lifetime->end();	}

	void private_worker_queue::schedule(async_task &&task)
	{
		auto l = _lifetime;

		_worker_queue.schedule([this, task, l] {
			l->try_if_alive([this, &task] {
				completion c(*this);

				task(c);
			});
		});
	}

	void private_worker_queue::deliver(function<void ()> &&progress)
	{
		auto l = _lifetime;

		_apartment_queue.schedule([progress, l] {
			l->try_if_alive(progress);
		});
	}


	private_worker_queue::completion::completion(private_worker_queue &owner)
		: _owner(owner)
	{	}

	void private_worker_queue::completion::deliver(function<void ()> &&progress)
	{	_owner.deliver(std::move(progress));	}
}
