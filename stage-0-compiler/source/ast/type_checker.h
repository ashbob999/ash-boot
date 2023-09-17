#pragma once

#include "ast.h"
#include "module.h"

namespace type_checker
{
	class TypeChecker
	{
	public:
		TypeChecker();
		bool check_types(ast::BaseExpr* body) const;
		bool check_prototypes(ast::BodyExpr* body) const;
		void set_file_id(int file_id);

	private:
		bool check_function(ast::FunctionDefinition* func) const;
		bool check_expression_dispatch(ast::BaseExpr* expr) const;
		template<class T, typename = std::enable_if_t<std::is_base_of_v<ast::BaseExpr, T>>>
		bool check_expression(T* expr) const;
		void expand_compound_assignment(ast::BinaryExpr* expr) const;
		bool log_error(const ast::BaseExpr* expr, const std::string& str) const;

	private:
		int current_file_id = -1;
	};
}
