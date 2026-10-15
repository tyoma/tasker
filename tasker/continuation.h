#pragma once

#include <memory>

namespace tasker
{
	template <typename ArgT>
	struct continuation
	{
		typedef std::shared_ptr<continuation> ptr;

		virtual ~continuation() = default;
		virtual void begin(const std::shared_ptr<ArgT> &antecedent) = 0;
	};
}
