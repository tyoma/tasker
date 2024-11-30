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

#include <functional>
#include <stdexcept>

namespace tasker
{
	enum async_state {	async_in_progress, async_completed, async_faulted,	};

	template <typename T>
	class async_result
	{
	public:
		async_result();
		~async_result();

		async_state state() const;

		void operator =(const async_result &rhs);

		void set(T &&from);

		template <typename E>
		void fail(const E &exception);
		void fail(std::exception_ptr &&exception);

		const T &operator *() const;

	protected:
		void assert_empty() const;
		void assert_readable() const;

	private:
		typedef std::function<void ()> rethrow_t;

	private:
		async_result(const async_result &other);

	private:
		char _buffer[sizeof(T) > sizeof(rethrow_t) ? sizeof(T) : sizeof(rethrow_t)];

	protected:
		async_state _state;
	};

	template <>
	class async_result<void> : async_result<short>
	{
	public:
		using async_result<short>::state;
		void set();
		using async_result<short>::fail;

		void operator *() const;
	};



	template <typename T>
	inline async_result<T>::async_result()
		: _state(async_in_progress)
	{	}

	template <typename T>
	inline async_result<T>::~async_result()
	{
		if (async_completed == _state)
			static_cast<T *>(static_cast<void *>(_buffer))->~T();
		else if (async_faulted == _state)
			static_cast<const rethrow_t *>(static_cast<void *>(_buffer))->~rethrow_t();
	}

	template <typename T>
	inline async_state async_result<T>::state() const
	{	return _state;	}

	template <typename T>
	inline void async_result<T>::operator =(const async_result &rhs)
	{
		assert_empty();
		if (async_completed == rhs._state)
			new (_buffer) T(*rhs);
		else if (async_faulted == rhs._state)
			new (_buffer) rethrow_t(*static_cast<const rethrow_t *>(static_cast<const void *>(rhs._buffer)));
		_state = rhs._state;
	}

	template <typename T>
	inline void async_result<T>::set(T &&from)
	{
		assert_empty();
		new (_buffer) T(std::forward<T>(from));
		_state = async_completed;
	}

	template <typename T>
	template <typename E>
	inline void async_result<T>::fail(const E &exception)
	{
		auto e = exception;

		assert_empty();
		new (_buffer) rethrow_t([e] {	throw e;	});
		_state = async_faulted;
	}

	template <typename T>
	inline void async_result<T>::fail(std::exception_ptr &&exception)
	{
		assert_empty();
		new (_buffer) rethrow_t([exception] {	std::rethrow_exception(exception);	});
		_state = async_faulted;
	}

	template <typename T>
	inline const T &async_result<T>::operator *() const
	{
		assert_readable();
		return *static_cast<const T *>(static_cast<const void *>(_buffer));
	}

	template <typename T>
	inline void async_result<T>::assert_empty() const
	{
		if (async_in_progress != _state)
			throw std::logic_error("a value/exception has already been set");
	}

	template <typename T>
	inline void async_result<T>::assert_readable() const
	{
		if (async_in_progress == _state)
			throw std::logic_error("cannot dereference as no value has been set");
		else if (async_faulted == _state)
			(*static_cast<const rethrow_t *>(static_cast<const void *>(_buffer)))();
	}


	inline void async_result<void>::set()
	{	
		assert_empty();
		_state = async_completed;
	}

	inline void async_result<void>::operator *() const
	{	assert_readable();	}
}
