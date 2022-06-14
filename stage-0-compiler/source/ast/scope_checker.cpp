#include <iostream>

#include "scope_checker.h"

namespace scope
{
	ast::BodyExpr* get_scope(ast::CallExpr* call_expr)
	{
		auto call_body = call_expr->get_body();
		//ast::BodyExpr* body = call_expr->get_body().get();
		ast::BodyExpr* body = call_body.get();

		// TODO: deal with function overloading
		auto f = body->function_prototypes.find(call_expr->callee);
		while (body != nullptr && f == body->function_prototypes.end())
		{
			call_body = call_body->get_body();
			//body = body->get_body().get();
			body = call_body.get();

			if (body != nullptr)
			{
				f = body->function_prototypes.find(call_expr->callee);
			}
		}

		return body;
	}

	/*shared_ptr<ast::BodyExpr> get_scope(shared_ptr<ast::CallExpr> call_expr)
	{
		shared_ptr<ast::BodyExpr> body = call_expr->get_body();
		//ast::BodyExpr* body = call_expr->get_body().get();

		// TODO: deal with function overloading
		auto f = body->function_prototypes.find(call_expr->callee);
		while (body != nullptr && f == body->function_prototypes.end())
		{
			body = shared_ptr<ast::BodyExpr>(body->get_body());
			if (body != nullptr)
			{
				f = body->function_prototypes.find(call_expr->callee);
			}
		}

		return body;
	}*/

	ast::BodyExpr* get_scope(ast::VariableReferenceExpr* var_ref)
	{
		auto var_body = var_ref->get_body();
		//ast::BodyExpr* body = var_ref->get_body().get();
		ast::BodyExpr* body = var_body.get();

		auto f = body->named_types.find(var_ref->name);
		while (body != nullptr && f == body->named_types.end())
		{
			var_body = var_body->get_body();
			//body = body->get_body().get();
			body = var_body.get();
			if (body != nullptr)
			{
				f = body->named_types.find(var_ref->name);
			}
		}

		return body;
	}

	/*shared_ptr<ast::BodyExpr> get_scope(shared_ptr<ast::VariableReferenceExpr> var_ref)
	{
		shared_ptr<ast::BodyExpr> body = var_ref->get_body();

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
	}*/

	void scope_error(std::string str)
	{
		std::cout << str << std::endl;
	}

}
