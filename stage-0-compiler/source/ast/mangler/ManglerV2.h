#pragma once

#include "../ast.h"

#include <string>
#include <vector>

namespace manglerV2
{
	int mangle(int current_module_id, const ast::FunctionPrototype* proto);
	int mangle(int current_module_id, const ast::CallExpr* expr);
	int mangle(const ast::CallExpr* expr);
	int mangle(int current_module_id, int function_id, const std::vector<types::Type>& function_args);
	int add_module(int current_module_id, int other_module_id);
	int add_mangled_name(int current_module_id, int mangled_name_id);
	int mangle_using(const ast::BinaryExpr* scope_expr);
	int extract_module(int function_id);

	std::string pretty_modules(int module_id);
}
