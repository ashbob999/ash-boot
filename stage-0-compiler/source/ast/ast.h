#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "llvm/IR/Instructions.h"

#include "types.h"
#include "operators.h"

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
		IfExpr,
		ForExpr,
		WhileExpr,
		CommentExpr,
		ReturnExpr,
		ContinueExpr,
		BreakExpr,
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
	class ForExpr;
	class WhileExpr;
	class CommentExpr;
	class ReturnExpr;
	class ContinueExpr;
	class BreakExpr;

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
		bool is_name_mangled = false;

	public:
		BaseExpr(AstExprType ast_type, BodyExpr* body);
		virtual ~BaseExpr() = default;
		virtual std::string to_string(int depth) = 0;
		virtual types::Type get_result_type() = 0;
		virtual bool check_types() = 0;

		virtual AstExprType get_type() final;
		virtual BodyExpr* get_body() final;
		virtual void set_body(BodyExpr* body) final;
		virtual void set_line_info(ExpressionLineInfo line_info) final;
		virtual ExpressionLineInfo& get_line_info() final;
		virtual void set_mangled(bool mangled) final;
		virtual bool is_mangled() final;
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

	enum class BodyType
	{
		Global,
		Function,
		Conditional,
		Loop,
	};

	// A collection of multiple BaseExpr
	class BodyExpr : public BaseExpr
	{
	public:
		BodyExpr(BodyExpr* body, BodyType body_type);
		~BodyExpr() override;
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		void add_base(shared_ptr<BaseExpr> expr);
		void add_function(shared_ptr<FunctionDefinition> func);
		void add_prototype(FunctionPrototype* proto);
		// TODO: remove?
		llvm::Type* get_llvm_type(llvm::LLVMContext& llvm_context, int str_id);

		//bool is_function_body = false;
		FunctionPrototype* parent_function = nullptr;
		BodyType body_type;
		std::vector<std::pair<int, ReferenceType>> in_scope_vars; // only used for checking vars are in scope, and order of defines
		std::vector<shared_ptr<BaseExpr>> expressions;
		std::vector<shared_ptr<FunctionDefinition>> functions;
		std::map<int, FunctionPrototype*> function_prototypes;
		//std::map<std::string, FunctionDefinition*> functions;
		std::map<int, types::Type> named_types;
		std::map<int, llvm::Type*> llvm_named_types;
		std::map<int, llvm::AllocaInst*> llvm_named_values;
		std::vector<int> extern_functions;
	};

	// Any variable declaration
	class VariableDeclarationExpr : public BaseExpr
	{
	public:
		VariableDeclarationExpr(BodyExpr* body, types::Type curr_type, int name_id, shared_ptr<BaseExpr> expr);
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
		VariableReferenceExpr(BodyExpr* body, int name_id);
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		int name_id;
	};

	// Any binary expression
	class BinaryExpr : public BaseExpr
	{
	public:
		BinaryExpr(BodyExpr* body, operators::BinaryOp binop, shared_ptr<BaseExpr> lhs, shared_ptr<BaseExpr> rhs);
		~BinaryExpr() override;
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		operators::BinaryOp binop;
		shared_ptr<BaseExpr> lhs;
		shared_ptr<BaseExpr> rhs;
	};

	class CallExpr : public BaseExpr, std::enable_shared_from_this<CallExpr>
	{
	public:
		CallExpr(BodyExpr* body, int callee_id, std::vector<shared_ptr<BaseExpr>>& args);
		~CallExpr() override;
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		int callee_id;
		std::vector<shared_ptr<BaseExpr>> args;
		bool is_extern = false;
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

	class ForExpr : public BaseExpr
	{
	public:
		ForExpr(BodyExpr* body, types::Type var_type, int name_id, shared_ptr<BaseExpr> start_expr, shared_ptr<BaseExpr> end_expr, shared_ptr<BaseExpr> step_expr, shared_ptr<BodyExpr> for_body);
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		types::Type var_type;
		int name_id;
		shared_ptr<BaseExpr> start_expr;
		shared_ptr<BaseExpr> end_expr;
		shared_ptr<BaseExpr> step_expr;
		shared_ptr<BodyExpr> for_body;
	};

	class WhileExpr : public BaseExpr
	{
	public:
		WhileExpr(BodyExpr* body, shared_ptr<BaseExpr> end_expr, shared_ptr<BodyExpr> while_body);
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		shared_ptr<BaseExpr> end_expr;
		shared_ptr<BodyExpr> while_body;
	};

	class CommentExpr : public BaseExpr
	{
	public:
		CommentExpr(BodyExpr* body);
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;
	};

	class ReturnExpr : public BaseExpr
	{
	public:
		ReturnExpr(BodyExpr* body, shared_ptr<BaseExpr> ret_expr);
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		shared_ptr<BaseExpr> ret_expr;
	};

	class ContinueExpr : public BaseExpr
	{
	public:
		ContinueExpr(BodyExpr* body);
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;
	};

	class BreakExpr : public BaseExpr
	{
	public:
		BreakExpr(BodyExpr* body);
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;
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
		bool is_extern = false;
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
