#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <list>
#include <string_view>

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

	class Module
	{
	public:
		void add_module(int module_id);
		bool is_module_available(int module_id);
		int find_function(int name_id, bool is_mangled);

	public:
		std::vector<int> modules;
		std::vector<int> using_modules;
		std::vector<int> exported_functions;
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
		static std::string mangle_module(Module mod);
		static std::string mangle_function(int function_id, std::vector<types::Type>& types);
		static std::string mangle_parent_functions(ast::BaseExpr* expr);
		static int mangle(Module mod, ast::FunctionPrototype* proto);
		static int mangle(Module mod, ast::CallExpr* expr);
		static int mangle(ast::CallExpr* expr);
		static int mangle(Module mod, int function_id, std::vector<types::Type> function_args);
		static int add_module(int module_id, int other_id, bool is_first_module);
		static int get_module(ast::BinaryExpr* scope_expr);
		static mangled_data demangle(int name_id);
		static std::string remove_dots(std::string& str);
		static std::vector<int> get_mangled_functions(int function_id);

	private:
		static std::unordered_map<int, std::vector<int>> mangled_map;
	};

}
