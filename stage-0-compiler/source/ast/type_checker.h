#pragma once

#include "ast.h"
#include "module.h"

namespace type_checker
{
	class TypeChecker
	{
	public:
		TypeChecker();
		bool check_types(ast::BaseExpr* body);
		void set_module(module::Module mod);

	private:
		bool check_function(ast::FunctionDefinition* func);
		bool check_expression_dispatch(ast::BaseExpr* expr);
		template<class T, typename = std::enable_if_t<std::is_base_of_v<ast::BaseExpr, T>>>
		bool check_expression(T* expr);
		bool log_error(ast::BaseExpr* expr, std::string str);

	private:
		module::Module current_module;
	};
}
