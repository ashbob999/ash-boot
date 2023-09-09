#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "llvm/IR/Instructions.h"

#include "../config.h"
#include "types.h"
#include "operators.h"

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
		UnaryExpr,
		CastExpr,
		SwitchExpr,
		CaseExpr,
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

	// Values must be defind with most constant at 0 & least constant at INT_MAX
	enum class ConstantStatus : int
	{
		Unknown,
		Constant,
		CanBeConstant,
		Variable,
	};

	inline constexpr ConstantStatus operator|(const ConstantStatus& left, const ConstantStatus& right)
	{
		int left_int = static_cast<int>(left);
		int right_int = static_cast<int>(right);

		return static_cast<ConstantStatus>(std::max(left_int, right_int));
	}

	inline constexpr ConstantStatus& operator|=(ConstantStatus& left, const ConstantStatus& right)
	{
		left = left | right;
		return left;
	}

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
	class UnaryExpr;
	class CastExpr;
	class SwitchExpr;
	class CaseExpr;

	class FunctionPrototype;
	class FunctionDefinition;

	struct parent_data
	{
		ast::BaseExpr* parent = nullptr;
		// location stores information about where the child is located within the parent object
		//     BodyExpr: location is index of the expressions
		//     VariableDeclarationExpr: location is 0 for expression
		//     BinaryExpr: location is 0 for lhs, and 1 for rhs
		//     CallExpr: location is index of the arguments
		//     IfExpr: location is 0 for condition, 1 for if body, and 2 for else body
		//     ForExpr: location is 0 for start,  1 for end, 2 for step, and 3 for body
		//     WhileExpr: location is 0 for end, and 1 for body
		//     UnaryExpr: location is 0 for expresion
		//     SwitchExpr: location is index of the case expression
		int location = 0;
	};

	// The expr ast class
	class BaseExpr
	{
	public:
		BaseExpr(AstExprType ast_type, BodyExpr* body);
		virtual ~BaseExpr() = default;
		virtual std::string to_string(int depth) = 0;
		virtual types::Type get_result_type() = 0;
		virtual bool check_types() = 0;

		// TODO: maybe remove virtual on final methods
		virtual AstExprType get_type() final;
		virtual BodyExpr* get_body() final;
		virtual void set_body(BodyExpr* body) final;
		virtual void set_line_info(ExpressionLineInfo line_info) final;
		virtual ExpressionLineInfo& get_line_info() final;
		virtual void set_mangled(bool mangled) final;
		virtual bool is_mangled() final;
		virtual void set_parent_data(BaseExpr* parent, int location) final;
		virtual parent_data get_parent_data() final;
		virtual bool is_constant()const final;

		ConstantStatus constant_status = ConstantStatus::Unknown;

	protected:
		AstExprType ast_type = AstExprType::BaseExpr;
		BodyExpr* body;
		types::Type result_type{ types::TypeEnum::None };
		ExpressionLineInfo line_info;
		bool is_name_mangled = false;
		parent_data parent;
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
		ptr_type<types::BaseType> value_type;
	};

	enum class BodyType
	{
		Global,
		Function,
		Conditional,
		Loop,
		Case,
		ScopeBlock,
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

		void add_base(ptr_type<BaseExpr> expr);
		void add_function(ptr_type<FunctionDefinition> func);
		void add_prototype(FunctionPrototype* proto);
		// TODO: remove?
		llvm::Type* get_llvm_type(llvm::LLVMContext& llvm_context, int str_id);

		//bool is_function_body = false;
		FunctionPrototype* parent_function = nullptr;
		BodyType body_type;
		std::vector<std::pair<int, ReferenceType>> in_scope_vars; // only used for checking vars are in scope, and order of defines
		std::vector<ptr_type<BaseExpr>> expressions;
		std::vector<ptr_type<FunctionDefinition>> functions;
		std::vector<FunctionPrototype*> original_function_prototypes;
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
		VariableDeclarationExpr(BodyExpr* body, types::Type curr_type, int name_id, ptr_type<BaseExpr> expr);
		~VariableDeclarationExpr() override;
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		types::Type curr_type;
		int name_id;
		ptr_type<BaseExpr> expr;
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
		BinaryExpr(BodyExpr* body, operators::BinaryOp binop, ptr_type<BaseExpr> lhs, ptr_type<BaseExpr> rhs);
		~BinaryExpr() override;
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		operators::BinaryOp binop;
		ptr_type<BaseExpr> lhs;
		ptr_type<BaseExpr> rhs;
	};

	class CallExpr : public BaseExpr
	{
	public:
		CallExpr(BodyExpr* body, int callee_id, std::vector<ptr_type<BaseExpr>>& args);
		~CallExpr() override;
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		int callee_id;
		int unmangled_callee_id;
		std::vector<ptr_type<BaseExpr>> args;
		bool is_extern = false;
	};

	class IfExpr : public BaseExpr
	{
	public:
		IfExpr(BodyExpr* body, ptr_type<BaseExpr> condition, ptr_type<BaseExpr> if_body, ptr_type<BaseExpr>else_body, bool should_return_value);
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		bool should_return_value = true; // should the if return a value after being evaluated
		ptr_type<BaseExpr> condition;
		ptr_type<BaseExpr> if_body;
		ptr_type<BaseExpr> else_body;
	};

	class ForExpr : public BaseExpr
	{
	public:
		ForExpr(BodyExpr* body, types::Type var_type, int name_id, ptr_type<BaseExpr> start_expr, ptr_type<BaseExpr> end_expr, ptr_type<BaseExpr> step_expr, ptr_type<BodyExpr> for_body);
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		types::Type var_type;
		int name_id;
		ptr_type<BaseExpr> start_expr;
		ptr_type<BaseExpr> end_expr;
		ptr_type<BaseExpr> step_expr;
		ptr_type<BodyExpr> for_body;
	};

	class WhileExpr : public BaseExpr
	{
	public:
		WhileExpr(BodyExpr* body, ptr_type<BaseExpr> end_expr, ptr_type<BaseExpr> while_body);
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		ptr_type<BaseExpr> end_expr;
		ptr_type<BaseExpr> while_body;
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
		ReturnExpr(BodyExpr* body, ptr_type<BaseExpr> ret_expr);
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		ptr_type<BaseExpr> ret_expr;
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

	class UnaryExpr : public BaseExpr
	{
	public:
		UnaryExpr(BodyExpr* body, operators::UnaryOp unop, ptr_type<BaseExpr> expr);
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		operators::UnaryOp unop;
		ptr_type<BaseExpr> expr;
	};

	class CastExpr : public BaseExpr
	{
	public:
		CastExpr(BodyExpr* body, int target_type_id, ptr_type<BaseExpr> expr);
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		int target_type_id;
		types::Type target_type;
		ptr_type<BaseExpr> expr;
	};

	class SwitchExpr : public BaseExpr
	{
	public:
		SwitchExpr(BodyExpr* body, ptr_type<BaseExpr> value_expr, std::vector<ptr_type<CaseExpr>>& cases);
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		bool should_return_value = false; // should the switch return a value after being evaluated
		ptr_type<BaseExpr> value_expr;
		std::vector<ptr_type<CaseExpr>> cases;
	};

	class CaseExpr : public BaseExpr
	{
	public:
		CaseExpr(BodyExpr* body, ptr_type<BaseExpr> case_expr, ptr_type<BaseExpr> case_body, bool default_case);
		std::string to_string(int depth) override;
		types::Type get_result_type() override;
		bool check_types() override;

		// bool should_return_value = false; // should the switch return a value after being evaluated
		ptr_type<BaseExpr> case_expr;
		ptr_type<BaseExpr> case_body;
		bool default_case = false;
	};

	// The prototype for a function (i.e. the definition)
	class FunctionPrototype
	{
	public:
		FunctionPrototype(std::string& name, types::Type return_type, std::vector<types::Type>& types, std::vector<int>& args);
		std::string to_string(int depth);
		FunctionPrototype(const FunctionPrototype&) = delete;

		int name_id;
		int unmangled_name_id;
		types::Type return_type;
		std::vector<types::Type> types;
		std::vector<int> args;
		bool is_extern = false;
	};

	// The function definition (i.e the body)
	class FunctionDefinition
	{
	public:
		FunctionDefinition(FunctionPrototype* prototype, ptr_type<BodyExpr> body);
		~FunctionDefinition();
		std::string to_string(int depth);
		bool check_return_type();

		FunctionPrototype* prototype;
		ptr_type<BodyExpr> body;
	};

	void change_ast_object(ast::BaseExpr* object, ptr_type<ast::BaseExpr> new_object);
}
