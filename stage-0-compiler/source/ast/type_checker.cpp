#include <iostream>
#include <iomanip>

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
		// check function body
		if (!check_expression_dispatch(func->body.get()))
		{
			return false;
		}

		// check function return type
		if (!func->check_return_type())
		{
			return log_error(func->body->expressions.back().get(), "function definition has invalid return type");
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
			return log_error(expr, "Variable declaration expression for: " + expr->name + ", expected type: " + type1 + " but got type: " + type2 + " instead");
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::VariableReferenceExpr>(ast::VariableReferenceExpr* expr)
	{
		// check variable is in scope
		if (scope::get_scope(expr) == nullptr)
		{
			return log_error(expr, "Variable reference for: " + expr->name + ", is not in scope");
		}

		// chekc variable type
		if (!expr->check_types())
		{
			return log_error(expr, "Variable reference for: " + expr->name + ", has invalid type");
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

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::CallExpr>(ast::CallExpr* expr)
	{
		// check function is in scope
		if (scope::get_scope(expr) == nullptr)
		{
			return log_error(expr, "Function call for: " + expr->callee + ", is not in scope");
		}

		// check call arguments
		if (!expr->check_types())
		{
			// TODO: make better
			return log_error(expr, "Call expression has arguments of invalid types");
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
