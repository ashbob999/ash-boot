#include <iostream>
#include <iomanip>

#include "type_checker.h"
#include "scope_checker.h"
#include "module.h"

namespace type_checker
{
	TypeChecker::TypeChecker()
	{}

	bool TypeChecker::check_types(ast::BaseExpr* body)
	{
		return check_expression_dispatch(body);
	}

	bool TypeChecker::log_error(ast::BaseExpr* expr, std::string str)
	{
		std::cout << str << std::endl;

		/*
		// TODO: use when file is read into char array
		ast::ExpressionLineInfo& line_info = expr->get_line_info();

		std::string lines;
		std::string positions;

		if (line_info.start_line == line_info.end_line)
		{
			lines += "At Line: ";
			lines += std::to_string(line_info.start_line);
		}
		else
		{
			lines += "At Lines: ";
			lines += std::to_string(line_info.start_line);
			lines += '-';
			lines += std::to_string(line_info.end_line);
		}

		if (line_info.start_pos == line_info.end_pos)
		{
			positions += "At Position: ";
			positions += std::to_string(line_info.start_pos);
		}
		else
		{
			positions += "At Positions: ";
			positions += std::to_string(line_info.start_pos);
			positions += '-';
			positions += std::to_string(line_info.end_pos);
		}

		std::cout << lines << ' ' << positions << std::endl;
		std::cout << '\t' << line_info.line << std::endl;
		std::cout << std::endl;
		*/
		return false;

	}

	bool TypeChecker::check_function(ast::FunctionDefinition* func)
	{
		// add args to scope
		for (auto& arg : func->prototype->args)
		{
			func->body->in_scope_vars.push_back({ arg, ast::ReferenceType::Variable });
		}

		// check function body
		if (!check_expression_dispatch(func->body.get()))
		{
			return false;
		}

		// check function return type
		if (!func->check_return_type())
		{
			if (func->body->expressions.size() == 0)
			{
				return log_error(nullptr, "function definition has invalid return type");
			}
			else
			{
				return log_error(func->body->expressions.back().get(), "function definition has invalid return type");
			}
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::LiteralExpr>(ast::LiteralExpr* expr)
	{
		// check the literal type
		if (!expr->check_types())
		{
			return log_error(expr, "Literal has invalid type");
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::BodyExpr>(ast::BodyExpr* body)
	{
		// check all function prototypes to make sure there are no redefinitions
		for (auto& p : body->function_prototypes)
		{
			if (scope::is_variable_defined(body, p.first, ast::ReferenceType::Function))
			{
				return log_error(body, "Function: " + module::StringManager::get_string(p.first) + ", is already defined");
			}

			body->in_scope_vars.push_back({ p.first, ast::ReferenceType::Function });
		}

		// check each function
		for (auto& f : body->functions)
		{
			if (!check_function(f.get()))
			{
				return false;
			}
		}

		// check each expression
		for (auto& e : body->expressions)
		{
			if (!check_expression_dispatch(e.get()))
			{
				return false;
			}
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::VariableDeclarationExpr>(ast::VariableDeclarationExpr* expr)
	{
		// check to see if we are redefining the variable, but only in the current scope
		for (auto& p : expr->get_body()->in_scope_vars)
		{
			if (p.second == ast::ReferenceType::Variable)
			{
				if (p.first == expr->name_id)
				{
					return log_error(expr, "Variable: " + module::StringManager::get_string(expr->name_id) + ", has already been defined");
				}
			}
		}

		expr->get_body()->in_scope_vars.push_back({ expr->name_id, ast::ReferenceType::Variable });

		// check value expression
		if (expr->expr != nullptr && !check_expression_dispatch(expr->expr.get()))
		{
			return false;
		}

		// check the expression result type
		if (!expr->check_types())
		{
			std::string type1 = types::to_string(expr->curr_type);
			std::string type2 = types::to_string(expr->expr->get_result_type());
			return log_error(expr, "Variable declaration expression for: " + module::StringManager::get_string(expr->name_id) + ", expected type: " + type1 + " but got type: " + type2 + " instead");
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::VariableReferenceExpr>(ast::VariableReferenceExpr* expr)
	{
		// check variable has already been defined
		if (!scope::is_variable_defined(expr, expr->name_id, ast::ReferenceType::Variable))
		{
			return log_error(expr, "Variable reference for: " + module::StringManager::get_string(expr->name_id) + ", is not in scope (not defined)");
		}

		// check variable is in scope
		if (scope::get_scope(expr) == nullptr)
		{
			return log_error(expr, "Variable reference for: " + module::StringManager::get_string(expr->name_id) + ", is not in scope");
		}

		// chekc variable type
		if (!expr->check_types())
		{
			return log_error(expr, "Variable reference for: " + module::StringManager::get_string(expr->name_id) + ", has invalid type");
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::BinaryExpr>(ast::BinaryExpr* expr)
	{
		// check lhs
		if (!check_expression_dispatch(expr->lhs.get()))
		{
			return false;
		}

		// check rhs
		if (!check_expression_dispatch(expr->rhs.get()))
		{
			return false;
		}

		// check both sides of the binary operator have the same type, type is given by lhs
		if (!expr->check_types())
		{
			std::string type1 = types::to_string(expr->lhs->get_result_type());
			std::string type2 = types::to_string(expr->rhs->get_result_type());
			return log_error(expr, "Binary operator " + ast::to_string(expr->binop) + " has incompatible types: " + type1 + " and " + type2 + " given");
		}

		// check type supports the specified operation
		if (!ast::is_type_supported(expr->binop, expr->get_result_type()))
		{
			std::string type = types::to_string(expr->rhs->get_result_type());
			return log_error(expr, "Binary operator " + ast::to_string(expr->binop) + " does not support the given type: " + type);
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::CallExpr>(ast::CallExpr* expr)
	{
		// check function has been defined
		if (!scope::is_variable_defined(expr, expr->callee_id, ast::ReferenceType::Function))
		{
			return log_error(expr, "Function call for: " + module::StringManager::get_string(expr->callee_id) + ", is not in scope (not defined)");
		}

		// check function is in scope
		if (scope::get_scope(expr) == nullptr)
		{
			return log_error(expr, "Function call for: " + module::StringManager::get_string(expr->callee_id) + ", is not in scope");
		}

		// check call arguments
		if (!expr->check_types())
		{
			// TODO: make better
			return log_error(expr, "Call expression has arguments of invalid types");
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::IfExpr>(ast::IfExpr* expr)
	{

		// check the condition
		if (!check_expression_dispatch(expr->condition.get()))
		{
			return false;
		}

		// check the condition has type bool
		if (expr->condition->get_result_type() != types::Type::Bool)
		{
			return log_error(expr, "If condition must have type bool");
		}

		// check if body
		if (!check_expression_dispatch(expr->if_body.get()))
		{
			return false;
		}

		// check else body
		if (!check_expression_dispatch(expr->else_body.get()))
		{
			return false;
		}

		// check both bodies have the same result type, type iof given by the if body
		if (!expr->check_types())
		{
			std::string type1 = types::to_string(expr->if_body->get_result_type());
			std::string type2 = types::to_string(expr->else_body->get_result_type());
			return log_error(expr, "If Statement has incompatible result types: " + type1 + " and " + type2 + " given");
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::ForExpr>(ast::ForExpr* expr)
	{
		// add variable to for body scope
		expr->for_body->in_scope_vars.push_back({ expr->name_id, ast::ReferenceType::Variable });

		// check the start expression
		if (!check_expression_dispatch(expr->start_expr.get()))
		{
			return false;
		}

		// check the variable has correct type
		if (expr->var_type != expr->start_expr->get_result_type())
		{
			std::string type1 = types::to_string(expr->var_type);
			std::string type2 = types::to_string(expr->start_expr->get_result_type());
			return log_error(expr, "for start expression has invalid type, expected type: " + type1 + ", but got type: " + type2 + " instead");
		}

		// check the end condition
		if (!check_expression_dispatch(expr->end_expr.get()))
		{
			return false;
		}

		// check the end condition has type bool
		if (expr->end_expr->get_result_type() != types::Type::Bool)
		{
			return log_error(expr, "For end condition must have type bool");
		}

		// check the step expression
		if (expr->step_expr != nullptr && !check_expression_dispatch(expr->step_expr.get()))
		{
			return nullptr;
		}

		// check for body
		if (!check_expression_dispatch(expr->for_body.get()))
		{
			return nullptr;
		}

		// check for
		if (!expr->check_types())
		{
			return log_error(expr, "For end condition must have type bool (for check types)");
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::CommentExpr>(ast::CommentExpr* expr)
	{
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
			case ast::AstExprType::IfExpr:
			{
				return check_expression(dynamic_cast<ast::IfExpr*>(expr));
			}
			case ast::AstExprType::ForExpr:
			{
				return check_expression(dynamic_cast<ast::ForExpr*>(expr));
			}
			case ast::AstExprType::CommentExpr:
			{
				return check_expression(dynamic_cast<ast::CommentExpr*>(expr));
			}
		}
		assert(false && "Missing Type Specialisation");
		return false;
	}

}
