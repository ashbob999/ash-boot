#pragma once

#include "ast.h"

namespace type_checker
{
	class TypeChecker
	{
	public:
		TypeChecker();
		bool check_types(ast::BaseExpr* body);

	private:
		bool check_function(ast::FunctionDefinition* func);
		bool check_expression_dispatch(ast::BaseExpr* expr);
		template<class T, typename = std::enable_if_t<std::is_base_of_v<ast::BaseExpr, T>>>
		bool check_expression(T* expr);
		bool log_error(std::string str);
	};
}
