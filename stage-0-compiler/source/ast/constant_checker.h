#pragma once

#include "ast.h"

namespace constant_checker
{
	void check_expression_dispatch(ast::BaseExpr* expr);

	// TODO: make private?
	template<class T, typename = std::enable_if_t<std::is_base_of_v<ast::BaseExpr, T>>>
	void check_expression(T* expr);
}
