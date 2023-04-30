#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <list>
#include <string_view>
#include <list>
#include <unordered_set>

#include "../config.h"
#include "ast.h"

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

	struct mangled_data
	{
		std::string module_name;
		std::string function_name;
		std::vector<std::string> type_names;
	};

	class Mangler
	{
	public:
		static std::string mangle_function(int function_id, std::vector<types::Type>& types);
		static std::string mangle_parent_functions(ast::BaseExpr* expr);
		static int mangle(int module_id, ast::FunctionPrototype* proto);
		static int mangle(int module_id, ast::CallExpr* expr);
		static int mangle(ast::CallExpr* expr);
		static int mangle(int module_id, int function_id, std::vector<types::Type> function_args);
		static int add_module(int module_id, int other_id, bool is_first_module);
		static int get_module(ast::BinaryExpr* scope_expr);
		static mangled_data demangle(int name_id);
		static std::string remove_dots(std::string& str);
		static std::vector<int> get_mangled_functions(int function_id);
		static int extract_module(int function_id);

	private:
		static std::unordered_map<int, std::vector<int>> mangled_map;
	};

	class ModuleManager
	{
	public:
		static void add_ast(int filename, ptr_type<ast::BodyExpr> ast_body);
		static int get_file_as_module(std::string& file_name);
		static void add_module(int filename, int module_id, std::unordered_set<int>& usings);
		static bool check_modules();
		static bool is_module_available(int filename, int module_id);
		static int find_function(int filename, int name_id, bool is_mangled);
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
		static void log_error(std::string str);

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
