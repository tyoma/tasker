#pragma once

#include "chrono.h"

#if defined(TASKER_USE_STD_MT)
	#include <thread>

	namespace mt
	{
		class thread : public std::thread
		{
		public:
			template <typename F>
			thread(F &&thread_function);
			~thread();
		};

		namespace this_thread
		{
			using namespace std::this_thread;
		}



		template <typename F>
		inline thread::thread(F &&thread_function)
			: std::thread(std::forward<F>(thread_function))
		{	}

		inline thread::~thread()
		{
			if (joinable())
				join();
		}
	}
#else
	#include <functional>

	namespace mt
	{
		class thread
		{
		public:
			typedef unsigned int id;

		public:
			explicit thread(const std::function<void()> &f);
			~thread() throw();

			void join();
			void detach();

			id get_id() const throw();

		private:
			id _id;
			void *_thread;
		};

		namespace this_thread
		{
			thread::id get_id();
			void sleep_for(milliseconds period);
		}



		inline thread::id thread::get_id() const throw()
		{	return _id;	}
	}
#endif
