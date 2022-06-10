#pragma once

#include <string>
#include <vector>
#include <map>

#include "llvm/IR/Instructions.h"

#include "types.h"

namespace ast
{
	enum class AstExprType
	{
		BaseExpr,
		LiteralExpr,
		BodyExpr,
		VariableDeclarationExpr,
		VariableReferenceExpr,
		BinaryExpr,
		CallExpr,
	};

	// TODO: delete all copy/move construcotrs or use smart pointers

	class BaseExpr;
	class LiteralExpr;
	class BodyExpr;
	class VariableDeclarationExpr;
	class VariableReferenceExpr;
	class BinaryExpr;
	class CallExpr;

	class FunctionPrototype;
	class FunctionDefinition;

	// The expr ast class
	class BaseExpr
	{
	protected:
		AstExprType ast_type = AstExprType::BaseExpr;
		BodyExpr* body;

	public:
		BaseExpr(AstExprType ast_type, BodyExpr* body);
		virtual ~BaseExpr() = default;
		virtual std::string to_string(int depth) = 0;

		virtual AstExprType get_type() final;
		virtual BodyExpr* get_body() final;
	};

	// Any literal value
	class LiteralExpr : public BaseExpr
	{
	public:
		LiteralExpr(BodyExpr* body, types::Type curr_type, std::string str);
		~LiteralExpr() override;
		std::string to_string(int depth) override;

		types::Type curr_type;
		types::BaseType* value_type;
	};

	// A collection of multiple BaseExpr
	class BodyExpr : public BaseExpr
	{
	public:
		BodyExpr();
		~BodyExpr() override;
		std::string to_string(int depth) override;

		void add_base(BaseExpr* expr);
		void add_function(FunctionDefinition* func);

		std::vector<BaseExpr*> expressions;
		std::vector<FunctionDefinition*> functions;
		std::map<std::string, llvm::Type*> named_types;
		std::map<std::string, llvm::AllocaInst*> named_values;
	};

	// Any variable declaration
	class VariableDeclarationExpr : public BaseExpr
	{
	public:
		VariableDeclarationExpr(BodyExpr* body, types::Type curr_type, std::string str, ast::BaseExpr* expr);
		~VariableDeclarationExpr() override;
		std::string to_string(int depth) override;

		types::Type curr_type;
		std::string name;
		ast::BaseExpr* expr;
	};

	// Any variable reference
	class VariableReferenceExpr : public BaseExpr
	{
	public:
		VariableReferenceExpr(BodyExpr* body, std::string str);
		std::string to_string(int depth) override;

		std::string name;
	};

	// The different binary operators
	enum class BinaryOp
	{
		None,
		Assignment,
		Addition,
		Subtraction,
		Multiplication,
	};

	BinaryOp is_binary_op(char c);

	std::string to_string(BinaryOp binop);

	// Any binary expression
	class BinaryExpr : public BaseExpr
	{
	public:
		BinaryExpr(BodyExpr* body, BinaryOp binop, BaseExpr* lhs, BaseExpr* rhs);
		~BinaryExpr() override;
		std::string to_string(int depth) override;

		BinaryOp binop;
		BaseExpr* lhs;
		BaseExpr* rhs;
	};

	class CallExpr :public BaseExpr
	{
	public:
		CallExpr(ast::BodyExpr* body, std::string callee, std::vector<BaseExpr*> args);
		std::string to_string(int depth) override;

		std::string callee;
		std::vector<BaseExpr*> args;
	};

	// The prototype for a function (i.e. the definition)
	class FunctionPrototype
	{
	public:
		FunctionPrototype(std::string name, types::Type return_type, std::vector<types::Type> types, std::vector<std::string> args);
		std::string to_string(int depth);

		std::string name;
		types::Type return_type;
		std::vector<types::Type> types;
		std::vector<std::string> args;
	};

	// The function definition (i.e the body)
	class FunctionDefinition
	{
	public:
		FunctionDefinition(FunctionPrototype* prototype, BaseExpr* body);
		~FunctionDefinition();
		std::string to_string(int depth);

		FunctionPrototype* prototype;
		BaseExpr* body;
	};
}
