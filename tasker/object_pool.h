#pragma once

#include <mt/mutex.h>

#include <memory>
#include <vector>

namespace tasker
{
	template <typename T>
	class object_pool : public std::enable_shared_from_this<object_pool<T>>
	{
	private:
		class reclaimer;

	public:
		typedef std::unique_ptr<T, reclaimer> pool_ptr;

	public:
		~object_pool();

		static std::shared_ptr<object_pool> construct(std::size_t limit);
		pool_ptr allocate();

	private:
		object_pool(std::size_t limit);

		void reclaim(T *object) noexcept;

	private:
		mt::mutex _mtx;
		std::size_t _remaining;
		std::vector<T *> _pool;
	};

	template <typename T>
	class object_pool<T>::reclaimer
	{
	public:
		reclaimer() = default;
		reclaimer(std::shared_ptr<object_pool<T>> &&pool);

		void operator ()(T *object);

	private:
		std::shared_ptr<object_pool<T>> _pool;
	};

	template <typename T>
	using pool_ptr = typename object_pool<T>::pool_ptr;



	template <typename T>
	inline object_pool<T>::object_pool(std::size_t limit)
		: _remaining(limit)
	{	}

	template <typename T>
	inline object_pool<T>::~object_pool()
	{
		for (auto object : _pool)
			delete object;
	}

	template <typename T>
	inline std::shared_ptr<object_pool<T>> object_pool<T>::construct(std::size_t limit)
	{	return std::shared_ptr<object_pool>(new object_pool(limit));	}

	template <typename T>
	inline typename object_pool<T>::pool_ptr object_pool<T>::allocate()
	{
		mt::lock_guard<mt::mutex> lock(_mtx);

		if (!_pool.empty())
		{
			auto pooled = _pool.back();

			_pool.pop_back();
			return pool_ptr(pooled, reclaimer(this->shared_from_this()));
		}
		else if (_remaining)
		{
			pool_ptr pooled_new(new T(), reclaimer(this->shared_from_this()));

			_remaining--;
			return pooled_new;
		}
		return nullptr;
	}

	template <typename T>
	inline void object_pool<T>::reclaim(T *object) noexcept
	{
		mt::lock_guard<mt::mutex> lock(_mtx);
		_pool.push_back(object);
	}


	template <typename T>
	inline object_pool<T>::reclaimer::reclaimer(std::shared_ptr<object_pool<T>> &&pool)
		: _pool(std::move(pool))
	{	}

	template <typename T>
	inline void object_pool<T>::reclaimer::operator ()(T *object)
	{
		_pool->reclaim(object);
		_pool.reset();
	}
}
