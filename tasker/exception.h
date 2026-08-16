#pragma once

#include <exception>

namespace tasker
{
	struct callback_dead_exception : std::exception
	{
	};
}
