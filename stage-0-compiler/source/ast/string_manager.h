#pragma once

#include <string>

namespace stringManager
{
	int get_id(const std::string& str);
	bool is_valid_id(int id);
	const std::string& get_string(int id);
}
