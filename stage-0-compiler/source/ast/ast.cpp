#include <sstream>

#include "ast.h"
#include "scope_checker.h"

namespace ast
{
	BaseExpr::BaseExpr(AstExprType ast_type, shared_ptr<BodyExpr> body)
		: ast_type(ast_type), body(body)
	{}

	AstExprType BaseExpr::get_type()
	{
		return this->ast_type;
	}

	shared_ptr<BodyExpr> BaseExpr::get_body()
	{
		return this->body;
	}

	void BaseExpr::set_line_info(LinePositionInfo line_info)
	{
		this->line_info = line_info;
	}

	LiteralExpr::LiteralExpr(shared_ptr<BodyExpr> body, types::Type curr_type, std::string str)
		: BaseExpr(AstExprType::LiteralExpr, body), curr_type(curr_type)
	{
		value_type = types::BaseType::create_type(curr_type, str);
	}

	LiteralExpr::~LiteralExpr()
	{
		if (value_type != nullptr)
		{
			delete value_type;
		}
	}

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

	VariableDeclarationExpr::VariableDeclarationExpr(shared_ptr<BodyExpr> body, types::Type curr_type, std::string str, shared_ptr<BaseExpr> expr)
		: BaseExpr(AstExprType::VariableDeclarationExpr, body), curr_type(curr_type), name(str), expr(expr)
	{}

	VariableDeclarationExpr::~VariableDeclarationExpr()
	{
		if (expr != nullptr)
		{
			//delete expr;
		}
	}

	std::string VariableDeclarationExpr::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Variable Declaration: {" << '\n';
		str << tabs << '\t' << "Name: " << this->name << '\n';
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

	VariableReferenceExpr::VariableReferenceExpr(shared_ptr<BodyExpr> body, std::string str)
		: BaseExpr(AstExprType::VariableReferenceExpr, body), name(str)
	{}

	std::string VariableReferenceExpr::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Variable Reference: {" << '\n';
		str << tabs << '\t' << "Name: " << this->name << '\n';
		str << tabs << "}," << '\n';

		return str.str();
	}

	types::Type VariableReferenceExpr::get_result_type()
	{
		if (result_type == types::Type::None)
		{
			//result_type = this->get_body()->named_types[this->name];
			result_type = scope::get_scope(this)->named_types[this->name];
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
		return BinaryOp::None;
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
		}
	}

	BinaryExpr::BinaryExpr(shared_ptr<BodyExpr> body, BinaryOp binop, shared_ptr<BaseExpr> lhs, shared_ptr<BaseExpr> rhs)
		: BaseExpr(AstExprType::BinaryExpr, body), binop(binop), lhs(lhs), rhs(rhs)
	{}

	BinaryExpr::~BinaryExpr()
	{
		if (lhs != nullptr)
		{
			//delete lhs;
		}
		if (rhs != nullptr)
		{
			//delete rhs;
		}
	}

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
			result_type = this->lhs->get_result_type();
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

	BodyExpr::BodyExpr(shared_ptr<BodyExpr> body)
		: BaseExpr(AstExprType::BodyExpr, body)
	{}

	BodyExpr::~BodyExpr()
	{
		for (auto& base : expressions)
		{
			if (base != nullptr)
			{
				//delete base;
			}
		}

		for (auto& func : functions)
		{
			if (func != nullptr)
			{
				//delete func;
			}
		}
	}

	std::string BodyExpr::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Body: {" << '\n';
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
			result_type = this->expressions.back()->get_result_type();
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
		function_prototypes.insert({ func->prototype->name, func->prototype });
	}

	llvm::Type* BodyExpr::get_llvm_type(llvm::LLVMContext& llvm_context, std::string str)
	{
		auto f = this->llvm_named_types.find(str);
		if (f != this->llvm_named_types.end())
		{
			return f->second;
		}
		llvm::Type* type = types::get_llvm_type(llvm_context, this->named_types[str]);
		this->llvm_named_types[str] = type;
		return type;
	}

	CallExpr::CallExpr(shared_ptr<BodyExpr> body, std::string callee, std::vector<shared_ptr<BaseExpr>> args)
		: BaseExpr(AstExprType::CallExpr, body), callee(callee), args(args)
	{}

	CallExpr::~CallExpr()
	{
		for (auto& e : args)
		{
			if (e != nullptr)
			{
				//delete e;
			}
		}
	}

	std::string CallExpr::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Function Call: {" << '\n';
		str << tabs << '\t' << "Name: " << this->callee << '\n';
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
			result_type = scope::get_scope(this)->function_prototypes[this->callee]->return_type;
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
		FunctionPrototype* proto = scope::get_scope(this)->function_prototypes[this->callee].get();

		for (int i = 0; i < this->args.size(); i++)
		{
			if (this->args[i]->get_result_type() != proto->types[i])
			{
				return false;
			}
		}

		return true;
	}

	FunctionPrototype::FunctionPrototype(std::string name, types::Type return_type, std::vector<types::Type> types, std::vector<std::string> args)
		: name(name), return_type(return_type), types(types), args(args)
	{}

	std::string FunctionPrototype::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "function prototype: {" << '\n';

		str << tabs << '\t' << "name: " << this->name << '\n';
		str << tabs << '\t' << "args: [" << '\n';

		for (int i = 0; i < this->args.size(); i++)
		{
			str << tabs << '\t' << '\t' << "{ " << this->args[i] << ": " << types::to_string(this->types[i]) << "}," << '\n';
		}

		str << tabs << '\t' << "]," << '\n';
		str << tabs << "}, " << '\n';

		return str.str();
	}

	FunctionDefinition::FunctionDefinition(shared_ptr<FunctionPrototype> prototype, shared_ptr<BodyExpr> body)
		: prototype(prototype), body(body)
	{}

	FunctionDefinition::~FunctionDefinition()
	{
		if (prototype != nullptr)
		{
			//delete prototype;
		}

		if (body != nullptr)
		{
			//delete body;
		}
	}

	std::string FunctionDefinition::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		for (auto& f : this->body->functions)
		{
			str << f->to_string(0);
			str << '\n';
		}

		str << this->prototype->to_string(depth);
		str << '\n';
		str << tabs << "function body : {" << '\n';
		str << tabs << '\t' << "Name: " << this->prototype->name << '\n';
		str << this->body->to_string(depth + 1);
		str << tabs << '}' << '\n';

		return str.str();
	}

	bool FunctionDefinition::check_return_type()
	{
		if (this->prototype->return_type == this->body->get_result_type())
		{
			return true;
		}

		return false;
	}

}
