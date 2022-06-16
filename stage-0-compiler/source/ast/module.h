#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <list>
#include <string_view>

namespace module
{
	class StringManager
	{
	public:
		static int get_id(std::string& str);
		static bool is_valid_id(int id);
		static std::string& get_string(int id);
	private:
		static int store_string(std::string& str);
	private:
		static std::unordered_map<std::string_view, int> string_to_id;
		static std::list<std::string> id_to_string;
	};
}
