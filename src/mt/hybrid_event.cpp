#include <mt/hybrid_event.h>

#include "intrinsic.h"

#include <mt/event.h>

namespace mt
{
	hybrid_event::hybrid_event()
		: _state(state_reset), _semaphore(new event())
	{	}

	hybrid_event::~hybrid_event()
	{	delete _semaphore;	}
	
	void hybrid_event::set()
	{
		if (interlocked_exchange(&_state, state_set) == state_blocked)
			_semaphore->set();
	}

	bool hybrid_event::wait(milliseconds timeout)
	{
		for (; ; )
		{
			for (unsigned int i = max_spin; i; --i)
			{
				if (interlocked_exchange(&_state, state_reset) == state_set)
					return true;
				pause();
			}
			if (interlocked_compare_exchange(&_state, state_blocked, state_reset) == state_reset)
				if (!_semaphore->wait(timeout))
					return false;
		}
	}
}
