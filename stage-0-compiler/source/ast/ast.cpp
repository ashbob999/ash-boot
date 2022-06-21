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
		if (result_type == types::Type::None)
		{
			result_type = this->curr_type;
		}
		return result_type;
	}

	bool LiteralExpr::check_types()
	{
		if (result_type == types::Type::None)
		{
			result_type = this->curr_type;
		}

		return true;
	}

	VariableDeclarationExpr::VariableDeclarationExpr(BodyExpr* body, types::Type curr_type, std::string& str, shared_ptr<BaseExpr> expr)
		: BaseExpr(AstExprType::VariableDeclarationExpr, body), curr_type(curr_type), name_id(module::StringManager::get_id(str)), expr(expr)
	{}

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
			str << tabs << '\t' << '\t' << "(default)" << '\n';
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
		if (result_type == types::Type::None)
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

	VariableReferenceExpr::VariableReferenceExpr(BodyExpr* body, std::string& str)
		: BaseExpr(AstExprType::VariableReferenceExpr, body), name_id(module::StringManager::get_id(str))
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
		if (result_type == types::Type::None)
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

	BinaryOp is_binary_op(char c)
	{
		if (c == '=')
		{
			return BinaryOp::Assignment;
		}
		else if (c == '+')
		{
			return BinaryOp::Addition;
		}
		else if (c == '-')
		{
			return BinaryOp::Subtraction;
		}
		else if (c == '*')
		{
			return BinaryOp::Multiplication;
		}
		else if (c == '/')
		{
			return BinaryOp::Division;
		}
		else if (c == '<')
		{
			return BinaryOp::LessThan;
		}
		else if (c == '>')
		{
			return BinaryOp::GreaterThan;
		}
		return BinaryOp::None;
	}

	bool is_binary_comparision(BinaryOp op)
	{
		switch (op)
		{
			case BinaryOp::LessThan:
			case BinaryOp::GreaterThan:
			{
				return true;
			}
			default:
			{
				return false;
			}
		}
	}

	std::string to_string(BinaryOp binop)
	{
		switch (binop)
		{
			case ast::BinaryOp::None:
			{
				return "None";
			}
			case ast::BinaryOp::Assignment:
			{
				return "Assignment (=)";
			}
			case ast::BinaryOp::Addition:
			{
				return "Addition (+)";
			}
			case ast::BinaryOp::Subtraction:
			{
				return "Subtraction (-)";
			}
			case ast::BinaryOp::Multiplication:
			{
				return "Multiplication (*)";
			}
			case ast::BinaryOp::Division:
			{
				return "Division (/)";
			}
			case ast::BinaryOp::LessThan:
			{
				return "LessThan (<)";
			}
			case ast::BinaryOp::GreaterThan:
			{
				return "LessThan (<)";
			}
		}
	}

	bool is_type_supported(BinaryOp binop, types::Type type)
	{
		switch (type)
		{
			case types::Type::Int:
			case types::Type::Float:
			{
				return true; // all operators supported
			}
			case types::Type::Bool:
			{
				if (is_binary_comparision(binop) || binop == BinaryOp::Assignment)
				{
					return true; // only support comparisons for bools
				}
				else
				{
					return false;
				}
			}
			default:
			{
				return false;
			}
		}
	}

	BinaryExpr::BinaryExpr(BodyExpr* body, BinaryOp binop, shared_ptr<BaseExpr> lhs, shared_ptr<BaseExpr> rhs)
		: BaseExpr(AstExprType::BinaryExpr, body), binop(binop), lhs(lhs), rhs(rhs)
	{}

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
		str << tabs << '\t' << "Operator: " << ast::to_string(this->binop) << '\n';
		str << tabs << '\t' << "RHS: {" << '\n';
		str << this->rhs->to_string(depth + 2);
		str << tabs << '\t' << "}," << '\n';
		str << tabs << "}," << '\n';

		return str.str();
	}

	types::Type BinaryExpr::get_result_type()
	{
		if (result_type == types::Type::None)
		{
			if (is_binary_comparision(this->binop))
			{
				result_type = types::Type::Bool;
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
		if (this->lhs->get_result_type() == this->rhs->get_result_type())
		{
			return true;
		}

		return false;
	}

	BodyExpr::BodyExpr(BodyExpr* body)
		: BaseExpr(AstExprType::BodyExpr, body)
	{}

	BodyExpr::~BodyExpr()
	{
		for (auto& proto : function_prototypes)
		{
			if (proto.second != nullptr)
			{
				delete proto.second;
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
		if (result_type == types::Type::None)
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

	void BodyExpr::add_base(shared_ptr<BaseExpr> expr)
	{
		expressions.push_back(expr);
	}

	void BodyExpr::add_function(shared_ptr<FunctionDefinition> func)
	{
		functions.push_back(func);
		//auto s = std::make_shared<FunctionPrototype>(func->prototype);
		function_prototypes[func->prototype->name_id] = func->prototype;
	}

	void BodyExpr::add_prototype(FunctionPrototype* proto)
	{
		function_prototypes[proto->name_id] = proto;
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

	CallExpr::CallExpr(BodyExpr* body, std::string& callee, std::vector<shared_ptr<BaseExpr>>& args)
		: BaseExpr(AstExprType::CallExpr, body), callee_id(module::StringManager::get_id(callee)), args(args)
	{}

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
		if (result_type == types::Type::None)
		{
			//result_type = scope::get_scope(shared_ptr<CallExpr>(this))->function_prototypes[this->callee]->return_type;
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
		//shared_ptr<FunctionPrototype> proto = scope::get_scope(shared_ptr<CallExpr>(this))->function_prototypes[this->callee];
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

	IfExpr::IfExpr(BodyExpr* body, shared_ptr<BaseExpr> condition, shared_ptr<BaseExpr> if_body, shared_ptr<BaseExpr> else_body)
		: BaseExpr(AstExprType::IfExpr, body), condition(condition), if_body(if_body), else_body(else_body)
	{}

	std::string IfExpr::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "if statement: {" << '\n';

		str << tabs << '\t' << "condition: {" << '\n';
		str << this->condition->to_string(depth + 2);
		str << tabs << '\t' << "}," << '\n';

		str << tabs << '\t' << "if body: {" << '\n';
		str << this->if_body->to_string(depth + 2);
		str << tabs << '\t' << "}," << '\n';

		str << tabs << '\t' << "else body: {" << '\n';
		str << this->else_body->to_string(depth + 2);
		str << tabs << '\t' << "}" << '\n';

		str << tabs << "}, " << '\n';

		return str.str();
	}

	types::Type IfExpr::get_result_type()
	{
		if (result_type == types::Type::None)
		{
			if (this->should_return_value)
			{
				result_type = this->if_body->get_result_type();
			}
			else
			{
				result_type = types::Type::Void;
			}
		}
		return result_type;
	}

	bool IfExpr::check_types()
	{
		if (this->condition->get_result_type() == types::Type::Bool)
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

	ForExpr::ForExpr(BodyExpr* body, types::Type var_type, int name_id, shared_ptr<BaseExpr> start_expr, shared_ptr<BaseExpr> end_expr, shared_ptr<BaseExpr> step_expr, shared_ptr<BodyExpr> for_body)
		: BaseExpr(AstExprType::ForExpr, body), var_type(var_type), name_id(name_id), start_expr(start_expr), end_expr(end_expr), step_expr(step_expr), for_body(for_body)
	{}

	std::string ForExpr::to_string(int depth)
	{
		return std::string();
	}

	types::Type ForExpr::get_result_type()
	{
		if (result_type == types::Type::None)
		{
			result_type = types::Type::Void;
		}
		return result_type;
	}

	bool ForExpr::check_types()
	{
		if (end_expr->get_result_type() == types::Type::Bool)
		{
			return true;
		}
		return false;
	}

	WhileExpr::WhileExpr(BodyExpr* body, shared_ptr<BaseExpr> end_expr, shared_ptr<BodyExpr> while_body)
		: BaseExpr(AstExprType::WhileExpr, body), end_expr(end_expr), while_body(while_body)
	{}

	std::string WhileExpr::to_string(int depth)
	{
		return std::string();
	}

	types::Type WhileExpr::get_result_type()
	{
		if (result_type == types::Type::None)
		{
			result_type = types::Type::Void;
		}
		return result_type;
	}

	bool WhileExpr::check_types()
	{
		if (end_expr->get_result_type() == types::Type::Bool)
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
		return types::Type::None;
	}

	bool CommentExpr::check_types()
	{
		return true;
	}

	FunctionPrototype::FunctionPrototype(std::string& name, types::Type return_type, std::vector<types::Type>& types, std::vector<int>& args)
		: name_id(module::StringManager::get_id(name)), return_type(return_type), types(types), args(args)
	{}

	std::string FunctionPrototype::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "function prototype: {" << '\n';

		str << tabs << '\t' << "name: " << module::StringManager::get_string(this->name_id) << '\n';
		str << tabs << '\t' << "args: [" << '\n';

		for (int i = 0; i < this->args.size(); i++)
		{
			str << tabs << '\t' << '\t' << "{ " << module::StringManager::get_string(this->args[i]) << ": " << types::to_string(this->types[i]) << "}," << '\n';
		}

		str << tabs << '\t' << "]," << '\n';
		str << tabs << "}, " << '\n';

		return str.str();
	}

	FunctionDefinition::FunctionDefinition(FunctionPrototype* prototype, shared_ptr<BodyExpr> body)
		: prototype(prototype), body(body)
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
		str << tabs << "function body : {" << '\n';
		str << tabs << '\t' << "Name: " << module::StringManager::get_string(this->prototype->name_id) << '\n';
		str << this->body->to_string(depth + 1);
		str << tabs << '}' << '\n';

		return str.str();
	}

	bool FunctionDefinition::check_return_type()
	{
		if (this->prototype->return_type == types::Type::Void)
		{
			return true;
		}
		else if (this->prototype->return_type == this->body->get_result_type())
		{
			return true;
		}

		return false;
	}

}
