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

	struct mangled_data
	{
		std::string module_name;
		std::string function_name;
		std::vector<std::string> type_names;
	};

	class Mangler
	{
	public:
		static std::string mangle_module(int module_id);
		static std::string mangle_function(int function_id, std::vector<types::Type>& types);
		static std::string mangle_parent_functions(ast::BaseExpr* expr);
		static int mangle(int module_id, ast::FunctionPrototype* proto);
		static int mangle(int module_id, ast::CallExpr* expr);
		static int mangle(int module_id, int function_id, std::vector<types::Type> function_args);
		static mangled_data demangle(int name_id);
		static std::string remove_dots(std::string& str);
		static std::vector<int> get_mangled_functions(int function_id);

	private:
		static std::unordered_map<int, std::vector<int>> mangled_map;
	};

}
