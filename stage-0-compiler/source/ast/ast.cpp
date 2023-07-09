#include <sstream>

#include "ast.h"
#include "scope_checker.h"
#include "module.h"

namespace ast
{
	BaseExpr::BaseExpr(AstExprType ast_type, BodyExpr* body)
		: ast_type(ast_type), body(body)
	{}

	AstExprType BaseExpr::get_type()
	{
		return this->ast_type;
	}

	BodyExpr* BaseExpr::get_body()
	{
		return this->body;
	}

	void BaseExpr::set_body(BodyExpr* new_body)
	{
		this->body = new_body;

		switch (this->ast_type)
		{
			case AstExprType::LiteralExpr:
			{
				break;
			}
			case AstExprType::BodyExpr:
			{
				BodyExpr* expr = dynamic_cast<BodyExpr*>(this);

				for (auto& f : expr->functions)
				{
					f->body->set_body(new_body);
				}

				for (auto& e : expr->expressions)
				{
					e->set_body(new_body);
				}

				break;
			}
			case AstExprType::VariableDeclarationExpr:
			{
				VariableDeclarationExpr* expr = dynamic_cast<VariableDeclarationExpr*>(this);

				if (expr->expr != nullptr)
				{
					expr->expr->set_body(new_body);
				}

				break;
			}
			case AstExprType::VariableReferenceExpr:
			{
				break;
			}
			case AstExprType::BinaryExpr:
			{
				BinaryExpr* expr = dynamic_cast<BinaryExpr*>(this);

				expr->lhs->set_body(new_body);
				expr->rhs->set_body(new_body);
				break;
			}
			case AstExprType::CallExpr:
			{
				CallExpr* expr = dynamic_cast<CallExpr*>(this);

				for (auto& e : expr->args)
				{
					e->set_body(new_body);
				}

				break;
			}
			case AstExprType::IfExpr:
			{
				IfExpr* expr = dynamic_cast<IfExpr*>(this);

				expr->condition->set_body(new_body);
				expr->if_body->set_body(new_body);
				expr->else_body->set_body(new_body);

				break;
			}
			case AstExprType::ForExpr:
			{
				ForExpr* expr = dynamic_cast<ForExpr*>(this);

				expr->start_expr->set_body(new_body);
				expr->end_expr->set_body(new_body);
				if (expr->step_expr != nullptr)
				{
					expr->step_expr->set_body(new_body);
				}
				expr->for_body->set_body(new_body);

				break;
			}
			case AstExprType::CommentExpr:
			{
				break;
			}
			case AstExprType::ReturnExpr:
			{
				break;
			}
			case AstExprType::ContinueExpr:
			{
				break;
			}
			case AstExprType::BreakExpr:
			{
				break;
			}
			case AstExprType::UnaryExpr:
			{
				UnaryExpr* expr = dynamic_cast<UnaryExpr*>(this);

				expr->expr->set_body(new_body);
				break;
			}
			case AstExprType::SwitchExpr:
			{
				SwitchExpr* expr = dynamic_cast<SwitchExpr*>(this);

				for (auto& c : expr->cases)
				{
					c->set_body(new_body);
				}

				break;
			}
			case AstExprType::CaseExpr:
			{
				CaseExpr* expr = dynamic_cast<CaseExpr*>(this);

				expr->case_expr->set_body(new_body);
				expr->case_body->set_body(new_body);

				break;
			}
			default:
			{
				assert(false && "missing type specialisation");
			}
		}
	}

	void BaseExpr::set_line_info(ExpressionLineInfo line_info)
	{
		this->line_info = line_info;
	}

	ExpressionLineInfo& BaseExpr::get_line_info()
	{
		return this->line_info;
	}

	void BaseExpr::set_mangled(bool mangled)
	{
		this->is_name_mangled = mangled;
	}

	bool BaseExpr::is_mangled()
	{
		return this->is_name_mangled;
	}

	void BaseExpr::set_parent_data(BaseExpr* parent, int location)
	{
		this->parent.parent = parent;
		this->parent.location = location;
	}

	parent_data BaseExpr::get_parent_data()
	{
		return this->parent;
	}

	bool BaseExpr::is_constant() const
	{
		return this->constant_status == ConstantStatus::Constant;
	}

	LiteralExpr::LiteralExpr(BodyExpr* body, types::Type curr_type, std::string& str)
		: BaseExpr(AstExprType::LiteralExpr, body), curr_type(curr_type)
	{
		value_type = types::BaseType::create_type(curr_type, str);
	}

	LiteralExpr::~LiteralExpr()
	{}

	std::string LiteralExpr::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Literal Value : {" << '\n';
		str << tabs << '\t' << "Type: " << types::to_string(this->curr_type) << '\n';
		str << tabs << '\t' << "Value: " << this->value_type->to_string() << '\n';
		str << tabs << "}," << '\n';

		return str.str();
	}

	types::Type LiteralExpr::get_result_type()
	{
		if (result_type.type_enum == types::TypeEnum::None)
		{
			result_type = this->curr_type;
		}
		return result_type;
	}

	bool LiteralExpr::check_types()
	{
		if (result_type.type_enum == types::TypeEnum::None)
		{
			result_type = this->curr_type;
		}

		return true;
	}

	VariableDeclarationExpr::VariableDeclarationExpr(BodyExpr* body, types::Type curr_type, int name_id, ptr_type<BaseExpr> expr)
		: BaseExpr(AstExprType::VariableDeclarationExpr, body), curr_type(curr_type), name_id(name_id), expr(std::move(expr))
	{
		if (this->expr != nullptr)
		{
			this->expr->set_parent_data(this, 0);
		}
	}

	VariableDeclarationExpr::~VariableDeclarationExpr()
	{}

	std::string VariableDeclarationExpr::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Variable Declaration: {" << '\n';
		str << tabs << '\t' << "Name: " << module::StringManager::get_string(this->name_id) << '\n';
		str << tabs << '\t' << "Type: " << types::to_string(this->curr_type) << '\n';
		str << tabs << '\t' << "Value: {" << '\n';
		if (this->expr == nullptr)
		{
			str << tabs << '\t' << '\t' << "<default>" << '\n';
		}
		else
		{
			str << this->expr->to_string(depth + 2);
		}
		str << tabs << '\t' << "}," << '\n';
		str << tabs << "}," << '\n';

		return str.str();
	}

	types::Type VariableDeclarationExpr::get_result_type()
	{
		if (result_type.type_enum == types::TypeEnum::None)
		{
			result_type = this->curr_type;
		}
		return result_type;
	}

	bool VariableDeclarationExpr::check_types()
	{
		if (this->expr == nullptr || this->get_result_type() == this->expr->get_result_type())
		{
			return true;
		}

		return false;
	}

	VariableReferenceExpr::VariableReferenceExpr(BodyExpr* body, int name_id)
		: BaseExpr(AstExprType::VariableReferenceExpr, body), name_id(name_id)
	{}

	std::string VariableReferenceExpr::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Variable Reference: {" << '\n';
		str << tabs << '\t' << "Name: " << module::StringManager::get_string(this->name_id) << '\n';
		str << tabs << "}," << '\n';

		return str.str();
	}

	types::Type VariableReferenceExpr::get_result_type()
	{
		if (result_type.type_enum == types::TypeEnum::None)
		{
			//result_type = this->get_body()->named_types[this->name];
			result_type = scope::get_scope(this)->named_types[this->name_id];
		}
		return result_type;
	}

	bool VariableReferenceExpr::check_types()
	{
		return true;
	}

	BinaryExpr::BinaryExpr(BodyExpr* body, operators::BinaryOp binop, ptr_type<BaseExpr> lhs, ptr_type<BaseExpr> rhs)
		: BaseExpr(AstExprType::BinaryExpr, body), binop(binop), lhs(std::move(lhs)), rhs(std::move(rhs))
	{
		this->lhs->set_parent_data(this, 0);
		this->rhs->set_parent_data(this, 1);
	}

	BinaryExpr::~BinaryExpr()
	{}

	std::string BinaryExpr::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Binary Operation: {" << '\n';
		str << tabs << '\t' << "LHS: {" << '\n';
		str << this->lhs->to_string(depth + 2);
		str << tabs << '\t' << "}," << '\n';
		str << tabs << '\t' << "Operator: " << operators::to_string(this->binop) << '\n';
		str << tabs << '\t' << "RHS: {" << '\n';
		str << this->rhs->to_string(depth + 2);
		str << tabs << '\t' << "}," << '\n';
		str << tabs << "}," << '\n';

		return str.str();
	}

	types::Type BinaryExpr::get_result_type()
	{
		if (result_type.type_enum == types::TypeEnum::None)
		{
			if (is_binary_comparision(this->binop))
			{
				result_type = types::TypeEnum::Bool;
			}
			else if (this->binop == operators::BinaryOp::ModuleScope)
			{
				result_type = this->rhs->get_result_type();
			}
			else
			{
				result_type = this->lhs->get_result_type();
			}
		}
		return result_type;
	}

	bool BinaryExpr::check_types()
	{
		if (this->binop == operators::BinaryOp::ModuleScope)
		{
			return true;
		}
		else if (this->lhs->get_result_type() == this->rhs->get_result_type())
		{
			return true;
		}

		return false;
	}

	BodyExpr::BodyExpr(BodyExpr* body, BodyType body_type)
		: BaseExpr(AstExprType::BodyExpr, body), body_type(body_type)
	{}

	BodyExpr::~BodyExpr()
	{
		for (auto& proto : original_function_prototypes)
		{
			if (proto != nullptr)
			{
				delete proto;
			}
		}
	}

	std::string BodyExpr::to_string(int depth)
	{
		// TODO: show nested functions
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Body: {" << '\n';

		for (auto& p : this->function_prototypes)
		{
			str << p.second->to_string(depth + 1);
		}

		for (auto& f : this->functions)
		{
			str << f->to_string(depth + 1);
		}

		for (auto& e : this->expressions)
		{
			str << e->to_string(depth + 1);
		}
		str << tabs << "}," << '\n';

		return str.str();
	}

	types::Type BodyExpr::get_result_type()
	{
		if (result_type.type_enum == types::TypeEnum::None)
		{
			if (this->expressions.size() > 0)
			{
				result_type = this->expressions.back()->get_result_type();
			}
		}
		return result_type;
	}

	bool BodyExpr::check_types()
	{
		return true;
	}

	void BodyExpr::add_base(ptr_type<BaseExpr> expr)
	{
		expr->set_parent_data(this, expressions.size());
		expressions.push_back(std::move(expr));
	}

	void BodyExpr::add_function(ptr_type<FunctionDefinition> func)
	{
		original_function_prototypes.push_back(func->prototype);
		functions.push_back(std::move(func));
	}

	void BodyExpr::add_prototype(FunctionPrototype* proto)
	{
		original_function_prototypes.push_back(proto);
	}

	llvm::Type* BodyExpr::get_llvm_type(llvm::LLVMContext& llvm_context, int str_id)
	{
		auto f = this->llvm_named_types.find(str_id);
		if (f != this->llvm_named_types.end())
		{
			return f->second;
		}
		llvm::Type* type = types::get_llvm_type(llvm_context, this->named_types[str_id]);
		this->llvm_named_types[str_id] = type;
		return type;
	}

	CallExpr::CallExpr(BodyExpr* body, int callee_id, std::vector<ptr_type<BaseExpr>>& args)
		: BaseExpr(AstExprType::CallExpr, body), callee_id(callee_id), args(args.size())
	{
		for (int i = 0; i < this->args.size(); i++)
		{
			this->args[i] = std::move(args[i]);
			this->args[i]->set_parent_data(this, i);
		}
	}

	CallExpr::~CallExpr()
	{}

	std::string CallExpr::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Function Call: {" << '\n';
		str << tabs << '\t' << "Name: " << module::StringManager::get_string(this->callee_id) << '\n';
		str << tabs << '\t' << "Args: {" << '\n';
		for (auto& a : this->args)
		{
			str << a->to_string(depth + 2);
		}
		str << tabs << '\t' << "}," << '\n';
		str << tabs << "}," << '\n';

		return str.str();
	}

	types::Type CallExpr::get_result_type()
	{
		if (result_type.type_enum == types::TypeEnum::None)
		{
			//result_type = scope::get_scope(ptr_type<CallExpr>(this))->function_prototypes[this->callee]->return_type;
			result_type = scope::get_scope(this)->function_prototypes[this->callee_id]->return_type;
			//result_type = this->get_body()->function_prototypes[this->callee]->return_type;
		}
		return result_type;
	}

	bool CallExpr::check_types()
	{
		/*
		FunctionPrototype* proto = this->get_body()->function_prototypes[this->callee];
		if (proto == nullptr)
		{
			proto = this->get_body()->get_body()->function_prototypes[this->callee];
		}

		BodyExpr* b1 = this->get_body();
		BodyExpr* b2 = this->get_body()->get_body();
		*/
		//ptr_type<FunctionPrototype> proto = scope::get_scope(ptr_type<CallExpr>(this))->function_prototypes[this->callee];
		FunctionPrototype* proto = scope::get_scope(this)->function_prototypes[this->callee_id];

		for (int i = 0; i < this->args.size(); i++)
		{
			if (this->args[i]->get_result_type() != proto->types[i])
			{
				return false;
			}
		}

		return true;
	}

	IfExpr::IfExpr(BodyExpr* body, ptr_type<BaseExpr> condition, ptr_type<BaseExpr> if_body, ptr_type<BaseExpr> else_body, bool should_return_value)
		: BaseExpr(AstExprType::IfExpr, body), condition(std::move(condition)), if_body(std::move(if_body)), else_body(std::move(else_body)), should_return_value(should_return_value)
	{
		this->condition->set_parent_data(this, 0);
		this->if_body->set_parent_data(this, 1);
		if (this->else_body != nullptr)
		{
			this->else_body->set_parent_data(this, 2);
		}
	}

	std::string IfExpr::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "If Statement: {" << '\n';

		str << tabs << '\t' << "Condition: {" << '\n';
		str << this->condition->to_string(depth + 2);
		str << tabs << '\t' << "}," << '\n';

		str << tabs << '\t' << "If Body: {" << '\n';
		str << this->if_body->to_string(depth + 2);
		str << tabs << '\t' << "}," << '\n';

		if (this->else_body != nullptr)
		{
			str << tabs << '\t' << "Else Body: {" << '\n';
			str << this->else_body->to_string(depth + 2);
			str << tabs << '\t' << "}" << '\n';
		}

		str << tabs << "}, " << '\n';

		return str.str();
	}

	types::Type IfExpr::get_result_type()
	{
		if (result_type.type_enum == types::TypeEnum::None)
		{
			if (this->should_return_value)
			{
				result_type = this->if_body->get_result_type();
			}
			else
			{
				result_type = types::TypeEnum::Void;
			}
		}
		return result_type;
	}

	bool IfExpr::check_types()
	{
		if (this->condition->get_result_type().type_enum == types::TypeEnum::Bool)
		{
			if (this->should_return_value)
			{
				if (this->if_body->get_result_type() == this->else_body->get_result_type())
				{
					return true;
				}
			}
			else
			{
				return true;
			}
		}
		return false;
	}

	ForExpr::ForExpr(BodyExpr* body, types::Type var_type, int name_id, ptr_type<BaseExpr> start_expr, ptr_type<BaseExpr> end_expr, ptr_type<BaseExpr> step_expr, ptr_type<BodyExpr> for_body)
		: BaseExpr(AstExprType::ForExpr, body), var_type(var_type), name_id(name_id), start_expr(std::move(start_expr)), end_expr(std::move(end_expr)), step_expr(std::move(step_expr)), for_body(std::move(for_body))
	{
		this->start_expr->set_parent_data(this, 0);
		this->end_expr->set_parent_data(this, 1);
		if (this->step_expr != nullptr)
		{
			this->step_expr->set_parent_data(this, 2);
		}
		this->for_body->set_parent_data(this, 3);
	}

	std::string ForExpr::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "For Loop: {" << '\n';

		str << tabs << '\t' << "Start Value: {" << '\n';
		str << tabs << '\t' << '\t' << "Type: " << types::to_string(this->var_type) << "," << '\n';
		str << tabs << '\t' << '\t' << "Name: " << module::StringManager::get_string(this->name_id) << "," << '\n';
		str << tabs << '\t' << '\t' << "Value: {" << '\n';
		str << this->start_expr->to_string(depth + 3);
		str << tabs << '\t' << '\t' << "}," << '\n';
		str << tabs << '\t' << "}," << '\n';

		str << tabs << '\t' << "End Condition: {" << '\n';
		str << this->end_expr->to_string(depth + 2);
		str << tabs << '\t' << "}," << '\n';

		str << tabs << '\t' << "Step Expression: {" << '\n';
		if (this->step_expr != nullptr)
		{
			str << this->step_expr->to_string(depth + 2);
		}
		else
		{
			str << tabs << '\t' << '\t' << "<empty>" << '\n';
		}
		str << tabs << '\t' << "}" << '\n';

		str << this->for_body->to_string(depth + 1);

		str << tabs << "}, " << '\n';

		return str.str();
	}

	types::Type ForExpr::get_result_type()
	{
		if (result_type.type_enum == types::TypeEnum::None)
		{
			result_type = types::TypeEnum::Void;
		}
		return result_type;
	}

	bool ForExpr::check_types()
	{
		if (end_expr->get_result_type().type_enum == types::TypeEnum::Bool)
		{
			return true;
		}
		return false;
	}

	WhileExpr::WhileExpr(BodyExpr* body, ptr_type<BaseExpr> end_expr, ptr_type<BaseExpr> while_body)
		: BaseExpr(AstExprType::WhileExpr, body), end_expr(std::move(end_expr)), while_body(std::move(while_body))
	{
		this->end_expr->set_parent_data(this, 0);
		this->while_body->set_parent_data(this, 1);
	}

	std::string WhileExpr::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "While Loop: {" << '\n';

		str << tabs << '\t' << "End Condition: {" << '\n';
		str << this->end_expr->to_string(depth + 2);
		str << tabs << '\t' << "}," << '\n';

		str << this->while_body->to_string(depth + 1);

		str << tabs << "}, " << '\n';

		return str.str();
	}

	types::Type WhileExpr::get_result_type()
	{
		if (result_type.type_enum == types::TypeEnum::None)
		{
			result_type = types::TypeEnum::Void;
		}
		return result_type;
	}

	bool WhileExpr::check_types()
	{
		if (end_expr->get_result_type().type_enum == types::TypeEnum::Bool)
		{
			return true;
		}
		return false;
	}

	CommentExpr::CommentExpr(BodyExpr* body)
		: BaseExpr(AstExprType::CommentExpr, body)
	{}

	std::string CommentExpr::to_string(int depth)
	{
		return std::string();
	}

	types::Type CommentExpr::get_result_type()
	{
		return types::TypeEnum::None;
	}

	bool CommentExpr::check_types()
	{
		return true;
	}

	ReturnExpr::ReturnExpr(BodyExpr* body, ptr_type<BaseExpr> ret_expr)
		: BaseExpr(AstExprType::ReturnExpr, body), ret_expr(std::move(ret_expr))
	{}

	std::string ReturnExpr::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Return Statement: {" << '\n';

		str << tabs << '\t' << "Value: {" << '\n';
		if (this->ret_expr != nullptr)
		{
			str << this->ret_expr->to_string(depth + 2);
		}
		else
		{
			str << tabs << '\t' << '\t' << "<void>" << '\n';
		}
		str << tabs << '\t' << "}," << '\n';

		str << tabs << "}, " << '\n';

		return str.str();
	}

	types::Type ReturnExpr::get_result_type()
	{
		if (result_type.type_enum == types::TypeEnum::None)
		{
			if (ret_expr == nullptr)
			{
				result_type = types::TypeEnum::Void;
			}
			else
			{
				result_type = ret_expr->get_result_type();
			}
		}
		return result_type;
	}

	bool ReturnExpr::check_types()
	{
		BodyExpr* body = this->get_body();

		while (body->body_type != ast::BodyType::Function)
		{
			body = body->get_body();
		}

		if (body->parent_function->return_type == this->get_result_type())
		{
			return true;
		}

		return false;
	}

	ContinueExpr::ContinueExpr(BodyExpr* body)
		: BaseExpr(AstExprType::ContinueExpr, body)
	{}

	std::string ContinueExpr::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Continue Statement: {}" << '\n';

		return str.str();
	}

	types::Type ContinueExpr::get_result_type()
	{
		if (result_type.type_enum == types::TypeEnum::None)
		{
			result_type = types::TypeEnum::Void;
		}
		return result_type;
	}

	bool ContinueExpr::check_types()
	{
		BodyExpr* body = this->get_body();

		while (body->body_type != BodyType::Function)
		{
			if (body->body_type == BodyType::Loop)
			{
				return true;
			}

			body = body->get_body();
		}

		return false;
	}

	BreakExpr::BreakExpr(BodyExpr* body)
		: BaseExpr(AstExprType::BreakExpr, body)
	{}

	std::string BreakExpr::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Break Statement: {}" << '\n';

		return str.str();
	}

	types::Type BreakExpr::get_result_type()
	{
		if (result_type.type_enum == types::TypeEnum::None)
		{
			result_type = types::TypeEnum::Void;
		}
		return result_type;
	}

	bool BreakExpr::check_types()
	{
		BodyExpr* body = this->get_body();

		while (body->body_type != BodyType::Function)
		{
			if (body->body_type == BodyType::Loop || body->body_type == BodyType::Case)
			{
				return true;
			}

			body = body->get_body();
		}

		return false;
	}

	UnaryExpr::UnaryExpr(BodyExpr* body, operators::UnaryOp unop, ptr_type<BaseExpr> expr)
		: BaseExpr(ast::AstExprType::UnaryExpr, body), unop(unop), expr(std::move(expr))
	{
		this->expr->set_parent_data(this, 0);
	}

	std::string UnaryExpr::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Unary Operation: {" << '\n';
		str << tabs << '\t' << "Operator: " << operators::to_string(this->unop) << '\n';
		str << tabs << '\t' << "Expression: {" << '\n';
		str << this->expr->to_string(depth + 2);
		str << tabs << '\t' << "}" << '\n';
		str << tabs << "}," << '\n';

		return str.str();
	}

	types::Type UnaryExpr::get_result_type()
	{
		if (result_type.type_enum == types::TypeEnum::None)
		{
			result_type = this->expr->get_result_type();
		}
		return result_type;
	}

	bool UnaryExpr::check_types()
	{
		return true;
	}

	CastExpr::CastExpr(BodyExpr* body, int target_type_id, ptr_type<BaseExpr> expr)
		: BaseExpr(AstExprType::CastExpr, body), target_type_id(target_type_id), expr(std::move(expr))
	{}

	std::string CastExpr::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Cast Operation: {" << '\n';
		str << tabs << '\t' << "Target Type: " << types::to_string(target_type) << '\n';
		str << tabs << '\t' << "Expression: {" << '\n';
		str << this->expr->to_string(depth + 2);
		str << tabs << '\t' << "}" << '\n';
		str << tabs << "}," << '\n';

		return str.str();
	}

	types::Type CastExpr::get_result_type()
	{
		if (result_type.type_enum == types::TypeEnum::None)
		{
			result_type = target_type;
		}
		return result_type;
	}

	bool CastExpr::check_types()
	{
		return true;
	}

	SwitchExpr::SwitchExpr(BodyExpr* body, ptr_type<BaseExpr> value_expr, std::vector<ptr_type<CaseExpr>>& cases)
		: BaseExpr(AstExprType::SwitchExpr, body), value_expr(std::move(value_expr))
	{
		for (auto& case_expr : cases)
		{
			case_expr->set_parent_data(this, this->cases.size());
			this->cases.push_back(std::move(case_expr));
		}
	}

	std::string SwitchExpr::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Switch Statement: {" << '\n';

		for (auto& case_expr : this->cases)
		{
			str << case_expr->to_string(depth + 1);
		}

		str << tabs << "}, " << '\n';

		return str.str();
	}

	types::Type SwitchExpr::get_result_type()
	{
		if (this->result_type.type_enum == types::TypeEnum::None)
		{
			this->result_type = types::TypeEnum::Void;
		}
		return this->result_type;
	}

	bool SwitchExpr::check_types()
	{
		if (this->value_expr->get_result_type().type_enum == types::TypeEnum::Int)
		{
			return true;
		}
		return false;
	}

	CaseExpr::CaseExpr(BodyExpr* body, ptr_type<BaseExpr> case_expr, ptr_type<BaseExpr> case_body, bool default_case)
		: BaseExpr(AstExprType::CaseExpr, body), case_expr(std::move(case_expr)), case_body(std::move(case_body)), default_case(default_case)
	{}

	std::string CaseExpr::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Case Statement: {" << '\n';

		if (this->default_case)
		{
			str << tabs << '\t' << "Value: " << "Default Case" << "," << '\n';
		}
		else
		{
			str << tabs << '\t' << "Value: {" << '\n';
			str << this->case_expr->to_string(depth + 2);
			str << tabs << '\t' << "}," << '\n';
		}

		str << tabs << '\t' << "Case Body: {" << '\n';
		str << this->case_body->to_string(depth + 2);
		str << tabs << '\t' << "}," << '\n';

		str << tabs << "}, " << '\n';

		return str.str();
	}

	types::Type CaseExpr::get_result_type()
	{
		if (this->result_type.type_enum == types::TypeEnum::None)
		{
			this->result_type = types::TypeEnum::Void;
		}
		return this->result_type;
	}

	bool CaseExpr::check_types()
	{
		// TODO: maybe check for constant expressions
		if (this->case_expr->get_result_type() == types::TypeEnum::Int)
		{
			return true;
		}
		return false;
	}

	FunctionPrototype::FunctionPrototype(std::string& name, types::Type return_type, std::vector<types::Type>& types, std::vector<int>& args)
		: name_id(module::StringManager::get_id(name)), return_type(return_type), types(types), args(args)
	{}

	std::string FunctionPrototype::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Function Prototype: {" << '\n';

		str << tabs << '\t' << "Name: " << module::StringManager::get_string(this->name_id) << '\n';
		str << tabs << '\t' << "Args: [" << '\n';

		for (int i = 0; i < this->args.size(); i++)
		{
			str << tabs << '\t' << '\t' << "{ " << module::StringManager::get_string(this->args[i]) << ": " << types::to_string(this->types[i]) << "}," << '\n';
		}

		str << tabs << '\t' << "]," << '\n';
		str << tabs << "}, " << '\n';

		return str.str();
	}

	FunctionDefinition::FunctionDefinition(FunctionPrototype* prototype, ptr_type<BodyExpr> body)
		: prototype(prototype), body(std::move(body))
	{}

	FunctionDefinition::~FunctionDefinition()
	{}

	std::string FunctionDefinition::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		for (auto& f : this->body->functions)
		{
			str << f->to_string(depth + 1);
			str << '\n';
		}

		//str << this->prototype->to_string(depth);
		//str << '\n';
		str << tabs << "Function Body : {" << '\n';
		str << tabs << '\t' << "Name: " << module::StringManager::get_string(this->prototype->name_id) << '\n';
		str << this->body->to_string(depth + 1);
		str << tabs << '}' << '\n';

		return str.str();
	}

	bool FunctionDefinition::check_return_type()
	{
		if (this->prototype->return_type == types::TypeEnum::Void)
		{
			return true;
		}
		else if (this->prototype->return_type == this->body->get_result_type())
		{
			return true;
		}

		return false;
	}

	void change_ast_object(ast::BaseExpr* object, ptr_type<ast::BaseExpr> new_object)
	{
		if (object == nullptr)
		{
			assert(false && "cannot change null ast object");
			return;
		}

		parent_data object_parent = object->get_parent_data();

		switch (object_parent.parent->get_type())
		{
			case AstExprType::BodyExpr:
			{
				BodyExpr* expr = dynamic_cast<BodyExpr*>(object_parent.parent);
				expr->expressions[object_parent.location] = std::move(new_object);
				break;
			}
			case AstExprType::VariableDeclarationExpr:
			{
				VariableDeclarationExpr* expr = dynamic_cast<VariableDeclarationExpr*>(object_parent.parent);
				expr->expr = std::move(new_object);
				break;
			}
			case AstExprType::BinaryExpr:
			{
				BinaryExpr* expr = dynamic_cast<BinaryExpr*>(object_parent.parent);
				if (object_parent.location == 0)
				{
					expr->lhs = std::move(new_object);
				}
				else
				{
					expr->rhs = std::move(new_object);
				}
				break;
			}
			case AstExprType::CallExpr:
			{
				CallExpr* expr = dynamic_cast<CallExpr*>(object_parent.parent);
				expr->args[object_parent.location] = std::move(new_object);
				break;
			}
			case AstExprType::IfExpr:
			{
				IfExpr* expr = dynamic_cast<IfExpr*>(object_parent.parent);
				if (object_parent.location == 0)
				{
					expr->condition = std::move(new_object);
				}
				else if (object_parent.location == 1)
				{
					expr->if_body = std::move(new_object);
				}
				else
				{
					expr->else_body = std::move(new_object);
				}
				break;
			}
			case AstExprType::ForExpr:
			{
				ForExpr* expr = dynamic_cast<ForExpr*>(object_parent.parent);
				if (object_parent.location == 0)
				{
					expr->start_expr = std::move(new_object);
				}
				else if (object_parent.location == 1)
				{
					expr->end_expr = std::move(new_object);
				}
				else if (object_parent.location == 2)
				{
					expr->start_expr = std::move(new_object);
				}
				else
				{
					ast::BaseExpr* baseExpr = new_object.get();
					new_object.reset();

					ast::BodyExpr* bodyExpr = dynamic_cast<ast::BodyExpr*>(baseExpr);
					if (bodyExpr == nullptr)
					{
						assert(false && "expr is not of type BodyExpr");
					}
					expr->for_body = ptr_type<ast::BodyExpr>(bodyExpr);
				}
				break;
			}
			case AstExprType::WhileExpr:
			{
				WhileExpr* expr = dynamic_cast<WhileExpr*>(object_parent.parent);
				if (object_parent.location == 0)
				{
					expr->end_expr = std::move(new_object);
				}
				else
				{
					expr->while_body = std::move(new_object);
				}
				break;
			}
			case AstExprType::UnaryExpr:
			{
				UnaryExpr* expr = dynamic_cast<UnaryExpr*>(object_parent.parent);
				expr->expr = std::move(new_object);
				break;
			}
			default:
			{
				assert(false && "cannot change ast object");
			}
		}
	}

}
