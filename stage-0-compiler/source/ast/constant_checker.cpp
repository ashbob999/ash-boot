#include "constant_checker.h"

namespace constant_checker
{
	using ast::ConstantStatus;

	template<>
	void check_expression<ast::LiteralExpr>(ast::LiteralExpr* expr)
	{
		if (expr->constant_status != ConstantStatus::Unknown)
		{
			return;
		}

		// always constant
		expr->constant_status = ConstantStatus::Constant;
	}

	template<>
	void check_expression<ast::BodyExpr>(ast::BodyExpr* expr)
	{
		if (expr->constant_status != ConstantStatus::Unknown)
		{
			return;
		}

		// check each function
		for (auto& f : expr->functions)
		{
			check_expression_dispatch(f->body.get());
			expr->constant_status |= f->body->constant_status;
		}

		// check each expression
		for (auto& e : expr->expressions)
		{
			check_expression_dispatch(e.get());
			expr->constant_status |= e->constant_status;
		}

		expr->constant_status |= ConstantStatus::CanBeConstant;
	}

	template<>
	void check_expression<ast::VariableDeclarationExpr>(ast::VariableDeclarationExpr* expr)
	{
		if (expr->constant_status != ConstantStatus::Unknown)
		{
			return;
		}

		check_expression_dispatch(expr->expr.get());
		expr->constant_status = expr->expr->constant_status;
	}

	template<>
	void check_expression<ast::VariableReferenceExpr>(ast::VariableReferenceExpr* expr)
	{
		if (expr->constant_status != ConstantStatus::Unknown)
		{
			return;
		}

		// TODO: do better checks for constant
		expr->constant_status = ConstantStatus::Variable;
	}

	template<>
	void check_expression<ast::BinaryExpr>(ast::BinaryExpr* expr)
	{
		if (expr->constant_status != ConstantStatus::Unknown)
		{
			return;
		}

		check_expression_dispatch(expr->lhs.get());
		check_expression_dispatch(expr->rhs.get());

		if (expr->binop == operators::BinaryOp::ModuleScope)
		{
			expr->constant_status = expr->rhs->constant_status;
		}
		else
		{
			expr->constant_status = expr->lhs->constant_status | expr->rhs->constant_status;
		}
	}

	template<>
	void check_expression<ast::CallExpr>(ast::CallExpr* expr)
	{
		if (expr->constant_status != ConstantStatus::Unknown)
		{
			return;
		}

		for (auto& e : expr->args)
		{
			check_expression_dispatch(e.get());
			expr->constant_status |= e->constant_status;
		}

		expr->constant_status |= ConstantStatus::CanBeConstant;

		// TEMP: no way to get function
		expr->constant_status = ConstantStatus::Variable;
	}

	template<>
	void check_expression<ast::IfExpr>(ast::IfExpr* expr)
	{
		if (expr->constant_status != ConstantStatus::Unknown)
		{
			return;
		}

		check_expression_dispatch(expr->condition.get());
		expr->constant_status |= expr->condition->constant_status;

		check_expression_dispatch(expr->if_body.get());
		expr->constant_status |= expr->if_body->constant_status;

		if (expr->else_body != nullptr)
		{
			check_expression_dispatch(expr->else_body.get());
			expr->constant_status |= expr->else_body->constant_status;
		}

		expr->constant_status |= ConstantStatus::CanBeConstant;
	}

	template<>
	void check_expression<ast::ForExpr>(ast::ForExpr* expr)
	{
		if (expr->constant_status != ConstantStatus::Unknown)
		{
			return;
		}

		check_expression_dispatch(expr->start_expr.get());
		expr->constant_status |= expr->start_expr->constant_status;

		check_expression_dispatch(expr->end_expr.get());
		expr->constant_status |= expr->end_expr->constant_status;

		if (expr->step_expr != nullptr)
		{
			check_expression_dispatch(expr->step_expr.get());
			expr->constant_status |= expr->step_expr->constant_status;
		}

		check_expression_dispatch(expr->for_body.get());
		expr->constant_status |= expr->for_body->constant_status;

		expr->constant_status |= ConstantStatus::CanBeConstant;
	}

	template<>
	void check_expression<ast::WhileExpr>(ast::WhileExpr* expr)
	{
		if (expr->constant_status != ConstantStatus::Unknown)
		{
			return;
		}

		check_expression_dispatch(expr->end_expr.get());
		expr->constant_status |= expr->end_expr->constant_status;

		check_expression_dispatch(expr->while_body.get());
		expr->constant_status |= expr->while_body->constant_status;

		expr->constant_status |= ConstantStatus::CanBeConstant;
	}

	template<>
	void check_expression<ast::CommentExpr>(ast::CommentExpr* expr)
	{
		if (expr->constant_status != ConstantStatus::Unknown)
		{
			return;
		}

		expr->constant_status = ConstantStatus::Constant;
	}

	template<>
	void check_expression<ast::ReturnExpr>(ast::ReturnExpr* expr)
	{
		if (expr->constant_status != ConstantStatus::Unknown)
		{
			return;
		}

		if (expr->ret_expr != nullptr)
		{
			check_expression_dispatch(expr->ret_expr.get());
			expr->constant_status = expr->ret_expr->constant_status;
		}
		else
		{
			expr->constant_status = ConstantStatus::Constant;
		}
	}

	template<>
	void check_expression<ast::ContinueExpr>(ast::ContinueExpr* expr)
	{
		if (expr->constant_status != ConstantStatus::Unknown)
		{
			return;
		}

		expr->constant_status = ConstantStatus::Constant;
	}

	template<>
	void check_expression<ast::BreakExpr>(ast::BreakExpr* expr)
	{
		if (expr->constant_status != ConstantStatus::Unknown)
		{
			return;
		}

		expr->constant_status = ConstantStatus::Constant;
	}

	template<>
	void check_expression<ast::UnaryExpr>(ast::UnaryExpr* expr)
	{
		if (expr->constant_status != ConstantStatus::Unknown)
		{
			return;
		}

		check_expression_dispatch(expr->expr.get());
		expr->constant_status = expr->expr->constant_status;
	}

	template<>
	void check_expression<ast::CastExpr>(ast::CastExpr* expr)
	{
		if (expr->constant_status != ConstantStatus::Unknown)
		{
			return;
		}

		check_expression_dispatch(expr->expr.get());
		expr->constant_status = expr->expr->constant_status;
	}

	template<>
	void check_expression<ast::SwitchExpr>(ast::SwitchExpr* expr)
	{
		if (expr->constant_status != ConstantStatus::Unknown)
		{
			return;
		}

		check_expression_dispatch(expr->value_expr.get());
		expr->constant_status |= expr->value_expr->constant_status;

		for (auto& e : expr->cases)
		{
			check_expression_dispatch(e.get());
			expr->constant_status |= e->constant_status;
		}

		expr->constant_status |= ConstantStatus::CanBeConstant;
	}

	template<>
	void check_expression<ast::CaseExpr>(ast::CaseExpr* expr)
	{
		if (expr->constant_status != ConstantStatus::Unknown)
		{
			return;
		}

		if (expr->case_expr != nullptr)
		{
			check_expression_dispatch(expr->case_expr.get());
			expr->constant_status |= expr->case_expr->constant_status;
		}

		check_expression_dispatch(expr->case_body.get());
		expr->constant_status |= expr->case_body->constant_status;
	}

	void check_expression_dispatch(ast::BaseExpr* expr)
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
	}
}
