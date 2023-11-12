#pragma once

#include "./config.h"

#include <type_traits>

template<class T, class... Args>
ptr_type<T> make_ptr(Args&&... args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}

template<class T>
ptr_type<T> ptr_copy(const ptr_type<T>& ptr) noexcept
{
	if (ptr)
	{
		return make_ptr<T>(*ptr);
	}
	return ptr_type<T>{};
}

template<class T, typename std::enable_if<std::is_copy_constructible_v<T>, bool>::type = true>
class copyable_ptr_
{
public:
	copyable_ptr_(const T& value) noexcept : ptr{make_ptr<T>(value)} {}

	copyable_ptr_(const copyable_ptr_& other) : ptr{ptr_copy(other.ptr)} {}
	copyable_ptr_& operator=(copyable_ptr_ other)
	{
		std::swap(*this, other);
		return *this;
	}

	T* get() const noexcept {return this->ptr.get()};

	typename std::add_lvalue_reference<T>::type operator*() const noexcept(noexcept(*std::declval<T*>()))
	{
		return *this->ptr.get();
	}

	T* operator->() const noexcept { return this->ptr.get(); }

private:
	ptr_type<T> ptr;
};

static_assert(std::is_copy_constructible_v<copyable_ptr_<int>>);
static_assert(std::is_copy_assignable_v<copyable_ptr_<int>>);
static_assert(std::is_move_constructible_v<copyable_ptr_<int>>);
static_assert(std::is_move_assignable_v<copyable_ptr_<int>>);

template<class T>
using copyable_ptr = std::conditional_t<std::is_copy_constructible_v<ptr_type<T>>, ptr_type<T>, copyable_ptr_<T>>;

template<class F>
class ScopeExit
{
public:
	explicit ScopeExit(F&& func) : function{func} {}
	~ScopeExit() { function(); }

	ScopeExit(const ScopeExit&) = delete;
	ScopeExit& operator=(const ScopeExit&) = delete;

	ScopeExit(ScopeExit&& other) : function{other.function} {}

private:
	F function;
};
