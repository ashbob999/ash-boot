#include "string_manager.h"

#include <cassert>
#include <list>
#include <unordered_map>

namespace stringManager
{
	static std::unordered_map<std::string_view, int> string_to_id;
	static std::list<std::string> id_to_string;

	int store_string(const std::string& str);
}

int stringManager::store_string(const std::string& str)
{
	if (str == "")
	{
		assert(false && "string cannot be empty");
	}

	int id = id_to_string.size();

	id_to_string.push_back(str);

	std::string_view sv{ id_to_string.back() };

	string_to_id.emplace(sv, id);

	// std::cout << id << ": " << str << std::endl;

	return id;
}

int stringManager::get_id(const std::string& str)
{
	std::string_view sv{ str };

	auto f = string_to_id.find(sv);
	if (f != string_to_id.end())
	{
		return f->second;
	}

	return store_string(str);
}

bool stringManager::is_valid_id(int id)
{
	if (id < 0 || id >= id_to_string.size())
	{
		return false;
	}
	return true;
}

const std::string& stringManager::get_string(int id)
{
	if (!is_valid_id(id))
	{
		assert(false && "invalid id");
	}

	// return id_to_string[id];
	auto it = id_to_string.begin();
	std::advance(it, id);
	return *it;
}
