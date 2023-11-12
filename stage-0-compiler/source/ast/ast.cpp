#include "ast.h"

#include "scope_checker.h"
#include "string_manager.h"

#include <sstream>

namespace ast
{
	static constexpr const char* ExprTypeKeyString = "expr_type";

	std::string constant_to_string(const BaseExpr* expr)
	{
		switch (expr->constant_status)
		{
			case ConstantStatus::Unknown:
			{
				return " (Unknown) ";
			}
			case ConstantStatus::Constant:
			{
				return " (Constant) ";
			}
			case ConstantStatus::CanBeConstant:
			{
				return " (Can Be Constant) ";
			}
			case ConstantStatus::Variable:
			{
				return " (Variable) ";
			}
		}
		return "";
	}

	BaseExpr::BaseExpr(AstExprType ast_type, BodyExpr* body) : ast_type(ast_type), body(body) {}

	AstExprType BaseExpr::get_type() const
	{
		return this->ast_type;
	}

	BodyExpr* BaseExpr::get_body() const
	{
		return this->body;
	}

	void BaseExpr::set_line_info(const ExpressionLineInfo& line_info)
	{
		this->line_info = line_info;
	}

	ExpressionLineInfo BaseExpr::get_line_info() const
	{
		return this->line_info;
	}

	void BaseExpr::set_mangled(bool mangled)
	{
		this->is_name_mangled = mangled;
	}

	bool BaseExpr::is_mangled() const
	{
		return this->is_name_mangled;
	}

	void BaseExpr::set_parent_data(BaseExpr* parent, int location)
	{
		this->parent.parent = parent;
		this->parent.location = location;
	}

	parent_data BaseExpr::get_parent_data() const
	{
		return this->parent;
	}

	bool BaseExpr::is_constant() const
	{
		return this->constant_status == ConstantStatus::Constant ||
			this->constant_status == ConstantStatus::CanBeConstant;
	}

	LiteralExpr::LiteralExpr(BodyExpr* body, const types::Type& curr_type, const std::string& str) :
		BaseExpr(AstExprType::LiteralExpr, body),
		curr_type(curr_type)
	{
		value_type = types::BaseType::create_type(curr_type, str);
	}

	LiteralExpr::~LiteralExpr() {}

	std::string LiteralExpr::to_string(int depth) const
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Literal Value : {" << constant_to_string(this) << '\n';
		str << tabs << '\t' << "Type: " << this->curr_type.to_string() << '\n';
		str << tabs << '\t' << "Value: " << this->value_type->to_string() << '\n';
		str << tabs << "}," << '\n';

		return str.str();
	}

	json::JsonValue LiteralExpr::to_json() const
	{
		json ::JsonObject root{};

		root.addData(ExprTypeKeyString, "Literal Value");
		// TODO: Check if these shouls use toJson
		root.addData("value_type", this->curr_type.to_string());
		root.addData("value", this->value_type->to_string());

		return root;
	}

	types::Type LiteralExpr::get_result_type()
	{
		if (result_type.get_type_enum() == types::TypeEnum::None)
		{
			result_type = this->curr_type;
		}
		return result_type;
	}

	bool LiteralExpr::check_types()
	{
		if (result_type.get_type_enum() == types::TypeEnum::None)
		{
			result_type = this->curr_type;
		}

		return true;
	}

	VariableDeclarationExpr::VariableDeclarationExpr(
		BodyExpr* body,
		types::Type curr_type,
		int name_id,
		ptr_type<BaseExpr> expr) :
		BaseExpr(AstExprType::VariableDeclarationExpr, body),
		curr_type(curr_type),
		name_id(name_id),
		expr(std::move(expr))
	{
		if (this->expr != nullptr)
		{
			this->expr->set_parent_data(this, 0);
		}
	}

	VariableDeclarationExpr::~VariableDeclarationExpr() {}

	std::string VariableDeclarationExpr::to_string(int depth) const
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Variable Declaration: {" << constant_to_string(this) << '\n';
		str << tabs << '\t' << "Name: " << stringManager::get_string(this->name_id) << '\n';
		str << tabs << '\t' << "Type: " << this->curr_type.to_string() << '\n';
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

	json::JsonValue VariableDeclarationExpr::to_json() const
	{
		json::JsonObject root{};

		root.addData(ExprTypeKeyString, "Variable Declaration");
		root.addData("variable_name", stringManager::get_string(this->name_id));
		root.addData("variable_type", this->curr_type.to_string());
		if (this->expr == nullptr)
		{
			root.addData("value", "<default>");
		}
		else
		{
			root.addData("value", this->expr->to_json());
		}

		return root;
	}

	types::Type VariableDeclarationExpr::get_result_type()
	{
		if (result_type.get_type_enum() == types::TypeEnum::None)
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

	VariableReferenceExpr::VariableReferenceExpr(BodyExpr* body, int name_id) :
		BaseExpr(AstExprType::VariableReferenceExpr, body),
		name_id(name_id)
	{}

	std::string VariableReferenceExpr::to_string(int depth) const
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Variable Reference: {" << constant_to_string(this) << '\n';
		str << tabs << '\t' << "Name: " << stringManager::get_string(this->name_id) << '\n';
		str << tabs << "}," << '\n';

		return str.str();
	}

	json::JsonValue VariableReferenceExpr::to_json() const
	{
		json::JsonObject root{};

		root.addData(ExprTypeKeyString, "Variable Reference");
		root.addData("variable_name", stringManager::get_string(this->name_id));

		return root;
	}

	types::Type VariableReferenceExpr::get_result_type()
	{
		if (result_type.get_type_enum() == types::TypeEnum::None)
		{
			// result_type = this->get_body()->named_types[this->name];
			result_type = scope::get_scope(this)->named_types[this->name_id];
		}
		return result_type;
	}

	bool VariableReferenceExpr::check_types()
	{
		return true;
	}

	BinaryExpr::BinaryExpr(BodyExpr* body, operators::BinaryOp binop, ptr_type<BaseExpr> lhs, ptr_type<BaseExpr> rhs) :
		BaseExpr(AstExprType::BinaryExpr, body),
		binop(binop),
		lhs(std::move(lhs)),
		rhs(std::move(rhs))
	{
		this->lhs->set_parent_data(this, 0);
		this->rhs->set_parent_data(this, 1);
	}

	BinaryExpr::~BinaryExpr() {}

	std::string BinaryExpr::to_string(int depth) const
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Binary Operation: {" << constant_to_string(this) << '\n';
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

	json::JsonValue BinaryExpr::to_json() const
	{
		json::JsonObject root{};

		root.addData(ExprTypeKeyString, "Binary");
		root.addData("left_hand_side", this->lhs->to_json());
		root.addData("operator", operators::to_string(this->binop));
		root.addData("right_hand_side", this->rhs->to_json());

		return root;
	}

	types::Type BinaryExpr::get_result_type()
	{
		if (result_type.get_type_enum() == types::TypeEnum::None)
		{
			if (is_binary_comparision(this->binop))
			{
				result_type = types::Type{types::TypeEnum::Bool};
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

	BodyExpr::BodyExpr(BodyExpr* body, BodyType body_type) : BaseExpr(AstExprType::BodyExpr, body), body_type(body_type)
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

	std::string BodyExpr::to_string(int depth) const
	{
		// TODO: show nested functions
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Body: {" << constant_to_string(this) << '\n';

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

	json::JsonValue BodyExpr::to_json() const
	{
		json::JsonObject root{};

		root.addData(ExprTypeKeyString, "Body");

		json::JsonArray prototypes;
		for (auto& proto : this->function_prototypes)
		{
			prototypes.addValue(proto.second->to_json());
		}
		root.addData("function_prototypes", prototypes);

		json::JsonArray functions;
		for (auto& func : this->functions)
		{
			functions.addValue(func->to_json());
		}
		root.addData("functions", functions);

		json::JsonArray expressions;
		for (auto& expr : this->expressions)
		{
			expressions.addValue(expr->to_json());
		}
		root.addData("expressions", expressions);

		return root;
	}

	types::Type BodyExpr::get_result_type()
	{
		if (result_type.get_type_enum() == types::TypeEnum::None)
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

	CallExpr::CallExpr(BodyExpr* body, int callee_id, std::vector<ptr_type<BaseExpr>>& args) :
		BaseExpr(AstExprType::CallExpr, body),
		callee_id(callee_id),
		unmangled_callee_id(callee_id),
		args(args.size())
	{
		for (int i = 0; i < this->args.size(); i++)
		{
			this->args[i] = std::move(args[i]);
			this->args[i]->set_parent_data(this, i);
		}
	}

	CallExpr::~CallExpr() {}

	std::string CallExpr::to_string(int depth) const
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Function Call: {" << constant_to_string(this) << '\n';
		str << tabs << '\t' << "Name: " << stringManager::get_string(this->callee_id) << '\n';
		str << tabs << '\t' << "Args: {" << '\n';
		for (auto& a : this->args)
		{
			str << a->to_string(depth + 2);
		}
		str << tabs << '\t' << "}," << '\n';
		str << tabs << "}," << '\n';

		return str.str();
	}

	json::JsonValue CallExpr::to_json() const
	{
		json::JsonObject root;

		root.addData(ExprTypeKeyString, "Call");
		root.addData("function_name", stringManager::get_string(this->callee_id));

		json::JsonArray arguments;
		for (auto& arg : this->args)
		{
			arguments.addValue(arg->to_json());
		}
		root.addData("function_arguements", arguments);

		return root;
	}

	types::Type CallExpr::get_result_type()
	{
		if (result_type.get_type_enum() == types::TypeEnum::None)
		{
			// result_type = scope::get_scope(ptr_type<CallExpr>(this))->function_prototypes[this->callee]->return_type;
			result_type = scope::get_scope(this)->function_prototypes[this->callee_id]->return_type;
			// result_type = this->get_body()->function_prototypes[this->callee]->return_type;
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
		// ptr_type<FunctionPrototype> proto =
		// scope::get_scope(ptr_type<CallExpr>(this))->function_prototypes[this->callee];
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

	IfExpr::IfExpr(
		BodyExpr* body,
		ptr_type<BaseExpr> condition,
		ptr_type<BaseExpr> if_body,
		ptr_type<BaseExpr> else_body,
		bool should_return_value) :
		BaseExpr(AstExprType::IfExpr, body),
		condition(std::move(condition)),
		if_body(std::move(if_body)),
		else_body(std::move(else_body)),
		should_return_value(should_return_value)
	{
		this->condition->set_parent_data(this, 0);
		this->if_body->set_parent_data(this, 1);
		if (this->else_body != nullptr)
		{
			this->else_body->set_parent_data(this, 2);
		}
	}

	std::string IfExpr::to_string(int depth) const
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "If Statement: {" << constant_to_string(this) << '\n';

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

	json::JsonValue IfExpr::to_json() const
	{
		json::JsonObject root;

		root.addData(ExprTypeKeyString, "If");
		root.addData("if_condition", this->condition->to_json());
		root.addData("if_body", this->if_body->to_json());

		if (this->else_body == nullptr)
		{
			root.addData("else_body", json::Null);
		}
		else
		{
			root.addData("else_body", this->else_body->to_json());
		}

		return root;
	}

	types::Type IfExpr::get_result_type()
	{
		if (result_type.get_type_enum() == types::TypeEnum::None)
		{
			if (this->should_return_value)
			{
				result_type = this->if_body->get_result_type();
			}
			else
			{
				result_type = types::Type{types::TypeEnum::Void};
			}
		}
		return result_type;
	}

	bool IfExpr::check_types()
	{
		if (this->condition->get_result_type().get_type_enum() == types::TypeEnum::Bool)
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

	ForExpr::ForExpr(
		BodyExpr* body,
		types::Type var_type,
		int name_id,
		ptr_type<BaseExpr> start_expr,
		ptr_type<BaseExpr> end_expr,
		ptr_type<BaseExpr> step_expr,
		ptr_type<BodyExpr> for_body) :
		BaseExpr(AstExprType::ForExpr, body),
		var_type(var_type),
		name_id(name_id),
		start_expr(std::move(start_expr)),
		end_expr(std::move(end_expr)),
		step_expr(std::move(step_expr)),
		for_body(std::move(for_body))
	{
		this->start_expr->set_parent_data(this, 0);
		this->end_expr->set_parent_data(this, 1);
		if (this->step_expr != nullptr)
		{
			this->step_expr->set_parent_data(this, 2);
		}
		this->for_body->set_parent_data(this, 3);
	}

	std::string ForExpr::to_string(int depth) const
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "For Loop: {" << constant_to_string(this) << '\n';

		str << tabs << '\t' << "Start Value: {" << '\n';
		str << tabs << '\t' << '\t' << "Type: " << this->var_type.to_string() << "," << '\n';
		str << tabs << '\t' << '\t' << "Name: " << stringManager::get_string(this->name_id) << "," << '\n';
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

	json::JsonValue ForExpr::to_json() const
	{
		json::JsonObject root{};

		root.addData(ExprTypeKeyString, "For");

		json::JsonObject start{};
		start.addData("type", this->var_type.to_string());
		start.addData("name", stringManager::get_string(this->name_id));
		start.addData("value", this->start_expr->to_json());
		root.addData("start", start);

		root.addData("end_condition", this->end_expr->to_json());
		if (this->step_expr == nullptr)
		{
			root.addData("step", json::Null);
		}
		else
		{
			root.addData("step", this->step_expr->to_json());
		}

		root.addData("loop_body", this->for_body->to_json());

		return root;
	}

	types::Type ForExpr::get_result_type()
	{
		if (result_type.get_type_enum() == types::TypeEnum::None)
		{
			result_type = types::Type{types::TypeEnum::Void};
		}
		return result_type;
	}

	bool ForExpr::check_types()
	{
		if (end_expr->get_result_type().get_type_enum() == types::TypeEnum::Bool)
		{
			return true;
		}
		return false;
	}

	WhileExpr::WhileExpr(BodyExpr* body, ptr_type<BaseExpr> end_expr, ptr_type<BaseExpr> while_body) :
		BaseExpr(AstExprType::WhileExpr, body),
		end_expr(std::move(end_expr)),
		while_body(std::move(while_body))
	{
		this->end_expr->set_parent_data(this, 0);
		this->while_body->set_parent_data(this, 1);
	}

	std::string WhileExpr::to_string(int depth) const
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "While Loop: {" << constant_to_string(this) << '\n';

		str << tabs << '\t' << "End Condition: {" << '\n';
		str << this->end_expr->to_string(depth + 2);
		str << tabs << '\t' << "}," << '\n';

		str << this->while_body->to_string(depth + 1);

		str << tabs << "}, " << '\n';

		return str.str();
	}

	json::JsonValue WhileExpr::to_json() const
	{
		json::JsonObject root{};

		root.addData(ExprTypeKeyString, "While");
		root.addData("end_condition", this->end_expr->to_json());
		root.addData("loop_body", this->while_body->to_json());

		return root;
	}

	types::Type WhileExpr::get_result_type()
	{
		if (result_type.get_type_enum() == types::TypeEnum::None)
		{
			result_type = types::Type{types::TypeEnum::Void};
		}
		return result_type;
	}

	bool WhileExpr::check_types()
	{
		if (end_expr->get_result_type().get_type_enum() == types::TypeEnum::Bool)
		{
			return true;
		}
		return false;
	}

	CommentExpr::CommentExpr(BodyExpr* body) : BaseExpr(AstExprType::CommentExpr, body) {}

	std::string CommentExpr::to_string(int depth) const
	{
		return std::string();
	}

	json::JsonValue CommentExpr::to_json() const
	{
		json::JsonObject root{};

		root.addData(ExprTypeKeyString, "Comment");

		return root;
	}

	types::Type CommentExpr::get_result_type()
	{
		return types::Type(types::TypeEnum::None);
	}

	bool CommentExpr::check_types()
	{
		return true;
	}

	ReturnExpr::ReturnExpr(BodyExpr* body, ptr_type<BaseExpr> ret_expr) :
		BaseExpr(AstExprType::ReturnExpr, body),
		ret_expr(std::move(ret_expr))
	{}

	std::string ReturnExpr::to_string(int depth) const
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Return Statement: {" << constant_to_string(this) << '\n';

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

	json::JsonValue ReturnExpr::to_json() const
	{
		json::JsonObject root{};

		root.addData(ExprTypeKeyString, "Return");

		if (this->ret_expr == nullptr)
		{
			root.addData("return_value", json::Null);
		}
		else
		{
			root.addData("return_value", this->ret_expr->to_json());
		}

		return root;
	}

	types::Type ReturnExpr::get_result_type()
	{
		if (result_type.get_type_enum() == types::TypeEnum::None)
		{
			if (ret_expr == nullptr)
			{
				result_type = types::Type{types::TypeEnum::Void};
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

	ContinueExpr::ContinueExpr(BodyExpr* body) : BaseExpr(AstExprType::ContinueExpr, body) {}

	std::string ContinueExpr::to_string(int depth) const
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Continue Statement: {}" << '\n';

		return str.str();
	}

	json::JsonValue ContinueExpr::to_json() const
	{
		json::JsonObject root{};

		root.addData(ExprTypeKeyString, "Continue");
		return root;
	}

	types::Type ContinueExpr::get_result_type()
	{
		if (result_type.get_type_enum() == types::TypeEnum::None)
		{
			result_type = types::Type{types::TypeEnum::Void};
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

	BreakExpr::BreakExpr(BodyExpr* body) : BaseExpr(AstExprType::BreakExpr, body) {}

	std::string BreakExpr::to_string(int depth) const
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Break Statement: {}" << constant_to_string(this) << '\n';

		return str.str();
	}

	json::JsonValue BreakExpr::to_json() const
	{
		json::JsonObject root{};

		root.addData(ExprTypeKeyString, "Break");

		return root;
	}

	types::Type BreakExpr::get_result_type()
	{
		if (result_type.get_type_enum() == types::TypeEnum::None)
		{
			result_type = types::Type{types::TypeEnum::Void};
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

	UnaryExpr::UnaryExpr(BodyExpr* body, operators::UnaryOp unop, ptr_type<BaseExpr> expr) :
		BaseExpr(ast::AstExprType::UnaryExpr, body),
		unop(unop),
		expr(std::move(expr))
	{
		this->expr->set_parent_data(this, 0);
	}

	std::string UnaryExpr::to_string(int depth) const
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Unary Operation: {" << constant_to_string(this) << '\n';
		str << tabs << '\t' << "Operator: " << operators::to_string(this->unop) << '\n';
		str << tabs << '\t' << "Expression: {" << '\n';
		str << this->expr->to_string(depth + 2);
		str << tabs << '\t' << "}" << '\n';
		str << tabs << "}," << '\n';

		return str.str();
	}

	json::JsonValue UnaryExpr::to_json() const
	{
		json::JsonObject root{};

		root.addData(ExprTypeKeyString, "Unary");
		root.addData("operator", operators::to_string(this->unop));
		root.addData("unary_value", this->expr->to_json());

		return root;
	}

	types::Type UnaryExpr::get_result_type()
	{
		if (result_type.get_type_enum() == types::TypeEnum::None)
		{
			result_type = this->expr->get_result_type();
		}
		return result_type;
	}

	bool UnaryExpr::check_types()
	{
		return true;
	}

	CastExpr::CastExpr(BodyExpr* body, int target_type_id, ptr_type<BaseExpr> expr) :
		BaseExpr(AstExprType::CastExpr, body),
		target_type_id(target_type_id),
		expr(std::move(expr))
	{}

	std::string CastExpr::to_string(int depth) const
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Cast Operation: {" << constant_to_string(this) << '\n';
		str << tabs << '\t' << "Target Type: " << this->target_type.to_string() << '\n';
		str << tabs << '\t' << "Expression: {" << '\n';
		str << this->expr->to_string(depth + 2);
		str << tabs << '\t' << "}" << '\n';
		str << tabs << "}," << '\n';

		return str.str();
	}

	json::JsonValue CastExpr::to_json() const
	{
		json::JsonObject root{};

		root.addData(ExprTypeKeyString, "Cast");
		root.addData("target_type", this->target_type.to_string());
		root.addData("cast_value", this->expr->to_json());

		return root;
	}

	types::Type CastExpr::get_result_type()
	{
		if (result_type.get_type_enum() == types::TypeEnum::None)
		{
			result_type = target_type;
		}
		return result_type;
	}

	bool CastExpr::check_types()
	{
		return true;
	}

	SwitchExpr::SwitchExpr(BodyExpr* body, ptr_type<BaseExpr> value_expr, std::vector<ptr_type<CaseExpr>>& cases) :
		BaseExpr(AstExprType::SwitchExpr, body),
		value_expr(std::move(value_expr))
	{
		for (auto& case_expr : cases)
		{
			case_expr->set_parent_data(this, this->cases.size());
			this->cases.push_back(std::move(case_expr));
		}
	}

	std::string SwitchExpr::to_string(int depth) const
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Switch Statement: {" << constant_to_string(this) << '\n';

		for (auto& case_expr : this->cases)
		{
			str << case_expr->to_string(depth + 1);
		}

		str << tabs << "}, " << '\n';

		return str.str();
	}

	json::JsonValue SwitchExpr::to_json() const
	{
		json::JsonObject root{};

		root.addData(ExprTypeKeyString, "Switch");

		json::JsonArray cases;
		for (auto& case_expr : this->cases)
		{
			cases.addValue(case_expr->to_json());
		}
		root.addData("cases", cases);

		return root;
	}

	types::Type SwitchExpr::get_result_type()
	{
		if (this->result_type.get_type_enum() == types::TypeEnum::None)
		{
			this->result_type = types::Type{types::TypeEnum::Void};
		}
		return this->result_type;
	}

	bool SwitchExpr::check_types()
	{
		if (this->value_expr->get_result_type().get_type_enum() == types::TypeEnum::Int)
		{
			return true;
		}
		return false;
	}

	CaseExpr::CaseExpr(BodyExpr* body, ptr_type<BaseExpr> case_expr, ptr_type<BaseExpr> case_body, bool default_case) :
		BaseExpr(AstExprType::CaseExpr, body),
		case_expr(std::move(case_expr)),
		case_body(std::move(case_body)),
		default_case(default_case)
	{}

	std::string CaseExpr::to_string(int depth) const
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Case Statement: {" << constant_to_string(this) << '\n';

		if (this->default_case)
		{
			str << tabs << '\t' << "Value: "
				<< "Default Case"
				<< "," << '\n';
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

	json::JsonValue CaseExpr::to_json() const
	{
		json::JsonObject root{};

		root.addData(ExprTypeKeyString, "Case");

		if (this->default_case)
		{
			root.addData("case_value", "<default_case>");
		}
		else
		{
			root.addData("case_value", this->case_expr->to_json());
		}

		root.addData("case_body", this->case_body->to_json());

		return root;
	}

	types::Type CaseExpr::get_result_type()
	{
		if (this->result_type.get_type_enum() == types::TypeEnum::None)
		{
			this->result_type = types::Type{types::TypeEnum::Void};
		}
		return this->result_type;
	}

	bool CaseExpr::check_types()
	{
		// TODO: maybe check for constant expressions
		if (this->case_expr->get_result_type().get_type_enum() == types::TypeEnum::Int)
		{
			return true;
		}
		return false;
	}

	FunctionPrototype::FunctionPrototype(
		const std::string& name,
		const types::Type& return_type,
		const std::vector<types::Type>& types,
		const std::vector<int>& args) :
		name_id(stringManager::get_id(name)),
		unmangled_name_id(name_id),
		return_type(return_type),
		types(types),
		args(args)
	{}

	std::string FunctionPrototype::to_string(int depth) const
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		str << tabs << "Function Prototype: {" << '\n';

		str << tabs << '\t' << "Name: " << stringManager::get_string(this->name_id) << '\n';
		str << tabs << '\t' << "Args: [" << '\n';

		for (int i = 0; i < this->args.size(); i++)
		{
			str << tabs << '\t' << '\t' << "{ " << stringManager::get_string(this->args[i]) << ": "
				<< this->types[i].to_string() << "}," << '\n';
		}

		str << tabs << '\t' << "]," << '\n';
		str << tabs << "}, " << '\n';

		return str.str();
	}

	json::JsonValue FunctionPrototype::to_json() const
	{
		json::JsonObject root{};

		root.addData("function_type", "Prototype");
		root.addData("function_name", stringManager::get_string(this->name_id));

		json::JsonArray arguments{};
		for (int i = 0; i < this->args.size(); i++)
		{
			json::JsonObject argument{};
			argument.addData("argument_type", this->types[i].to_string());
			argument.addData("arguemnt_name", stringManager::get_string(this->args[i]));
			arguments.addValue(argument);
		}
		root.addData("function_arguments", arguments);

		root.addData("return_type", this->return_type.to_string());
		root.addData("is_extern", this->is_extern);

		return root;
	}

	FunctionDefinition::FunctionDefinition(FunctionPrototype* prototype, ptr_type<BodyExpr> body) :
		prototype(prototype),
		body(std::move(body))
	{}

	FunctionDefinition::~FunctionDefinition() {}

	std::string FunctionDefinition::to_string(int depth) const
	{
		std::string tabs(depth, '\t');

		std::stringstream str;

		for (auto& f : this->body->functions)
		{
			str << f->to_string(depth + 1);
			str << '\n';
		}

		str << this->prototype->to_string(depth);
		str << '\n';
		str << tabs << "Function Body : {" << '\n';
		str << tabs << '\t' << "Name: " << stringManager::get_string(this->prototype->name_id) << '\n';
		str << this->body->to_string(depth + 1);
		str << tabs << '}' << '\n';

		return str.str();
	}

	json::JsonValue FunctionDefinition::to_json() const
	{
		json::JsonObject root{};

		root.addData("function_type", "Definition");
		root.addData("function_prototype", this->prototype->to_json());
		root.addData("function_name", stringManager::get_string(this->prototype->name_id));
		root.addData("function_body", this->body->to_json());

		return root;
	}

	bool FunctionDefinition::check_return_type() const
	{
		if (this->prototype->return_type.get_type_enum() == types::TypeEnum::Void)
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
