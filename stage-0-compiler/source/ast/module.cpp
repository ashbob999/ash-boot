#include <cassert>

#include "module.h"

namespace module
{
	int StringManager::store_string(std::string& str)
	{
		if (str == "")
		{
			assert(false && "string cannot be empty");
		}

		int id = id_to_string.size();

		id_to_string.push_back(str);

		std::string_view sv{ id_to_string.back() };

		string_to_id.emplace(sv, id);

		return id;
	}

	int StringManager::get_id(std::string& str)
	{
		std::string_view sv{ str };

		auto f = string_to_id.find(sv);
		if (f != string_to_id.end())
		{
			return f->second;
		}

		return store_string(str);
	}

	bool StringManager::is_valid_id(int id)
	{
		if (id < 0 || id >= id_to_string.size())
		{
			return false;
		}
		return true;
	}

	std::string& StringManager::get_string(int id)
	{
		if (!is_valid_id(id))
		{
			assert(false && "invalid id");
		}

		//return id_to_string[id];
		auto it = id_to_string.begin();
		std::advance(it, id);
		return *it;
	}

	std::unordered_map<std::string_view, int> StringManager::string_to_id;
	std::list<std::string> StringManager::id_to_string;
}
