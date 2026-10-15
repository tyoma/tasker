#pragma once

#include <exception>

namespace tasker
{
	struct cancelled_exception : std::exception
	{
	};

	struct callback_dead_exception : std::exception
	{
	};
}
