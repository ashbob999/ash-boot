#include <sstream>

#include "ast.h"

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

	LiteralExpr::LiteralExpr(BodyExpr* body, types::Type curr_type, std::string str)
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

	VariableDeclarationExpr::VariableDeclarationExpr(BodyExpr* body, types::Type curr_type, std::string str, ast::BaseExpr* expr)
		: BaseExpr(AstExprType::VariableDeclarationExpr, body), curr_type(curr_type), name(str), expr(expr)
	{}

	VariableDeclarationExpr::~VariableDeclarationExpr()
	{
		if (expr != nullptr)
		{
			delete expr;
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

	VariableReferenceExpr::VariableReferenceExpr(BodyExpr* body, std::string str)
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

	BinaryExpr::BinaryExpr(BodyExpr* body, BinaryOp binop, BaseExpr* lhs, BaseExpr* rhs)
		: BaseExpr(AstExprType::BinaryExpr, body), binop(binop), lhs(lhs), rhs(rhs)
	{}

	BinaryExpr::~BinaryExpr()
	{
		if (lhs != nullptr)
		{
			delete lhs;
		}
		if (rhs != nullptr)
		{
			delete rhs;
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

	BodyExpr::BodyExpr()
		: BaseExpr(AstExprType::BodyExpr, this)
	{}

	BodyExpr::~BodyExpr()
	{
		for (auto& base : expressions)
		{
			if (base != nullptr)
			{
				delete base;
			}
		}

		for (auto& func : functions)
		{
			if (func != nullptr)
			{
				delete func;
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

	void BodyExpr::add_base(BaseExpr* expr)
	{
		expressions.push_back(expr);
	}

	void BodyExpr::add_function(FunctionDefinition* func)
	{
		functions.push_back(func);
	}

	CallExpr::CallExpr(ast::BodyExpr* body, std::string callee, std::vector<BaseExpr*> args)
		: BaseExpr(AstExprType::CallExpr, body), callee(callee), args(args)
	{}

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

	FunctionDefinition::FunctionDefinition(FunctionPrototype* prototype, BaseExpr* body)
		: prototype(prototype), body(body)
	{}

	FunctionDefinition::~FunctionDefinition()
	{
		if (body != nullptr)
		{
			delete body;
		}
	}

	std::string FunctionDefinition::to_string(int depth)
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		for (auto& f : this->body->get_body()->functions)
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

}
