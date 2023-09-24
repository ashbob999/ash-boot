#pragma once

#include "../ast.h"

#include <vector>

namespace [[deprecated(
	"ManglerV1 is deprecated, and won't work with newer language constructs. Use ManglerV2 instead.")]] manglerV1
{
	int mangle(int module_id, const ast::FunctionPrototype* proto);
	int mangle(int module_id, const ast::CallExpr* expr);
	int mangle(const ast::CallExpr* expr);
	int mangle(int module_id, int function_id, const std::vector<types::Type>& function_args);
	int add_module(int module_id, int other_module_id);
	int add_mangled_name(int module_id, int mangled_name);
	int mangle_using(const ast::BinaryExpr* scope_expr);
	int extract_module(int function_id);
}
