#pragma once

#include <list>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../config.h"
#include "ast.h"

namespace module
{
	class ModuleManager
	{
	public:
		static void add_ast(int filename, ptr_type<ast::BodyExpr> ast_body);
		static int get_file_as_module(const std::string& file_name);
		static void add_module(int filename, int module_id, std::unordered_set<int>& usings);
		static bool check_modules();
		static bool is_module_available(int filename, int module_id);
		static int find_function(int filename, int name_id, bool is_mangled);
		static std::vector<int> get_matching_function_locations(int filename, int name_id);
		static std::unordered_set<int>& get_exported_functions(int filename);
		static int get_module(int filename);
		static ast::BodyExpr* get_ast(int filename);
		static std::vector<int> get_build_files_order();
		static ast::BodyExpr* find_body(int function_id);

	private:
		static std::unordered_set<int> find_using_modules(int module_id);
		static std::list<int> get_module_order();
		static std::vector<std::pair<int, int>> get_circular_dependencies();
		static void handle_circular_dependencies(std::unordered_set<int> circular_dependencies);
		static void log_error(const std::string& str);

	private:
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
	};

}
