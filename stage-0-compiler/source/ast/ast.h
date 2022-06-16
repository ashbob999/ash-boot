#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "llvm/IR/Instructions.h"

#include "types.h"

using std::shared_ptr;

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
		IfExpr
	};

	struct ExpressionLineInfo
	{
		int start_line = 0;
		int end_line = 0;
		int start_pos = 0;
		int end_pos = 0;
		std::string line;
	};

	enum class ReferenceType
	{
		Variable,
		Function,
	};

	// TODO: delete all copy/move construcotrs or use smart pointers

	class BaseExpr;
	class LiteralExpr;
	class BodyExpr;
	class VariableDeclarationExpr;
	class VariableReferenceExpr;
	class BinaryExpr;
	class CallExpr;
	class IfExpr;

	class FunctionPrototype;
	class FunctionDefinition;

	// The expr ast class
	class BaseExpr
	{
	protected:
		AstExprType ast_type = AstExprType::BaseExpr;
		BodyExpr* body;
		types::Type result_type = types::Type::None;
		ExpressionLineInfo line_info;

	public:
		BaseExpr(AstExprType ast_type, BodyExpr* body);
		virtual ~BaseExpr() = default;
		virtual std::string to_string(int depth) = 0;
		virtual types::Type get_result_type() = 0;
		virtual bool check_types() = 0;

		virtual AstExprType get_type() final;
		virtual BodyExpr* get_body() final;
		virtual void set_line_info(ExpressionLineInfo line_info) final;
		virtual ExpressionLineInfo& get_line_info() final;
	};

	// Any literal value
	class LiteralExpr : public BaseExpr
	{
	public:
		LiteralExpr(BodyExpr* body, types::Type curr_type, std::string& str);
		~LiteralExpr() override;
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		types::Type curr_type;
		shared_ptr<types::BaseType> value_type;
	};

	// A collection of multiple BaseExpr
	class BodyExpr : public BaseExpr
	{
	public:
		BodyExpr(BodyExpr* body);
		~BodyExpr() override;
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		void add_base(shared_ptr<BaseExpr> expr);
		void add_function(shared_ptr<FunctionDefinition> func);
		void add_prototype(FunctionPrototype* proto);
		// TODO: remove?
		llvm::Type* get_llvm_type(llvm::LLVMContext& llvm_context, int str_id);

		bool is_function_body = false;
		std::vector<std::pair<int, ReferenceType>> in_scope_vars; // only used for checking vars are in scope, and order of defines
		std::vector<shared_ptr<BaseExpr>> expressions;
		std::vector<shared_ptr<FunctionDefinition>> functions;
		std::map<int, FunctionPrototype*> function_prototypes;
		//std::map<std::string, FunctionDefinition*> functions;
		std::map<int, types::Type> named_types;
		std::map<int, llvm::Type*> llvm_named_types;
		std::map<int, llvm::AllocaInst*> llvm_named_values;
	};

	// Any variable declaration
	class VariableDeclarationExpr : public BaseExpr
	{
	public:
		VariableDeclarationExpr(BodyExpr* body, types::Type curr_type, std::string& str, shared_ptr<BaseExpr> expr);
		~VariableDeclarationExpr() override;
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		types::Type curr_type;
		int name_id;
		shared_ptr<BaseExpr> expr;
	};

	// Any variable reference
	class VariableReferenceExpr : public BaseExpr
	{
	public:
		VariableReferenceExpr(BodyExpr* body, std::string& str);
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		int name_id;
	};

	// The different binary operators
	enum class BinaryOp
	{
		None,
		Assignment,
		Addition,
		Subtraction,
		Multiplication,
		LessThan,
		GreaterThan,
	};

	BinaryOp is_binary_op(char c);
	bool is_binary_comparision(BinaryOp op);

	std::string to_string(BinaryOp binop);

	// Any binary expression
	class BinaryExpr : public BaseExpr
	{
	public:
		BinaryExpr(BodyExpr* body, BinaryOp binop, shared_ptr<BaseExpr> lhs, shared_ptr<BaseExpr> rhs);
		~BinaryExpr() override;
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		BinaryOp binop;
		shared_ptr<BaseExpr> lhs;
		shared_ptr<BaseExpr> rhs;
	};

	class CallExpr : public BaseExpr, std::enable_shared_from_this<CallExpr>
	{
	public:
		CallExpr(BodyExpr* body, std::string& callee, std::vector<shared_ptr<BaseExpr>>& args);
		~CallExpr() override;
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		int callee_id;
		std::vector<shared_ptr<BaseExpr>> args;
	};

	class IfExpr : public BaseExpr
	{
	public:
		IfExpr(BodyExpr* body, shared_ptr<BaseExpr> condition, shared_ptr<BaseExpr> if_body, shared_ptr<BaseExpr>else_body);
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		bool should_return_value = true; // should the if return a value after being evaluated
		shared_ptr<BaseExpr> condition;
		shared_ptr<BaseExpr> if_body;
		shared_ptr<BaseExpr> else_body;
	};

	// The prototype for a function (i.e. the definition)
	class FunctionPrototype
	{
	public:
		FunctionPrototype(std::string& name, types::Type return_type, std::vector<types::Type>& types, std::vector<int>& args);
		std::string to_string(int depth);
		FunctionPrototype(const FunctionPrototype&) = delete;

		int name_id;
		types::Type return_type;
		std::vector<types::Type> types;
		std::vector<int> args;
	};

	// The function definition (i.e the body)
	class FunctionDefinition
	{
	public:
		FunctionDefinition(FunctionPrototype* prototype, shared_ptr<BodyExpr> body);
		~FunctionDefinition();
		std::string to_string(int depth);
		bool check_return_type();

		FunctionPrototype* prototype;
		shared_ptr<BodyExpr> body;
	};
}
