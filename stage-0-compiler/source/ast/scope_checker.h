#pragma once

#include "ast.h"

namespace scope
{
	ast::BodyExpr* get_scope(ast::CallExpr* call_expr);
	//shared_ptr<ast::BodyExpr> get_scope(shared_ptr<ast::CallExpr> call_expr);
	ast::BodyExpr* get_scope(ast::VariableReferenceExpr* var_ref);
	//shared_ptr<ast::BodyExpr> get_scope(shared_ptr<ast::VariableReferenceExpr> var_ref);

	void scope_error(std::string str);
}