#include <iostream>

#include "scope_checker.h"

namespace scope
{
	ast::BodyExpr* get_scope(ast::CallExpr* call_expr)
	{
		ast::BodyExpr* body = call_expr->get_body();

		// TODO: deal with function overloading
		auto f = body->function_prototypes.find(call_expr->callee);
		while (body != nullptr && f == body->function_prototypes.end())
		{
			body = body->get_body();
			if (body != nullptr)
			{
				f = body->function_prototypes.find(call_expr->callee);
			}
		}

		return body;
	}

	ast::BodyExpr* get_scope(ast::VariableReferenceExpr* var_ref)
	{
		ast::BodyExpr* body = var_ref->get_body();

		auto f = body->named_types.find(var_ref->name);
		while (body != nullptr && f == body->named_types.end())
		{
			body = body->get_body();
			if (body != nullptr)
			{
				f = body->named_types.find(var_ref->name);
			}
		}

		return body;
	}

	void scope_error(std::string str)
	{
		std::cout << str << std::endl;
	}

}
