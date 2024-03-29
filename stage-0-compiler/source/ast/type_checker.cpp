#include <iomanip>
#include <iostream>

#include "../utils.h"
#include "constant_checker.h"
#include "mangler.h"
#include "scope_checker.h"
#include "string_manager.h"
#include "type_checker.h"

namespace type_checker
{
	TypeChecker::TypeChecker() {}

	bool TypeChecker::check_types(ast::BaseExpr* body) const
	{
		return check_expression_dispatch(body);
	}

	bool TypeChecker::check_prototypes(ast::BodyExpr* body) const
	{
		// check and add exported functions
		if (body->get_body() == nullptr)
		{
			for (auto& proto : body->original_function_prototypes)
			{
				// if function is main with no args then don't mangle, but only on top level bodies
				if (body->get_body() == nullptr)
				{
					std::string main = "main";
					int main_id = stringManager::get_id(main);

					if (proto->name_id == main_id && proto->args.size() == 0)
					{
						continue;
					}
				}

				int id = mangler::mangle(moduleManager::get_module(this->current_file_id), proto);

				int func_id = moduleManager::find_function(this->current_file_id, id, true);
				if (func_id != -1)
				{
					log_error(body, "Function: " + stringManager::get_string(func_id) + ", is already defined.");
					return false;
				}
				moduleManager::get_exported_functions(this->current_file_id).insert(id);
				body->function_prototypes.insert({id, proto});
			}
		}

		return true;
	}

	void TypeChecker::set_file_id(int file_id)
	{
		this->current_file_id = file_id;
	}

	bool TypeChecker::log_error(const ast::BaseExpr* expr, const std::string& str) const
	{
		std::cout << str << std::endl;
		std::cout << '\t' << "In File: " << stringManager::get_string(this->current_file_id) << std::endl;

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

	bool TypeChecker::check_function(ast::FunctionDefinition* func) const
	{
		// add args to scope
		for (auto& arg : func->prototype->args)
		{
			func->body->in_scope_vars.push_back({arg, ast::ReferenceType::Variable});
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
	bool TypeChecker::check_expression<ast::LiteralExpr>(ast::LiteralExpr* expr) const
	{
		// check the literal type
		if (!expr->check_types())
		{
			return log_error(expr, "Literal has invalid type");
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::BodyExpr>(ast::BodyExpr* body) const
	{
		// mangle all of the function prototypes
		std::map<int, ast::FunctionPrototype*> function_prototypes;

		for (auto& proto : body->original_function_prototypes)
		{
			// if function is main with no args then don't mangle, but only on top level bodies
			if (body->get_body() == nullptr)
			{
				std::string main = "main";
				int main_id = stringManager::get_id(main);

				if (proto->name_id == main_id && proto->args.size() == 0)
				{
					function_prototypes.insert({proto->name_id, proto});
					continue;
				}
			}
			int id = mangler::mangle(moduleManager::get_module(this->current_file_id), proto);
			function_prototypes.insert({id, proto});
			proto->name_id = id;
		}

		body->function_prototypes = function_prototypes;

		// check all function prototypes to make sure there are no redefinitions
		// TODO: allow same name function in embedded scopes
		for (auto& p : body->function_prototypes)
		{
			if (scope::is_variable_defined(body, p.first, ast::ReferenceType::Function))
			{
				return log_error(body, "Function: " + stringManager::get_string(p.first) + ", is already defined");
			}

			body->in_scope_vars.push_back({p.first, ast::ReferenceType::Function});
			if (p.second->is_extern)
			{
				// add unmangled function name to extern list
				body->extern_functions.push_back(p.second->unmangled_name_id);
			}
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
	bool TypeChecker::check_expression<ast::VariableDeclarationExpr>(ast::VariableDeclarationExpr* expr) const
	{
		// check to see if we are redefining the variable, but only in the current scope
		for (auto& p : expr->get_body()->in_scope_vars)
		{
			if (p.second == ast::ReferenceType::Variable)
			{
				if (p.first == expr->name_id)
				{
					return log_error(
						expr,
						"Variable: " + stringManager::get_string(expr->name_id) + ", has already been defined");
				}
			}
		}

		expr->get_body()->in_scope_vars.push_back({expr->name_id, ast::ReferenceType::Variable});

		// check value expression
		if (expr->expr != nullptr && !check_expression_dispatch(expr->expr.get()))
		{
			return false;
		}

		// check the expression result type
		if (!expr->check_types())
		{
			std::string type1 = expr->curr_type.to_string();
			std::string type2 = expr->expr->get_result_type().to_string();
			return log_error(
				expr,
				"Variable declaration expression for: " + stringManager::get_string(expr->name_id) +
					", expected type: " + type1 + " but got type: " + type2 + " instead");
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::VariableReferenceExpr>(ast::VariableReferenceExpr* expr) const
	{
		// check variable has already been defined
		if (!scope::is_variable_defined(expr, expr->name_id, ast::ReferenceType::Variable))
		{
			return log_error(
				expr,
				"Variable reference for: " + stringManager::get_string(expr->name_id) +
					", is not in scope (not defined)");
		}

		// check variable is in scope
		if (scope::get_scope(expr) == nullptr)
		{
			return log_error(
				expr,
				"Variable reference for: " + stringManager::get_string(expr->name_id) + ", is not in scope");
		}

		// chekc variable type
		if (!expr->check_types())
		{
			return log_error(
				expr,
				"Variable reference for: " + stringManager::get_string(expr->name_id) + ", has invalid type");
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::BinaryExpr>(ast::BinaryExpr* expr) const
	{
		if (expr->binop == operators::BinaryOp::ModuleScope)
		{
			// lhs will contain any other scope binops and variable references
			// but rhs will only contain call expressions

			// recursively check lhs type
			ast::BinaryExpr* scope_expr = expr;
			while (true)
			{
				ast::AstExprType lhs_type = scope_expr->lhs->get_type();

				if (lhs_type != ast::AstExprType::VariableReferenceExpr && lhs_type != ast::AstExprType::BinaryExpr)
				{
					return log_error(
						scope_expr,
						"Binary Operator: " + operators::to_string(scope_expr->binop) +
							", lhs must be an identifier or another scoper operator.");
				}
				else
				{
					// check rhs type is variable refernce, if were not at the top scope binop
					if (scope_expr != expr && scope_expr->rhs->get_type() != ast::AstExprType::VariableReferenceExpr)
					{
						return log_error(
							scope_expr,
							"Binary Operator: " + operators::to_string(scope_expr->binop) +
								", rhs must be an identifier.");
					}
					if (lhs_type == ast::AstExprType::BinaryExpr)
					{
						scope_expr = dynamic_cast<ast::BinaryExpr*>(scope_expr->lhs.get());
						continue;
					}
					else
					{
						// do nothing for variable reference
						break;
					}
				}
			}

			// check rhs type
			ast::AstExprType rhs_type = expr->rhs->get_type();
			if (rhs_type != ast::AstExprType::CallExpr)
			{
				return log_error(
					expr,
					"Binary Operator: " + operators::to_string(expr->binop) + ", rhs must be a call expression.");
			}

			// check module exists
			int module_id = -1;
			if (expr->lhs->get_type() == ast::AstExprType::BinaryExpr)
			{
				module_id = mangler::add_mangled_name(
					-1,
					mangler::mangle_using(dynamic_cast<ast::BinaryExpr*>(expr->lhs.get())));
			}
			else
			{
				module_id =
					mangler::add_module(-1, dynamic_cast<ast::VariableReferenceExpr*>(expr->lhs.get())->name_id);
			}

			if (!moduleManager::is_module_available(this->current_file_id, module_id))
			{
				return log_error(
					expr,
					"Binary Operator: " + operators::to_string(expr->binop) +
						", lhs module: " + stringManager::get_string(module_id) + ", does not exist.");
			}

			// get rhs name id
			int rhs_mangled_id = -1;
			if (expr->rhs->get_type() == ast::AstExprType::CallExpr)
			{
				ast::CallExpr* call_expr = dynamic_cast<ast::CallExpr*>(expr->rhs.get());
				rhs_mangled_id = mangler::mangle(module_id, call_expr);
			}

			// set rhs as mangled
			expr->rhs->set_mangled(true);

			// set rhs mangled name id
			if (expr->rhs->get_type() == ast::AstExprType::CallExpr)
			{
				dynamic_cast<ast::CallExpr*>(expr->rhs.get())->callee_id = rhs_mangled_id;
			}

			// check rhs
			if (!check_expression_dispatch(expr->rhs.get()))
			{
				return false;
			}
		}
		else
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

			if (operators::is_assignemnt(expr->binop) &&
				expr->lhs->get_type() != ast::AstExprType::VariableReferenceExpr)
			{
				return log_error(
					expr,
					"Binary operator " + operators::to_string(expr->binop) + " lhs must be an identifier.");
			}

			if (operators::is_compound_assignemnt(expr->binop))
			{
				expand_compound_assignment(expr);
			}

			// check both sides of the binary operator have the same type, type is given by lhs
			if (!expr->check_types())
			{
				std::string type1 = expr->lhs->get_result_type().to_string();
				std::string type2 = expr->lhs->get_result_type().to_string();
				return log_error(
					expr,
					"Binary operator " + operators::to_string(expr->binop) + " has incompatible types: " + type1 +
						" and " + type2 + " given");
			}

			// check type supports the specified operation
			if (!operators::is_type_supported(expr->binop, expr->get_result_type()))
			{
				std::string type = expr->rhs->get_result_type().to_string();
				return log_error(
					expr,
					"Binary operator " + operators::to_string(expr->binop) +
						" does not support the given type: " + type);
			}
		}
		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::CallExpr>(ast::CallExpr* expr) const
	{
		for (auto& e : expr->args)
		{
			if (!check_expression_dispatch(e.get()))
			{
				return false;
			}
		}

		int id = -1;
		int no_module_id = -1;

		bool function_is_in_same_file = true;

		// expr will be mangled if it is the rhs of a module scope operator
		if (expr->is_mangled())
		{
			id = expr->callee_id;
			no_module_id = id;
		}
		else
		{
			id = mangler::mangle(moduleManager::get_module(this->current_file_id), expr);
			no_module_id = mangler::mangle(expr);
		}

		// check function has been defined in current scope
		if (!scope::is_variable_defined(expr, id, ast::ReferenceType::Function))
		{
			// check to see if function exists in the modules
			int full_function_id =
				moduleManager::find_function(this->current_file_id, no_module_id, expr->is_mangled());
			if (full_function_id == -1)
			{
				return log_error(
					expr,
					"Function call for: " + stringManager::get_string(expr->callee_id) +
						", is not in scope (not defined)");
			}
			else
			{
				id = full_function_id;
			}

			function_is_in_same_file = false;
		}

		// see if it is a extern function call
		if (scope::find_extern_function(expr, expr->unmangled_callee_id))
		{
			expr->is_extern = true;
		}
		else
		{
			if (!expr->is_mangled())
			{
				auto modules_function_is_in =
					moduleManager::get_matching_function_locations(this->current_file_id, no_module_id);

				if (modules_function_is_in.size() > 1 ||
					(function_is_in_same_file && modules_function_is_in.size() > 0))
				{
					std::string error_message =
						"Function call for: " + stringManager::get_string(expr->unmangled_callee_id) +
						", is ambiguous (defined in multiple places)";

					if (function_is_in_same_file)
					{
						error_message += '\n';
						error_message += '\t';
						error_message +=
							"Function defined in file: " + stringManager::get_string(this->current_file_id);
					}

					for (auto& module_id : modules_function_is_in)
					{
						error_message += '\n';
						error_message += '\t';
						error_message += "Function defined in module: " + mangler::pretty_modules(module_id);
					}

					return log_error(expr, error_message);
				}
			}
		}

		// mangle the call id
		expr->callee_id = id;

		// check function is in scope
		if (scope::get_scope(expr) == nullptr)
		{
			// return log_error(expr, "Function call for: " + stringManager::get_string(expr->callee_id) + ", is
			// not in scope");
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
	bool TypeChecker::check_expression<ast::IfExpr>(ast::IfExpr* expr) const
	{
		// check the condition
		if (!check_expression_dispatch(expr->condition.get()))
		{
			return false;
		}

		// check the condition has type bool
		if (expr->condition->get_result_type().get_type_enum() != types::TypeEnum::Bool)
		{
			return log_error(expr, "If condition must have type bool");
		}

		// check if body
		if (!check_expression_dispatch(expr->if_body.get()))
		{
			return false;
		}

		// check else body
		if (expr->else_body != nullptr && !check_expression_dispatch(expr->else_body.get()))
		{
			return false;
		}

		// check both bodies have the same result type, type iof given by the if body
		if (!expr->check_types())
		{
			std::string type1 = expr->if_body->get_result_type().to_string();
			std::string type2 = expr->else_body->get_result_type().to_string();
			return log_error(expr, "If Statement has incompatible result types: " + type1 + " and " + type2 + " given");
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::ForExpr>(ast::ForExpr* expr) const
	{
		// add variable to for body scope
		expr->for_body->in_scope_vars.push_back({expr->name_id, ast::ReferenceType::Variable});

		// check the start expression
		if (!check_expression_dispatch(expr->start_expr.get()))
		{
			return false;
		}

		// check the variable has correct type
		if (expr->var_type != expr->start_expr->get_result_type())
		{
			std::string type1 = expr->var_type.to_string();
			std::string type2 = expr->start_expr->get_result_type().to_string();
			return log_error(
				expr,
				"for start expression has invalid type, expected type: " + type1 + ", but got type: " + type2 +
					" instead");
		}

		// check the end condition
		if (!check_expression_dispatch(expr->end_expr.get()))
		{
			return false;
		}

		// check the end condition has type bool
		if (expr->end_expr->get_result_type().get_type_enum() != types::TypeEnum::Bool)
		{
			return log_error(expr, "For end condition must have type bool");
		}

		// check the step expression
		if (expr->step_expr != nullptr && !check_expression_dispatch(expr->step_expr.get()))
		{
			return false;
		}

		// check for body
		if (!check_expression_dispatch(expr->for_body.get()))
		{
			return false;
		}

		// check for
		if (!expr->check_types())
		{
			return log_error(expr, "For end condition must have type bool (for check types)");
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::WhileExpr>(ast::WhileExpr* expr) const
	{
		// check the end condition
		if (!check_expression_dispatch(expr->end_expr.get()))
		{
			return false;
		}

		// check the end condition has type bool
		if (expr->end_expr->get_result_type().get_type_enum() != types::TypeEnum::Bool)
		{
			return log_error(expr, "While end condition must have type bool");
		}

		// check for body
		if (!check_expression_dispatch(expr->while_body.get()))
		{
			return false;
		}

		// check for
		if (!expr->check_types())
		{
			return log_error(expr, "While end condition must have type bool (for check types)");
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::CommentExpr>(ast::CommentExpr* expr) const
	{
		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::ReturnExpr>(ast::ReturnExpr* expr) const
	{
		// check return expression
		if (expr->ret_expr != nullptr && !check_expression_dispatch(expr->ret_expr.get()))
		{
			return false;
		}

		// check return
		if (!expr->check_types())
		{
			ast::BodyExpr* body = expr->get_body();

			while (body->body_type != ast::BodyType::Function)
			{
				body = body->get_body();
			}

			std::string type1 = body->parent_function->return_type.to_string();

			std::string type2 = expr->ret_expr->get_result_type().to_string();
			return log_error(
				expr,
				"Return statement has incompatible type with function return type, expected: " + type1 +
					" , but got: " + type2 + " instead");
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::ContinueExpr>(ast::ContinueExpr* expr) const
	{
		// check continue
		if (!expr->check_types())
		{
			return log_error(expr, "Continue statement is only allowed in loops");
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::BreakExpr>(ast::BreakExpr* expr) const
	{
		// check break
		if (!expr->check_types())
		{
			return log_error(expr, "Break statement is not allowed in current expression");
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::UnaryExpr>(ast::UnaryExpr* expr) const
	{
		// check expression
		if (!check_expression_dispatch(expr->expr.get()))
		{
			return false;
		}

		// check both sides of the binary operator have the same type, type is given by lhs
		if (!expr->check_types())
		{
			std::string type = expr->expr->get_result_type().to_string();
			return log_error(
				expr,
				"Unary operator " + operators::to_string(expr->unop) + " has incompatible type: " + type + " given");
		}

		// check type supports the specified operation
		if (!operators::is_type_supported(expr->unop, expr->get_result_type()))
		{
			std::string type = expr->expr->get_result_type().to_string();
			return log_error(
				expr,
				"Unary operator " + operators::to_string(expr->unop) + " does not support the given type: " + type);
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::CastExpr>(ast::CastExpr* expr) const
	{
		// check expression
		if (!check_expression_dispatch(expr->expr.get()))
		{
			return false;
		}

		types::Type from_type = expr->expr->get_result_type();

		// check target type is a valid type
		types::Type type = types::is_valid_type(stringManager::get_string(expr->target_type_id));
		if (type.get_type_enum() == types::TypeEnum::None)
		{
			return log_error(expr, "Cast target type is invalid: " + stringManager::get_string(expr->target_type_id));
		}

		expr->target_type = type;

		// check that type can be converted into the target type
		if (!types::is_cast_valid(from_type, type))
		{
			std::string type1 = from_type.to_string();
			std::string type2 = type.to_string();
			return log_error(expr, "Cannot cast from type: " + type1 + ", to type: " + type2);
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::SwitchExpr>(ast::SwitchExpr* expr) const
	{
		// check expression
		if (!check_expression_dispatch(expr->value_expr.get()))
		{
			return false;
		}

		// check value
		if (!expr->check_types())
		{
			std::string type = expr->value_expr->get_result_type().to_string();
			return log_error(expr, "Switch value must have type integer, but got type: " + type);
		}

		types::Type value_type = expr->value_expr->get_result_type();

		// make sure all types are identical
		for (auto& case_expr : expr->cases)
		{
			if (case_expr->default_case)
			{
				continue;
			}

			if (case_expr->case_expr->get_result_type() != value_type)
			{
				std::string type1 = value_type.to_string();
				std::string type2 = case_expr->case_expr->get_result_type().to_string();
				return log_error(
					case_expr.get(),
					"Case has invalid type, expected: " + type1 + " , but got: " + type2 + " instead");
			}
		}

		// make sure there are no duplicate values
		std::vector<types::IntType> case_values;

		for (auto& case_expr : expr->cases)
		{
			if (case_expr->default_case)
			{
				continue;
			}

			// TODO: make checks cleaner/simpler
			ast::LiteralExpr* literal_expr = dynamic_cast<ast::LiteralExpr*>(case_expr->case_expr.get());
			if (literal_expr == nullptr)
			{
				return log_error(case_expr.get(), "Case value not literal expression");
			}

			types::IntType* type = dynamic_cast<types::IntType*>(literal_expr->value_type.get());
			if (type == nullptr)
			{
				return log_error(case_expr.get(), "Case value must be an integer");
			}

			auto it = std::find(case_values.begin(), case_values.end(), *type);
			if (it != case_values.end())
			{
				std::string value = type->to_string();
				return log_error(case_expr.get(), "Switch already has a case value of: " + value);
			}

			case_values.push_back(*type);
		}

		// check case
		for (auto& expr : expr->cases)
		{
			if (!this->check_expression_dispatch(expr.get()))
			{
				return false;
			}
		}

		return true;
	}

	template<>
	bool TypeChecker::check_expression<ast::CaseExpr>(ast::CaseExpr* expr) const
	{
		// check case body
		if (!this->check_expression_dispatch(expr->case_body.get()))
		{
			return false;
		}

		return true;
	}

	bool TypeChecker::check_expression_dispatch(ast::BaseExpr* expr) const
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
			case ast::AstExprType::WhileExpr:
			{
				return check_expression(dynamic_cast<ast::WhileExpr*>(expr));
			}
			case ast::AstExprType::CommentExpr:
			{
				return check_expression(dynamic_cast<ast::CommentExpr*>(expr));
			}
			case ast::AstExprType::ReturnExpr:
			{
				return check_expression(dynamic_cast<ast::ReturnExpr*>(expr));
			}
			case ast::AstExprType::ContinueExpr:
			{
				return check_expression(dynamic_cast<ast::ContinueExpr*>(expr));
			}
			case ast::AstExprType::BreakExpr:
			{
				return check_expression(dynamic_cast<ast::BreakExpr*>(expr));
			}
			case ast::AstExprType::UnaryExpr:
			{
				return check_expression(dynamic_cast<ast::UnaryExpr*>(expr));
			}
			case ast::AstExprType::CastExpr:
			{
				return check_expression(dynamic_cast<ast::CastExpr*>(expr));
			}
			case ast::AstExprType::SwitchExpr:
			{
				return check_expression(dynamic_cast<ast::SwitchExpr*>(expr));
			}
			case ast::AstExprType::CaseExpr:
			{
				return check_expression(dynamic_cast<ast::CaseExpr*>(expr));
			}
		}
		assert(false && "Missing Type Specialisation");
		return false;
	}

	void TypeChecker::expand_compound_assignment(ast::BinaryExpr* expr) const
	{
		// turn a op= b into a = a op b

		operators::BinaryOp op = operators::extract_compound_assignment_operator(expr->binop);

		ast::VariableReferenceExpr* lhs = dynamic_cast<ast::VariableReferenceExpr*>(expr->lhs.get());
		if (lhs == nullptr)
		{
			// NOTE: this should never be reached
			std::cout << "expand compound lhs is not var ref";
			std::abort();
		}

		// copy the variable reference expr
		ptr_type<ast::VariableReferenceExpr> var_ref_expr =
			make_ptr<ast::VariableReferenceExpr>(expr->get_body(), lhs->name_id);

		ptr_type<ast::BinaryExpr> op_expr =
			make_ptr<ast::BinaryExpr>(expr->get_body(), op, std::move(var_ref_expr), std::move(expr->rhs));

		expr->binop = operators::BinaryOp::Assignment;
		expr->rhs = std::move(op_expr);
	}
}
