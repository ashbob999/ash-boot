#include "module_manager.h"

#include "mangler.h"
#include "string_manager.h"

#include <iostream>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace moduleManager
{
	// filename -> module name
	static std::unordered_map<int, int> file_modules;
	// filename -> ast bodyexpr
	static std::unordered_map<int, ptr_type<ast::BodyExpr>> ast_files;
	// module name -> filenames
	static std::unordered_map<int, std::unordered_set<int>> module_contents;
	// filename -> usings
	static std::unordered_map<int, std::unordered_set<int>> file_usings;
	// module -> usings
	static std::unordered_map<int, std::unordered_set<int>> module_usings;
	// module name -> exported functions
	static std::unordered_map<int, std::unordered_set<int>> exported_functions;

	std::unordered_set<int> find_using_modules(int module_id);
	std::list<int> get_module_order();
	std::vector<std::pair<int, int>> get_circular_dependencies();
	void handle_circular_dependencies(std::unordered_set<int> circular_dependencies);
	void log_error(const std::string& str);
}

void moduleManager::add_ast(int filename, ptr_type<ast::BodyExpr> ast_body)
{
	moduleManager::ast_files[filename] = std::move(ast_body);
}

int moduleManager::get_file_as_module(const std::string& file_name)
{
	return stringManager::get_id(file_name);
}

void moduleManager::add_module(int filename, int module_id, std::unordered_set<int>& usings)
{
	file_modules[filename] = module_id;
	module_contents[module_id].insert(filename);
	file_usings[filename] = usings;
	module_usings[module_id].insert(usings.begin(), usings.end());

	// remove current module from usings
	file_usings[filename].erase(module_id);
	module_usings[module_id].erase(module_id);

	if (exported_functions.find(module_id) == exported_functions.end())
	{
		exported_functions[module_id] = {};
	}
}

bool moduleManager::check_modules()
{
	// check all using modules exist
	for (auto& p : file_modules)
	{
		int filename = p.first;

		for (auto& v : file_usings.at(filename))
		{
			auto f = std::find_if(
				file_modules.begin(),
				file_modules.end(),
				[&](const std::pair<int, int>& p) { return p.second == v; });

			if (f == file_modules.end())
			{
				log_error(
					"Using Module '" + stringManager::get_string(v) +
					"' does not exist (in file: " + stringManager::get_string(filename) + ")");
				return false;
			}
		}
	}

	// TODO: check for circular dependencies

	return true;
}

bool moduleManager::is_module_available(int filename, int module_id)
{
	int id = file_modules.at(filename);

	if (id == module_id)
	{
		return true;
	}

	for (auto& m : file_usings.at(filename))
	{
		if (m == module_id)
		{
			return true;
		}
	}

	return false;
}

int moduleManager::find_function(int filename, int name_id, bool is_mangled)
{
	std::unordered_set<int>& exported_functions = get_exported_functions(filename);
	int module_id = get_module(filename);

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
		for (auto& m : file_usings.at(filename))
		{
			std::unordered_set<int>& funcs = moduleManager::exported_functions.at(m);

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
		for (auto& m : file_usings.at(filename))
		{
			std::unordered_set<int>& funcs = moduleManager::exported_functions.at(m);

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

std::vector<int> moduleManager::get_matching_function_locations(int filename, int name_id)
{
	std::vector<int> modules;

	// check using modules
	for (auto& m : file_usings.at(filename))
	{
		std::unordered_set<int>& funcs = exported_functions.at(m);

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

std::unordered_set<int>& moduleManager::get_exported_functions(int filename)
{
	int module_id = file_modules.at(filename);
	return exported_functions.at(module_id);
}

int moduleManager::get_module(int filename)
{
	return file_modules.at(filename);
}

ast::BodyExpr* moduleManager::get_ast(int filename)
{
	return ast_files.at(filename).get();
}

std::unordered_set<int> moduleManager::find_using_modules(int module_id)
{
	std::unordered_set<int> modules;
	for (auto& p : module_usings)
	{
		if (p.second.find(module_id) != p.second.end())
		{
			modules.insert(p.first);
		}
	}
	return modules;
}

std::list<int> moduleManager::get_module_order()
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

	for (auto& p : module_usings)
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

		// for (auto& m : module_usings.at(node))
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

std::vector<std::pair<int, int>> moduleManager::get_circular_dependencies()
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

	for (auto& p : module_contents)
	{
		int u = p.first;
		auto r = dfs_visit(u);
		circular_dependencies.insert(circular_dependencies.end(), r.begin(), r.end());
	}

	return circular_dependencies;
}

void moduleManager::handle_circular_dependencies(std::unordered_set<int> circular_dependencies)
{
	log_error("Module Graph has circular dependencies:");

	auto r = get_circular_dependencies();
	for (auto p : r)
	{
		log_error(
			"\tIn module '" + stringManager::get_string(p.first) + "': requiring '" +
			stringManager::get_string(p.second) + "' creates a cycle.");
	}
}

std::vector<int> moduleManager::get_build_files_order()
{
	std::list<int> module_order = get_module_order();

	std::vector<int> files;
	for (auto& m : module_order)
	{
		std::unordered_set<int>& s = module_contents.at(m);
		files.insert(files.end(), s.begin(), s.end());
	}

	return files;
}

ast::BodyExpr* moduleManager::find_body(int function_id)
{
	int module_name = mangler::extract_module(function_id);

	for (auto& file : module_contents.at(module_name))
	{
		ast::BodyExpr* body = get_ast(file);

		if (body->function_prototypes.find(function_id) != body->function_prototypes.end())
		{
			return body;
		}
	}

	return nullptr;
}

void moduleManager::log_error(const std::string& str)
{
	std::cout << str << std::endl;
}
