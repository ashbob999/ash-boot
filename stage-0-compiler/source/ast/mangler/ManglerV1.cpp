#include "ManglerV1.h"

#include "../string_manager.h"

#include <string>

namespace manglerV1
{
	std::string mangle_function(int function_id, const std::vector<types::Type>& types);
	std::string mangle_call(const ast::CallExpr* expr);
}

int manglerV1::mangle(int module_id, const ast::FunctionPrototype* proto)
{
	std::string name;

	// add module name
	name += stringManager::get_string(module_id);

	name += "__";

	name += mangle_function(proto->name_id, proto->types);

	int id = stringManager::get_id(name);

	return id;
}

int manglerV1::mangle(int module_id, const ast::CallExpr* expr)
{
	std::string name;

	// add module name
	name += stringManager::get_string(module_id);

	name += "__";

	name += mangle_call(expr);

	int id = stringManager::get_id(name);

	return id;
}

int manglerV1::mangle(const ast::CallExpr* expr)
{
	std::string name = mangle_call(expr);

	int id = stringManager::get_id(name);

	return id;
}

int manglerV1::mangle(int module_id, int function_id, const std::vector<types::Type>& function_args)
{
	std::string name;

	// add module name
	name += stringManager::get_string(module_id);

	name += "__";

	name += mangle_function(function_id, function_args);

	int id = stringManager::get_id(name);

	return id;
}

int manglerV1::add_module(int module_id, int other_module_id)
{
	std::string name;

	if (module_id != -1)
	{
		name += stringManager::get_string(module_id);
		name += '_';
	}

	name += stringManager::get_string(other_module_id);

	return stringManager::get_id(name);
}

int manglerV1::add_mangled_name(int module_id, int mangled_name)
{
	std::string name;

	if (module_id != -1)
	{
		name += stringManager::get_string(module_id);
		name += "__";
	}

	name += stringManager::get_string(mangled_name);

	return stringManager::get_id(name);
}

int manglerV1::mangle_using(const ast::BinaryExpr* scope_expr)
{
	if (scope_expr->binop != operators::BinaryOp::ModuleScope)
	{
		assert(false && "Mangler::mangle_using, Scope Expression has invalid binop.");
	}

	std::string modules;

	// add lhs
	if (scope_expr->lhs->get_type() == ast::AstExprType::BinaryExpr)
	{
		modules += stringManager::get_string(mangle_using(dynamic_cast<ast::BinaryExpr*>(scope_expr->lhs.get())));
	}
	else
	{
		modules += stringManager::get_string(dynamic_cast<ast::VariableReferenceExpr*>(scope_expr->lhs.get())->name_id);
	}

	modules += '_';

	// add rhs
	modules += stringManager::get_string(dynamic_cast<ast::VariableReferenceExpr*>(scope_expr->rhs.get())->name_id);

	return stringManager::get_id(modules);
}

int manglerV1::extract_module(int function_id)
{
	std::string module_name;

	const std::string& name = stringManager::get_string(function_id);

	for (int i = 0; i < name.length(); i++)
	{
		if (name[i] == '_' && i < name.length() - 1 && name[i + 1] == '_')
		{
			break;
		}
		else
		{
			module_name += name[i];
		}
	}

	return stringManager::get_id(module_name);
}

std::string manglerV1::mangle_function(int function_id, const std::vector<types::Type>& types)
{
	std::string name;

	// add function name
	name += stringManager::get_string(function_id);

	name += "__";

	// add function args
	if (types.size() > 0)
	{
		for (int i = 0; i < types.size() - 1; i++)
		{
			name += types[i].to_string();
			name += '_';
		}

		name += types.back().to_string();
	}

	return name;
}

std::string manglerV1::mangle_call(const ast::CallExpr* expr)
{
	std::vector<types::Type> types;
	for (auto& e : expr->args)
	{
		types.push_back(e->get_result_type());
	}

	return mangle_function(expr->callee_id, types);
}
