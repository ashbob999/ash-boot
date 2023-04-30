#pragma once

#include <memory>

template<class T>
using ptr_type = std::unique_ptr<T>;

template<class T, class... Args>
ptr_type<T> make_ptr(Args&&... args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}
