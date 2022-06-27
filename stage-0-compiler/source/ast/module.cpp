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

	std::string Mangler::mangle_module(Module mod)
	{
		std::string name;

		if (mod.modules.size() > 0)
		{
			for (int i = 0; i < mod.modules.size() - 1; i++)
			{
				name += StringManager::get_string(mod.modules[i]);
				name += '_';
			}
			name += StringManager::get_string(mod.modules.back());
		}

		return name;
	}

	std::string Mangler::mangle_function(int function_id, std::vector<types::Type>& types)
	{
		std::string name;

		// add function name
		name += StringManager::get_string(function_id);

		name += "__";

		// add function args
		if (types.size() > 0)
		{
			for (int i = 0; i < types.size() - 1; i++)
			{
				name += types::to_string(types[i]);
				name += '_';
			}

			name += types::to_string(types.back());
		}

		return name;
	}

	int Mangler::mangle(Module mod, ast::FunctionPrototype* proto)
	{
		std::string name;

		// add module name
		name += mangle_module(mod);

		name += "__";

		name += mangle_function(proto->name_id, proto->types);

		int id = StringManager::get_id(name);
		Mangler::mangled_map[proto->name_id].push_back(id);

		return id;
	}

	int Mangler::mangle(Module mod, ast::CallExpr* expr)
	{
		std::string name;

		// add module name
		name += mangle_module(mod);

		name += "__";

		std::vector<types::Type> types;
		for (auto& e : expr->args)
		{
			types.push_back(e->get_result_type());
		}

		name += mangle_function(expr->callee_id, types);

		int id = StringManager::get_id(name);
		Mangler::mangled_map[expr->callee_id].push_back(id);

		return id;
	}

	int Mangler::mangle(Module mod, int function_id, std::vector<types::Type> function_args)
	{
		std::string name;

		// add module name
		name += mangle_module(mod);

		name += "__";

		name += mangle_function(function_id, function_args);

		int id = StringManager::get_id(name);
		Mangler::mangled_map[function_id].push_back(id);

		return id;
	}

	mangled_data Mangler::demangle(int name_id)
	{
		std::string& name = StringManager::get_string(name_id);

		mangled_data data;

		int i = 0;
		for (; i < name.length() - 1; i++)
		{
			if (name[i] == '_' && name[i + 1] == '_')
			{
				i += 2;
				break;
			}
			else
			{
				data.module_name += name[i];
			}
		}

		for (; i < name.length() - 1; i++)
		{
			if (name[i] == '_' && name[i + 1] == '_')
			{
				i += 2;
				break;
			}
			else
			{
				data.function_name += name[i];
			}
		}

		std::string type;

		for (; i < name.length() - 1; i++)
		{
			if (name[i] == '_')
			{
				i++;
				data.type_names.push_back(type);
				type = "";
				break;
			}
			else
			{
				type += name[i];
			}
		}

		data.type_names.push_back(type);

		return data;
	}

	std::string Mangler::remove_dots(std::string& str)
	{
		std::string s;

		for (auto& c : str)
		{
			if (c == '.')
			{
				break;
			}
			s += c;
		}

		return s;
	}

	std::vector<int> Mangler::get_mangled_functions(int function_id)
	{
		return Mangler::mangled_map[function_id];
	}

	std::unordered_map<int, std::vector<int>> Mangler::mangled_map;

	void Module::add_module(int module_id)
	{
		this->modules.push_back(module_id);
	}

}
