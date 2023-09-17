#pragma once

#include "ast.h"

namespace scope
{
	ast::BodyExpr* get_scope(const ast::CallExpr* call_expr);
	ast::BodyExpr* get_scope(const ast::VariableReferenceExpr* var_ref);
	bool is_variable_defined(const ast::BaseExpr* expr, int name_id, ast::ReferenceType type);
	bool find_extern_function(const ast::BaseExpr* expr, int name_id);

	void scope_error(const std::string& str);
}
