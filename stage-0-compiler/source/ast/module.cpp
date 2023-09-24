#include <cassert>
#include <iostream>

#include "mangler.h"
#include "module.h"

namespace module
{
	int StringManager::store_string(const std::string& str)
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

	int StringManager::get_id(const std::string& str)
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

	const std::string& StringManager::get_string(int id)
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

	std::unordered_map<std::string_view, int> StringManager::string_to_id;
	std::list<std::string> StringManager::id_to_string;

	void ModuleManager::add_ast(int filename, ptr_type<ast::BodyExpr> ast_body)
	{
		ModuleManager::ast_files[filename] = std::move(ast_body);
	}

	int ModuleManager::get_file_as_module(const std::string& file_name)
	{
		return StringManager::get_id(file_name);
	}

	void ModuleManager::add_module(int filename, int module_id, std::unordered_set<int>& usings)
	{
		ModuleManager::file_modules[filename] = module_id;
		ModuleManager::module_contents[module_id].insert(filename);
		ModuleManager::file_usings[filename] = usings;
		ModuleManager::module_usings[module_id].insert(usings.begin(), usings.end());

		// remove current module from usings
		ModuleManager::file_usings[filename].erase(module_id);
		ModuleManager::module_usings[module_id].erase(module_id);

		if (ModuleManager::exported_functions.find(module_id) == ModuleManager::exported_functions.end())
		{
			ModuleManager::exported_functions[module_id] = {};
		}
	}

	bool ModuleManager::check_modules()
	{
		// check all using modules exist
		for (auto& p : ModuleManager::file_modules)
		{
			int filename = p.first;

			for (auto& v : ModuleManager::file_usings.at(filename))
			{
				auto f = std::find_if(
					ModuleManager::file_modules.begin(),
					ModuleManager::file_modules.end(),
					[&](const std::pair<int, int>& p) { return p.second == v; });

				if (f == ModuleManager::file_modules.end())
				{
					log_error(
						"Using Module '" + StringManager::get_string(v) +
						"' does not exist (in file: " + StringManager::get_string(filename) + ")");
					return false;
				}
			}
		}

		// TODO: check for circular dependencies

		return true;
	}

	bool ModuleManager::is_module_available(int filename, int module_id)
	{
		int id = ModuleManager::file_modules.at(filename);

		if (id == module_id)
		{
			return true;
		}

		for (auto& m : ModuleManager::file_usings.at(filename))
		{
			if (m == module_id)
			{
				return true;
			}
		}

		return false;
	}

	int ModuleManager::find_function(int filename, int name_id, bool is_mangled)
	{
		std::unordered_set<int>& exported_functions = get_exported_functions(filename);
		int module_id = ModuleManager::get_module(filename);

		if (is_mangled)
		{
			// check current module
			for (auto& f : exported_functions)
			{
				if (f == name_id)
				{
					return f;
				}
			}

			// check using modules
			for (auto& m : ModuleManager::file_usings.at(filename))
			{
				std::unordered_set<int>& funcs = ModuleManager::exported_functions.at(m);

				// check module funtions
				for (auto& f : funcs)
				{
					if (f == name_id)
					{
						return f;
					}
				}
			}
		}
		else
		{
			{
				// check current module
				int mangled_id = mangler::add_mangled_name(module_id, name_id);
				for (auto& f : exported_functions)
				{
					if (f == mangled_id)
					{
						return f;
					}
				}
			}

			// check using modules
			for (auto& m : ModuleManager::file_usings.at(filename))
			{
				std::unordered_set<int>& funcs = ModuleManager::exported_functions.at(m);

				int mangled_id = mangler::add_mangled_name(m, name_id);

				// check module funtions
				for (auto& f : funcs)
				{
					if (f == mangled_id)
					{
						return f;
					}
				}
			}
		}

		return -1;
	}

	std::vector<int> ModuleManager::get_matching_function_locations(int filename, int name_id)
	{
		std::vector<int> modules;

		// check using modules
		for (auto& m : ModuleManager::file_usings.at(filename))
		{
			std::unordered_set<int>& funcs = ModuleManager::exported_functions.at(m);

			int mangled_id = mangler::add_mangled_name(m, name_id);

			// check module funtions
			for (auto& f : funcs)
			{
				if (f == mangled_id)
				{
					modules.push_back(m);
					break;
				}
			}
		}

		return modules;
	}

	std::unordered_set<int>& ModuleManager::get_exported_functions(int filename)
	{
		int module_id = ModuleManager::file_modules.at(filename);
		return ModuleManager::exported_functions.at(module_id);
	}

	int ModuleManager::get_module(int filename)
	{
		return ModuleManager::file_modules.at(filename);
	}

	ast::BodyExpr* ModuleManager::get_ast(int filename)
	{
		return ModuleManager::ast_files.at(filename).get();
	}

	std::unordered_set<int> ModuleManager::find_using_modules(int module_id)
	{
		std::unordered_set<int> modules;
		for (auto& p : ModuleManager::module_usings)
		{
			if (p.second.find(module_id) != p.second.end())
			{
				modules.insert(p.first);
			}
		}
		return modules;
	}

	std::list<int> ModuleManager::get_module_order()
	{
		// do a topological sort on the modules
		// from:
		//     https://en.wikipedia.org/wiki/Topological_sorting#Algorithms
		//     https://www.techiedelight.com/kahn-topological-sort-algorithm/

		// list thta contains the topological order
		std::list<int> L;
		// set of all nodes without an incoming node
		std::unordered_set<int> S;
		std::unordered_map<int, int> indegree;

		for (auto& p : ModuleManager::module_usings)
		{
			// set the indegree for the module
			indegree[p.first] = p.second.size();

			if (p.second.size() == 0)
			{
				// add nodes with no incoming node to S
				S.insert(p.first);
			}
		}

		while (S.size() > 0)
		{
			// get the next node
			int node = *S.begin();
			// remove the node
			S.erase(node);

			// add node to tail of  L
			L.push_back(node);

			// for (auto& m : ModuleManager::module_usings.at(node))
			for (auto& m : find_using_modules(node))
			{
				// remove edge from node -> m from graph
				indegree[m] = indegree[m] - 1;

				// if m has no other incoming edges, insert m into S
				if (indegree[m] == 0)
				{
					S.insert(m);
				}
			}
		}

		// if a graph has edges, then the graph has at least one cycle
		std::unordered_set<int> circular_dependencies;
		for (auto& p : indegree)
		{
			if (p.second > 0)
			{
				circular_dependencies.insert(p.first);
			}
		}

		if (circular_dependencies.size() > 0)
		{
			handle_circular_dependencies(circular_dependencies);
			return {};
		}

		return L;
	}

	std::vector<std::pair<int, int>> ModuleManager::get_circular_dependencies()
	{
		std::vector<std::pair<int, int>> circular_dependencies;

		// find cycles in dag:
		// https://stackoverflow.com/questions/261573/best-algorithm-for-detecting-cycles-in-a-directed-graph
		std::unordered_set<int> discovered;
		std::unordered_set<int> finished;

		std::function<std::vector<std::pair<int, int>>(int)> dfs_visit;
		dfs_visit = [&](int u)
		{
			std::vector<std::pair<int, int>> found_cycles;

			discovered.insert(u);

			for (auto& v : find_using_modules(u))
			{
				if (discovered.find(v) != discovered.end())
				{
					found_cycles.push_back({ u, v });
					continue;
				}

				if (finished.find(v) == finished.end())
				{
					auto r = dfs_visit(v);
					found_cycles.insert(found_cycles.end(), r.begin(), r.end());
				}
			}

			discovered.erase(u);
			finished.insert(u);

			return found_cycles;
		};

		for (auto& p : ModuleManager::module_contents)
		{
			int u = p.first;
			auto r = dfs_visit(u);
			circular_dependencies.insert(circular_dependencies.end(), r.begin(), r.end());
		}

		return circular_dependencies;
	}

	void ModuleManager::handle_circular_dependencies(std::unordered_set<int> circular_dependencies)
	{
		log_error("Module Graph has circular dependencies:");

		auto r = get_circular_dependencies();
		for (auto p : r)
		{
			log_error(
				"\tIn module '" + StringManager::get_string(p.first) + "': requiring '" +
				StringManager::get_string(p.second) + "' creates a cycle.");
		}
	}

	std::vector<int> ModuleManager::get_build_files_order()
	{
		std::list<int> module_order = get_module_order();

		std::vector<int> files;
		for (auto& m : module_order)
		{
			std::unordered_set<int>& s = ModuleManager::module_contents.at(m);
			files.insert(files.end(), s.begin(), s.end());
		}

		return files;
	}

	ast::BodyExpr* ModuleManager::find_body(int function_id)
	{
		int module_name = mangler::extract_module(function_id);

		for (auto& file : ModuleManager::module_contents.at(module_name))
		{
			ast::BodyExpr* body = get_ast(file);

			if (body->function_prototypes.find(function_id) != body->function_prototypes.end())
			{
				return body;
			}
		}

		return nullptr;
	}

	void ModuleManager::log_error(const std::string& str)
	{
		std::cout << str << std::endl;
	}

	// filename -> module name
	std::unordered_map<int, int> ModuleManager::file_modules;
	// filename -> ast bodyexpr
	std::unordered_map<int, ptr_type<ast::BodyExpr>> ModuleManager::ast_files;
	// module name -> filenames
	std::unordered_map<int, std::unordered_set<int>> ModuleManager::module_contents;
	// filename -> usings
	std::unordered_map<int, std::unordered_set<int>> ModuleManager::file_usings;
	// module -> usings
	std::unordered_map<int, std::unordered_set<int>> ModuleManager::module_usings;
	// module name -> exported functions
	std::unordered_map<int, std::unordered_set<int>> ModuleManager::exported_functions;

}
