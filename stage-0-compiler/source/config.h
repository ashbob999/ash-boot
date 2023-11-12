#pragma once

#include <memory>

template<class T>
using ptr_type = std::unique_ptr<T>;
