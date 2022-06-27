#pragma once

#include "ast.h"

namespace scope
{
	ast::BodyExpr* get_scope(ast::CallExpr* call_expr);
	ast::BodyExpr* get_scope(ast::VariableReferenceExpr* var_ref);
	bool is_variable_defined(ast::BaseExpr* expr, int name_id, ast::ReferenceType type);
	bool find_extern_function(ast::BaseExpr* expr, int name_id);

	void scope_error(std::string str);
}
