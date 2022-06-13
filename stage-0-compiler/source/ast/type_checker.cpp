#include <iostream>

#include "type_checker.h"
#include "scope_checker.h"

namespace type_checker
{
	TypeChecker::TypeChecker()
	{}

	bool TypeChecker::check_types(ast::BaseExpr* body)
	{
		return check_expression_dispatch(body);
	}

	bool TypeChecker::log_error(std::string str)
	{
		std::cout << str << std::endl;
		return false;
	}

	bool TypeChecker::check_function(ast::FunctionDefinition* func)
	{
		/*
		* function definition
		if (!func_def->check_return_type())
			{
				end_;
				return log_error_definition("function definition has invalid return type");
			}
		*/

		// check function body
		if (!check_expression_dispatch(func->body))
		{
			return log_error("function body has invalid types");
		}

		// check function return type
		if (!func->check_return_type())
		{
			return log_error("function definition has invalid return type");
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::LiteralExpr>(ast::LiteralExpr* expr)
	{
		/*
		* literal
		if (!literal->check_types())
			{
				end_;
				return log_error("literal type invalid");
			}
		*/

		if (!expr->check_types())
		{
			return log_error("literal type invalid");
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::BodyExpr>(ast::BodyExpr* body)
	{
		// check each function
		for (auto& f : body->functions)
		{
			if (!check_function(f))
			{
				return false;
			}
		}

		// check each expression
		for (auto& e : body->expressions)
		{
			if (!check_expression_dispatch(e))
			{
				return false;
			}
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::VariableDeclarationExpr>(ast::VariableDeclarationExpr* expr)
	{
		// check value expression
		if (expr->expr != nullptr && !check_expression_dispatch(expr->expr))
		{
			return log_error("variable declaration value failed type checks");
		}

		/*
		* variable delcaration
		if (!var->check_types())
			{
				end_;
				return log_error("variable declaration expected type: " + types::to_string(var->curr_type) + " but got type: " + types::to_string(var->expr->get_result_type()));
			}
		*/

		if (!expr->check_types())
		{
			return log_error("variable declaration expected type: " + types::to_string(expr->curr_type) + " but got type: " + types::to_string(expr->expr->get_result_type()));
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::VariableReferenceExpr>(ast::VariableReferenceExpr* expr)
	{
		// check variable is in scope
		if (scope::get_scope(expr) == nullptr)
		{
			return log_error("variable being referenced is not in scope");
		}

		/*
		* variable reference
		if (!var->check_types())
				{
					end_;
					return log_error("variable reference type error");
				}
		*/

		if (!expr->check_types())
		{
			return log_error("variable reference type error");
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::BinaryExpr>(ast::BinaryExpr* expr)
	{
		// check lhs
		if (!check_expression_dispatch(expr->lhs))
		{
			return log_error("binary op lhs type check failed");
		}

		// check rhs
		if (!check_expression_dispatch(expr->rhs))
		{
			return log_error("binary op rhs type check failed");
		}

		/*
		* binop
		if (!lhs->check_types())
				{
					end_;
					return log_error("binop incompatible types: " + types::to_string(lhs_->lhs->get_result_type()) + " and " + types::to_string(lhs_->rhs->get_result_type()));
				}
		*/

		if (!expr->check_types())
		{
			return log_error("binop incompatible types: " + types::to_string(expr->lhs->get_result_type()) + " and " + types::to_string(expr->rhs->get_result_type()));
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::CallExpr>(ast::CallExpr* expr)
	{
		// check function is in scope
		if (scope::get_scope(expr) == nullptr)
		{
			return log_error("function being called is not in scope");
		}

		/*
		* call expr
		if (!call->check_types())
			{
				end_;
				return log_error("call expression has arguments of invalid types");
			}
		*/

		if (!expr->check_types())
		{
			return log_error("call expression has arguments of invalid types");
		}

		return true;
	}

	bool TypeChecker::check_expression_dispatch(ast::BaseExpr* expr)
	{
		switch (expr->get_type())
		{
			case ast::AstExprType::LiteralExpr:
			{
				return check_expression(dynamic_cast<ast::LiteralExpr*>(expr));
			}
			case ast::AstExprType::BodyExpr:
			{
				return check_expression(dynamic_cast<ast::BodyExpr*>(expr));
			}
			case ast::AstExprType::VariableDeclarationExpr:
			{
				return check_expression(dynamic_cast<ast::VariableDeclarationExpr*>(expr));
			}
			case ast::AstExprType::VariableReferenceExpr:
			{
				return check_expression(dynamic_cast<ast::VariableReferenceExpr*>(expr));
			}
			case ast::AstExprType::BinaryExpr:
			{
				return check_expression(dynamic_cast<ast::BinaryExpr*>(expr));
			}
			case ast::AstExprType::CallExpr:
			{
				return check_expression(dynamic_cast<ast::CallExpr*>(expr));
			}
		}
		return false;
	}

}
