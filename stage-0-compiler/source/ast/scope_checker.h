#pragma once

#include "ast.h"

namespace scope
{
	ast::BodyExpr* get_scope(ast::CallExpr* call_expr);
	ast::BodyExpr* get_scope(ast::VariableReferenceExpr* var_ref);
	bool is_variable_defined(ast::BaseExpr* expr, std::string name, ast::ReferenceType type);

	void scope_error(std::string str);
}
