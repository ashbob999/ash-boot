#include <iostream>

#include "module_manager.h"
#include "scope_checker.h"

namespace scope
{
	ast::BodyExpr* get_scope(const ast::CallExpr* call_expr)
	{
		// checks current function, and then all the parents
		ast::BodyExpr* body = call_expr->get_body();

		// TODO: deal with function overloading
		auto f = body->function_prototypes.find(call_expr->callee_id);
		while (body != nullptr && f == body->function_prototypes.end())
		{
			body = body->get_body();

			if (body != nullptr)
			{
				f = body->function_prototypes.find(call_expr->callee_id);
			}
		}

		if (body != nullptr)
		{
			return body;
		}

		// check using modules
		return moduleManager::find_body(call_expr->callee_id);
	}

	ast::BodyExpr* get_scope(const ast::VariableReferenceExpr* var_ref)
	{
		ast::BodyExpr* body = var_ref->get_body();

		auto f = body->named_types.find(var_ref->name_id);
		while (body != nullptr && f == body->named_types.end())
		{
			body = body->get_body();

			if (body != nullptr)
			{
				f = body->named_types.find(var_ref->name_id);
			}
		}

		return body;
	}

	bool is_variable_defined(const ast::BaseExpr* expr, int name_id, ast::ReferenceType type)
	{
		// TODO: handle generic functions and overloading

		const ast::BodyExpr* body = nullptr;

		if (expr->get_type() == ast::AstExprType::BodyExpr)
		{
			body = dynamic_cast<const ast::BodyExpr*>(expr);
		}
		else
		{
			body = expr->get_body();
		}

		if (body == nullptr)
		{
			return false;
		}

		std::pair<int, ast::ReferenceType> p{ name_id, type };

		auto f = std::find(body->in_scope_vars.begin(), body->in_scope_vars.end(), p);
		while (body != nullptr &&
			   (type == ast::ReferenceType::Function || body->body_type != ast::BodyType::Function) &&
			   f == body->in_scope_vars.end())
		{
			body = body->get_body();

			if (body != nullptr)
			{
				f = std::find(body->in_scope_vars.begin(), body->in_scope_vars.end(), p);
			}
		}

		if (body == nullptr)
		{
			return false;
		}

		if (f == body->in_scope_vars.end())
		{
			return false;
		}

		return true;
	}

	bool find_extern_function(const ast::BaseExpr* expr, int name_id)
	{
		const ast::BodyExpr* body = nullptr;

		if (expr->get_type() == ast::AstExprType::BodyExpr)
		{
			body = dynamic_cast<const ast::BodyExpr*>(expr);
		}
		else
		{
			body = expr->get_body();
		}

		if (body == nullptr)
		{
			return false;
		}

		auto f = std::find(body->extern_functions.begin(), body->extern_functions.end(), name_id);
		while (body != nullptr && f == body->extern_functions.end())
		{
			body = body->get_body();

			if (body != nullptr)
			{
				f = std::find(body->extern_functions.begin(), body->extern_functions.end(), name_id);
			}
		}

		if (body == nullptr)
		{
			return false;
		}

		if (f == body->extern_functions.end())
		{
			return false;
		}

		return true;
	}

	void scope_error(const std::string& str)
	{
		std::cout << str << std::endl;
	}

}
