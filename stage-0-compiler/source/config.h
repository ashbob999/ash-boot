#pragma once

#include <memory>

template<class T>
using ptr_type = std::shared_ptr<T>;

template<class T, class... Args>
ptr_type<T> make_ptr(Args&&... args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}
