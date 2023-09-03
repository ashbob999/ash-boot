#pragma once

#include "ast.h"

#include <vector>
#include <string>

namespace mangler
{
	struct mangled_data
	{
		std::string module_name;
		std::string function_name;
		std::vector<std::string> type_names;
	};

	class ManglerV1
	{
	public:
		static int mangle(int module_id, ast::FunctionPrototype* proto);
		static int mangle(int module_id, ast::CallExpr* expr);
		static int mangle(ast::CallExpr* expr);
		static int mangle(int module_id, int function_id, std::vector<types::Type> function_args);
		static int add_module(int module_id, int other_module_id);
		static int add_mangled_name(int module_id, int mangled_name);
		static int mangle_using(ast::BinaryExpr* scope_expr);
		static int extract_module(int function_id);

	private:
		static std::string mangle_function(int function_id, std::vector<types::Type>& types);
		static std::string mangle_call(ast::CallExpr* expr);
	};

	class ManglerV2
	{
	public:
		static int mangle(int current_module_id, ast::FunctionPrototype* proto);
		static int mangle(int current_module_id, ast::CallExpr* expr);
		static int mangle(ast::CallExpr* expr);
		static int mangle(int current_module_id, int function_id, std::vector<types::Type> function_args);
		static int add_module(int current_module_id, int other_module_id);
		static int add_mangled_name(int current_module_id, int mangled_name_id);
		static int mangle_using(ast::BinaryExpr* scope_expr);
		static int extract_module(int function_id);

	private:
	public:
		static constexpr const char* StartString = "_AS_";
		static constexpr size_t StartStringSize = 4;
		static std::string remove_start_string(const std::string& string);
		static std::string get_name_or_start(int name_id);
		static std::string mangle_function(int function_id, std::vector<types::Type>& types);
		static std::string mangle_type(types::Type& type);
		static std::string mangle_call(ast::CallExpr* expr);
	};

	using Mangler = ManglerV2;
}
