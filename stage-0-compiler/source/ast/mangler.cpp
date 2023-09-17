#include "mangler.h"
#include "module.h"

namespace mangler
{
	int ManglerV1::mangle(int module_id, ast::FunctionPrototype* proto)
	{
		std::string name;

		// add module name
		name += module::StringManager::get_string(module_id);

		name += "__";

		name += mangle_function(proto->name_id, proto->types);

		int id = module::StringManager::get_id(name);

		return id;
	}

	int ManglerV1::mangle(int module_id, ast::CallExpr* expr)
	{
		std::string name;

		// add module name
		name += module::StringManager::get_string(module_id);

		name += "__";

		name += mangle_call(expr);

		int id = module::StringManager::get_id(name);

		return id;
	}

	int ManglerV1::mangle(ast::CallExpr* expr)
	{
		std::string name = mangle_call(expr);

		int id = module::StringManager::get_id(name);

		return id;
	}

	int ManglerV1::mangle(int module_id, int function_id, std::vector<types::Type> function_args)
	{
		std::string name;

		// add module name
		name += module::StringManager::get_string(module_id);

		name += "__";

		name += mangle_function(function_id, function_args);

		int id = module::StringManager::get_id(name);

		return id;
	}

	int ManglerV1::add_module(int module_id, int other_module_id)
	{
		std::string name;

		if (module_id != -1)
		{
			name += module::StringManager::get_string(module_id);
			name += '_';
		}

		name += module::StringManager::get_string(other_module_id);

		return module::StringManager::get_id(name);
	}

	int ManglerV1::add_mangled_name(int module_id, int mangled_name)
	{
		std::string name;

		if (module_id != -1)
		{
			name += module::StringManager::get_string(module_id);
			name += "__";
		}

		name += module::StringManager::get_string(mangled_name);

		return module::StringManager::get_id(name);
	}

	int ManglerV1::mangle_using(ast::BinaryExpr* scope_expr)
	{
		if (scope_expr->binop != operators::BinaryOp::ModuleScope)
		{
			assert(false && "Mangler::mangle_using, Scope Expression has invalid binop.");
		}

		std::string modules;

		// add lhs
		if (scope_expr->lhs->get_type() == ast::AstExprType::BinaryExpr)
		{
			modules += module::StringManager::get_string(mangle_using(dynamic_cast<ast::BinaryExpr*>(scope_expr->lhs.get())));
		}
		else
		{
			modules += module::StringManager::get_string(dynamic_cast<ast::VariableReferenceExpr*>(scope_expr->lhs.get())->name_id);
		}

		modules += '_';

		// add rhs
		modules += module::StringManager::get_string(dynamic_cast<ast::VariableReferenceExpr*>(scope_expr->rhs.get())->name_id);

		return module::StringManager::get_id(modules);
	}

	int ManglerV1::extract_module(int function_id)
	{
		std::string module_name;

		std::string& name = module::StringManager::get_string(function_id);

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

		return module::StringManager::get_id(module_name);
	}

	std::string ManglerV1::mangle_function(int function_id, std::vector<types::Type>& types)
	{
		std::string name;

		// add function name
		name += module::StringManager::get_string(function_id);

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

	std::string ManglerV1::mangle_call(ast::CallExpr* expr)
	{
		std::vector<types::Type> types;
		for (auto& e : expr->args)
		{
			types.push_back(e->get_result_type());
		}

		return mangle_function(expr->callee_id, types);
	}

	int ManglerV2::mangle(int current_module_id, ast::FunctionPrototype* proto)
	{
		std::string name = ManglerV2::get_name_or_start(current_module_id);

		name += mangle_function(proto->name_id, proto->types);

		return module::StringManager::get_id(name);
	}

	int ManglerV2::mangle(int current_module_id, ast::CallExpr* expr)
	{
		std::string name = ManglerV2::get_name_or_start(current_module_id);

		name += mangle_call(expr);

		return module::StringManager::get_id(name);
	}

	int ManglerV2::mangle(ast::CallExpr* expr)
	{
		std::string name = ManglerV2::mangle_call(expr);
		return module::StringManager::get_id(name);
	}

	int ManglerV2::mangle(int current_module_id, int function_id, std::vector<types::Type> function_args)
	{
		std::string name = ManglerV2::get_name_or_start(current_module_id);

		name += mangle_function(function_id, function_args);

		return module::StringManager::get_id(name);
	}

	int ManglerV2::add_module(int current_module_id, int other_module_id)
	{
		std::string name = ManglerV2::get_name_or_start(current_module_id);

		std::string module_name = module::StringManager::get_string(other_module_id);

		// module = M<char length><name>
		name += 'M';
		name += std::to_string(module_name.size());
		name += module_name;

		return module::StringManager::get_id(name);
	}

	int ManglerV2::add_mangled_name(int current_module_id, int mangled_name_id)
	{
		std::string name = ManglerV2::get_name_or_start(current_module_id);
		name += module::StringManager::get_string(mangled_name_id);
		return module::StringManager::get_id(name);
	}

	int ManglerV2::mangle_using(ast::BinaryExpr* scope_expr)
	{
		if (scope_expr->binop != operators::BinaryOp::ModuleScope)
		{
			assert(false && "ManglerV2::mangle_using, Scope Expression has invalid binop.");
		}

		std::string modules;

		// add lhs
		if (scope_expr->lhs->get_type() == ast::AstExprType::BinaryExpr)
		{
			modules += module::StringManager::get_string(mangle_using(dynamic_cast<ast::BinaryExpr*>(scope_expr->lhs.get())));
		}
		else
		{
			std::string name = module::StringManager::get_string(dynamic_cast<ast::VariableReferenceExpr*>(scope_expr->lhs.get())->name_id);
			modules += 'M';
			modules += std::to_string(name.size());
			modules += name;
		}

		// add rhs
		std::string name = module::StringManager::get_string(dynamic_cast<ast::VariableReferenceExpr*>(scope_expr->rhs.get())->name_id);
		modules += 'M';
		modules += std::to_string(name.size());
		modules += name;

		return module::StringManager::get_id(modules);
	}

	int ManglerV2::extract_module(int function_id)
	{
		std::string mangled_name = module::StringManager::get_string(function_id);

		size_t i = ManglerV2::StartStringSize;

		std::string module_string = ManglerV2::StartString;

		// module = M<char length><name>

		while (i < mangled_name.size())
		{
			if (mangled_name[i] == 'M')
			{
				size_t module_start = i;
				i++;
				std::string count_str;
				count_str = mangled_name[i];
				i++;

				while (mangled_name[i] >= '0' && mangled_name[i] <= '9')
				{
					count_str += mangled_name[i];
					i++;

					if (i >= mangled_name.size())
					{
						assert("ManglerV2::extract_module out of range (getting count )" && false);
					}
				}

				int count = std::stoi(count_str);

				if (i + count > mangled_name.size())
				{
					assert("ManglerV2::extract_module out of range (getting chars)" && false);
				}

				i += count;
				module_string += mangled_name.substr(module_start, i - module_start);
			}
			else
			{
				break;
			}

		}

		return module::StringManager::get_id(module_string);
	}

	std::string ManglerV2::pretty_modules(int module_id)
	{
		if (module_id == -1)
		{
			return "";
		}

		std::string module_string = ManglerV2::remove_start_string(module::StringManager::get_string(module_id));

		std::string pretty_string;

		size_t i = 0;

		while (i < module_string.size())
		{
			if (module_string[i] != 'M')
			{
				break;
			}

			i++;

			std::string count_str;
			count_str = module_string[i];
			i++;

			while (module_string[i] >= '0' && module_string[i] <= '9')
			{
				count_str += module_string[i];
				i++;

				if (i >= module_string.size())
				{
					assert("ManglerV2::pretty_modules out of range (getting count )" && false);
				}
			}

			int count = std::stoi(count_str);

			if (i + count > module_string.size())
			{
				assert("ManglerV2::pretty_modules out of range (getting chars)" && false);
			}

			pretty_string += module_string.substr(i, count);

			i += count;

			if (i < module_string.size() && module_string[i] == 'M')
			{
				pretty_string += "::";
			}

		}

		return pretty_string;
	}

	std::string ManglerV2::remove_start_string(const std::string& string)
	{
		std::string start{ ManglerV2::StartString };

		if (string.rfind(start, 0) == std::string::npos)
		{
			assert("String does not begin with start text" && false);
		}

		std::string removed_string{ string };
		removed_string.erase(0, start.size());
		return removed_string;
	}

	std::string ManglerV2::get_name_or_start(int name_id)
	{
		if (name_id == -1)
		{
			return ManglerV2::StartString;
		}
		else
		{
			return module::StringManager::get_string(name_id);
		}
	}

	std::string ManglerV2::mangle_function(int function_id, std::vector<types::Type>& types)
	{
		// function = F<char length><name>P<param count><types>*

		std::string name = "F";

		std::string function_name = module::StringManager::get_string(function_id);

		// add function name
		name += std::to_string(function_name.size());
		name += function_name;

		// add param count
		name += 'P';
		name += std::to_string(types.size());

		// add function args
		for (int i = 0; i < types.size(); i++)
		{
			name += ManglerV2::mangle_type(types[i]);
		}

		return name;
	}

	std::string ManglerV2::mangle_type(types::Type& type)
	{
		// type (primitive) = V<char length><name>
		//	or
		// type (custom) = V<type name ref>

		std::string name = "V";

		// TODO: handle the type to string better & handle the second option
		std::string type_name = type.to_string();

		name += std::to_string(type_name.size());
		name += type_name;

		return name;
	}

	std::string ManglerV2::mangle_call(ast::CallExpr* expr)
	{
		std::vector<types::Type> types;
		for (auto& e : expr->args)
		{
			types.push_back(e->get_result_type());
		}

		return mangle_function(expr->callee_id, types);
	}
}
