#include <cassert>
#include <iostream>

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

	int Mangler::mangle(int module_id, ast::FunctionPrototype* proto)
	{
		std::string name;

		// add module name
		name += StringManager::get_string(module_id);

		name += "__";

		name += mangle_function(proto->name_id, proto->types);

		int id = StringManager::get_id(name);
		Mangler::mangled_map[proto->name_id].push_back(id);

		return id;
	}

	int Mangler::mangle(int module_id, ast::CallExpr* expr)
	{
		std::string name;

		// add module name
		name += StringManager::get_string(module_id);

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

	int Mangler::mangle(ast::CallExpr* expr)
	{
		std::string name;

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

	int Mangler::mangle(int module_id, int function_id, std::vector<types::Type> function_args)
	{
		std::string name;

		// add module name
		name += StringManager::get_string(module_id);

		name += "__";

		name += mangle_function(function_id, function_args);

		int id = StringManager::get_id(name);
		Mangler::mangled_map[function_id].push_back(id);

		return id;
	}

	int Mangler::add_module(int module_id, int other_id, bool is_first_module)
	{
		std::string name;

		name += StringManager::get_string(module_id);
		name += '_';

		if (is_first_module)
		{
			name += '_';
		}

		name += StringManager::get_string(other_id);

		return StringManager::get_id(name);
	}

	int Mangler::get_module(ast::BinaryExpr* scope_expr)
	{
		if (scope_expr->binop != operators::BinaryOp::ModuleScope)
		{
			assert(false && "Mangler::get_module, Scope Expression has invalid binop.");
		}

		std::string modules;

		// add lhs
		if (scope_expr->lhs->get_type() == ast::AstExprType::BinaryExpr)
		{
			modules += StringManager::get_string(get_module(dynamic_cast<ast::BinaryExpr*>(scope_expr->lhs.get())));
		}
		else
		{
			modules += StringManager::get_string(dynamic_cast<ast::VariableReferenceExpr*>(scope_expr->lhs.get())->name_id);
		}

		modules += '_';

		// add rhs
		modules += StringManager::get_string(dynamic_cast<ast::VariableReferenceExpr*>(scope_expr->rhs.get())->name_id);

		return StringManager::get_id(modules);
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

	int Mangler::extract_module(int function_id)
	{
		std::string module_name;

		std::string& name = StringManager::get_string(function_id);

		for (int i = 0; i < name.length(); i++)
		{
			if (name[i] == '_' && i < name.length() - 1 && name[i + 1] == '_')
			{
				break;
			}
			else
			{
				module_name += name[i];
			}
		}

		return StringManager::get_id(module_name);
	}

	std::unordered_map<int, std::vector<int>> Mangler::mangled_map;

	void ModuleManager::add_ast(int filename, shared_ptr<ast::BodyExpr> ast_body)
	{
		ModuleManager::ast_files[filename] = ast_body;
	}

	int ModuleManager::get_file_as_module(std::string& file_name)
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
				auto f = std::find_if(ModuleManager::file_modules.begin(), ModuleManager::file_modules.end(), [&](const std::pair<int, int>& p)
				{
					return p.second == v;
				});

				if (f == ModuleManager::file_modules.end())
				{
					log_error("Using Module '" + StringManager::get_string(v) + "' does not exist (in file: " + StringManager::get_string(filename) + ")");
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
				//int id = Mangler::add_module(using_id, name_id, false);

				/*if (find(exported_functions.begin(), exported_functions.end(), name) != exported_functions.end())
				{
					return id;
				}*/
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
					//int id = Mangler::add_module(using_id, name_id, false);

					if (f == name_id)
					{
						return f;
					}
				}
			}
		}
		else
		{
			// check current module
			for (auto& f : exported_functions)
			{
				int id = Mangler::add_module(module_id, name_id, true);

				if (f == id)
				{
					return id;
				}
			}
		}

		return -1;
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

			//for (auto& m : ModuleManager::module_usings.at(node))
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

		// find cycles in dag: https://stackoverflow.com/questions/261573/best-algorithm-for-detecting-cycles-in-a-directed-graph
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
			log_error("\tIn module '" + StringManager::get_string(p.first) + "': requiring '" + StringManager::get_string(p.second) + "' creates a cycle.");
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
		int module_name = Mangler::extract_module(function_id);

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

	void ModuleManager::log_error(std::string str)
	{
		std::cout << str << std::endl;
	}

	// filename -> module name
	std::unordered_map<int, int> ModuleManager::file_modules;
	// filename -> ast bodyexpr
	std::unordered_map<int, shared_ptr<ast::BodyExpr>> ModuleManager::ast_files;
	// module name -> filenames
	std::unordered_map<int, std::unordered_set<int>> ModuleManager::module_contents;
	// filename -> usings
	std::unordered_map<int, std::unordered_set<int>> ModuleManager::file_usings;
	// module -> usings
	std::unordered_map<int, std::unordered_set<int>> ModuleManager::module_usings;
	// module name -> exported functions
	std::unordered_map<int, std::unordered_set<int>> ModuleManager::exported_functions;

}
