#include <iostream>

#include "llvm/IR/Verifier.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/DiagnosticInfo.h"

#include "builder.h"
#include "scope_checker.h"

#ifdef _WIN64
#define __print_function__ __FUNCSIG__
#endif
#ifdef __linux__
#define __print_function__ __PRETTY_FUNCTION__
#endif

#define null_end std::cout << "value is null\n" << __print_function__ << std::endl;
#define here_ std::cout << "here" << std::endl;

namespace builder
{
	LLVMBuilder::LLVMBuilder()
	{
		llvm_context = new llvm::LLVMContext();
		llvm_module = new llvm::Module("ash-boot", *llvm_context);
		llvm_ir_builder = new llvm::IRBuilder<>(*llvm_context);

		llvm_context->setDiagnosticHandlerCallBack(&this->diagnostic_handler_callback);
	}

	LLVMBuilder::~LLVMBuilder()
	{
		delete llvm_ir_builder;
		delete llvm_module;
		delete llvm_context;
	}

	llvm::Value* LLVMBuilder::log_error_value(std::string str)
	{
		std::cout << str << std::endl;
		return nullptr;
	}

	llvm::AllocaInst* LLVMBuilder::create_entry_block_alloca(llvm::Function* the_function, llvm::Type* type, llvm::StringRef name)
	{
		llvm::IRBuilder<> tmp(&the_function->getEntryBlock(), the_function->getEntryBlock().begin());
		return tmp.CreateAlloca(type, nullptr, name);
	}

	llvm::Function* LLVMBuilder::generate_function_prototype(ast::FunctionPrototype* prototype)
	{
		std::vector<llvm::Type*> types;
		for (auto& type : prototype->types)
		{
			types.push_back(types::get_llvm_type(*llvm_context, type));
		}

		// create the function type
		llvm::FunctionType* ft = llvm::FunctionType::get(types::get_llvm_type(*llvm_context, prototype->return_type), types, false);

		llvm::Function* f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, prototype->name, llvm_module);

		// set names for all arguments

		int index = 0;
		for (auto& arg : f->args())
		{
			arg.setName(prototype->args[index]);
			index++;
		}

		return f;
	}

	llvm::Function* LLVMBuilder::generate_function_definition(ast::FunctionDefinition* function_definition)
	{
		// create all of the functions defined inside the current function
		for (auto& func : function_definition->body->functions)
		{
			llvm::Function* f = generate_function_definition(func);
			if (f == nullptr)
			{
				null_end;
				return nullptr;
			}
		}

		// check for existing function
		llvm::Function* the_function = llvm_module->getFunction(function_definition->prototype->name);

		if (the_function == nullptr)
		{
			the_function = generate_function_prototype(function_definition->prototype);
		}

		if (the_function == nullptr)
		{
			null_end;
			return nullptr;
		}

		// create a basic block to insert the code into
		llvm::BasicBlock* bb = llvm::BasicBlock::Create(*llvm_context, "entry", the_function);
		llvm_ir_builder->SetInsertPoint(bb);

		// record the functions types and arguments
		//llvm_named_types.clear();
		//llvm_named_values.clear();

		for (auto& arg : the_function->args())
		{
			llvm::AllocaInst* alloca = create_entry_block_alloca(the_function, arg.getType(), arg.getName());

			llvm_ir_builder->CreateStore(&arg, alloca);

			//llvm_named_values[std::string{ arg.getName() }] = alloca;
			function_definition->body->llvm_named_values[std::string{ arg.getName() }] = alloca;
			function_definition->body->llvm_named_types[std::string{ arg.getName() }] = arg.getType();
		}

		llvm::Value* return_value = generate_code_dispatch(function_definition->body);

		if (return_value != nullptr)
		{
			// finish off the function
			// check for void return type
			if (function_definition->prototype->return_type == types::Type::Void)
			{
				llvm_ir_builder->CreateRetVoid();
			}
			else
			{
				llvm_ir_builder->CreateRet(return_value);
			}
			// validate the generated code, checking for consistency
			llvm::verifyFunction(*the_function);

			// Todo: run the optimiser on the function

			return the_function;
		}

		// error reading body, remove function
		the_function->eraseFromParent();
		null_end;
		here_;
		return nullptr;
	}

	template<>
	llvm::Value* LLVMBuilder::generate_code<ast::LiteralExpr>(ast::LiteralExpr* expr)
	{
		return expr->value_type->get_value(llvm_context);
	}

	template<>
	llvm::Value* LLVMBuilder::generate_code<ast::BodyExpr>(ast::BodyExpr* expr)
	{
		llvm::Value* value = nullptr;

		if (expr->expressions.size() == 0)
		{
			return llvm::Constant::getNullValue(types::get_llvm_type(*llvm_context, types::Type::Int));
		}

		for (auto& e : expr->expressions)
		{
			value = generate_code_dispatch(e);
			if (value == nullptr)
			{
				null_end;
				return nullptr;
			}
		}

		return value;
	}

	template<>
	llvm::Value* LLVMBuilder::generate_code<ast::VariableDeclarationExpr>(ast::VariableDeclarationExpr* expr)
	{
		llvm::AllocaInst* old_binding;

		llvm::Function* the_function = llvm_ir_builder->GetInsertBlock()->getParent();

		// register the variable and emit its initialiser
		const std::string& var_name = expr->name;
		ast::BaseExpr* init = expr->expr;

		// Emit the initializer before adding the variable to scope, this prevents
		// the initializer from referencing the variable itself, and permits stuff
		// like this:
		//  var a = 1 in
		//    var a = a in ...   # refers to outer 'a'.

		llvm::Value* init_value;

		if (init != nullptr)
		{
			init_value = generate_code_dispatch(init);
			if (init_value == nullptr)
			{
				null_end;

				return nullptr;
			}
		}
		else
		{
			// use defualt value
			init_value = types::get_default_value(*llvm_context, expr->curr_type);
		}

		llvm::AllocaInst* alloca = create_entry_block_alloca(the_function, types::get_llvm_type(*llvm_context, expr->curr_type), expr->name);
		llvm_ir_builder->CreateStore(init_value, alloca);

		// remember the old varibale binding so we can restore when the function ends
		//old_binding = llvm_named_values[var_name];
		// remember the new binding
		//llvm_named_values[var_name] = alloca;
		//llvm_named_types[var_name] = types::get_llvm_type(*llvm_context, expr->curr_type);
		expr->get_body()->llvm_named_values[var_name] = alloca;
		expr->get_body()->llvm_named_types[var_name] = types::get_llvm_type(*llvm_context, expr->curr_type);

		// create the body code?????????
		// TODO: SORT
		//llvm::Value* 

		// put the variable back into scope
		//llvm_named_values[expr->name] = old_binding;

		return init_value;
		//return llvm::Constant::getNullValue(llvm::Type::getVoidTy(*llvm_context));
		//return llvm::UndefValue::get(llvm::Type::getVoidTy(*llvm_context));
		//llvm::Value::
	}

	template<>
	llvm::Value* LLVMBuilder::generate_code<ast::VariableReferenceExpr>(ast::VariableReferenceExpr* expr)
	{
		// lookup the variable in the function
		//auto f = llvm_named_values.find(expr->name);
		//auto f = expr->get_body()->llvm_named_values.find(expr->name);
		auto b = scope::get_scope(expr);
		//if (f == expr->get_body()->llvm_named_values.end())
		if (b == nullptr)
		{
			return log_error_value("unknown variable name: " + expr->name);
		}

		auto f = b->llvm_named_values.find(expr->name);

		// load the value
		//return llvm_ir_builder->CreateLoad(llvm_named_types[f->first], f->second, f->first.c_str());
		//return llvm_ir_builder->CreateLoad(expr->get_body()->llvm_named_types[f->first], f->second, f->first.c_str());
		return llvm_ir_builder->CreateLoad(b->llvm_named_types[f->first], f->second, f->first.c_str());
	}

	template<>
	llvm::Value* LLVMBuilder::generate_code<ast::BinaryExpr>(ast::BinaryExpr* expr)
	{
		// lhs is not an expression for assignment
		if (expr->binop == ast::BinaryOp::Assignment)
		{
			// lhs must be a variable definition or declaration
			ast::VariableReferenceExpr* lhs_expr = dynamic_cast<ast::VariableReferenceExpr*>(expr->lhs);
			if (lhs_expr == nullptr)
			{
				return log_error_value("destination of '=' must be an identifier");
			}

			// create the rhs code
			llvm::Value* rhs = generate_code_dispatch(expr->rhs);
			if (rhs == nullptr)
			{
				null_end;
				return nullptr;
			}

			// look up the name
			//auto f = llvm_named_values.find(lhs_expr->name);
			auto f = lhs_expr->get_body()->llvm_named_values.find(lhs_expr->name);
			if (f == lhs_expr->get_body()->llvm_named_values.end())
			{
				return log_error_value("unknown variable name");
			}

			llvm_ir_builder->CreateStore(rhs, f->second);
			return rhs;
		}

		llvm::Value* lhs = generate_code_dispatch(expr->lhs);
		llvm::Value* rhs = generate_code_dispatch(expr->rhs);

		if (lhs == nullptr || rhs == nullptr)
		{
			null_end;
			return nullptr;
		}

		switch (expr->binop)
		{
			case ast::BinaryOp::Assignment:
			{
				null_end;
				return nullptr;
			}
			case ast::BinaryOp::Addition:
			{
				return llvm_ir_builder->CreateAdd(lhs, rhs, "add");
			}
			case ast::BinaryOp::Subtraction:
			{
				return llvm_ir_builder->CreateSub(lhs, rhs, "sub");
			}
			case ast::BinaryOp::Multiplication:
			{
				return llvm_ir_builder->CreateMul(lhs, rhs, "mul");
			}
			default:
			{
				return log_error_value("invalid binary operator");
			}
		}
	}

	template<>
	llvm::Value* LLVMBuilder::generate_code<ast::CallExpr>(ast::CallExpr* expr)
	{
		llvm::Function* callee_func = llvm_module->getFunction(expr->callee);

		if (callee_func == nullptr)
		{
			return log_error_value("unknown function referenced");
		}

		if (callee_func->arg_size() != expr->args.size())
		{
			return log_error_value("incorrect number of arguemnts passed");
		}

		std::vector<llvm::Value*> args;
		for (int i = 0; i < expr->args.size(); i++)
		{
			args.push_back(generate_code_dispatch(expr->args[i]));
			if (args.back() == nullptr)
			{
				null_end;
				return nullptr;
			}
		}

		return llvm_ir_builder->CreateCall(callee_func, args, "call");
	}

	llvm::Value* LLVMBuilder::generate_code_dispatch(ast::BaseExpr* expr)
	{
		switch (expr->get_type())
		{
			case ast::AstExprType::LiteralExpr:
			{
				return generate_code(dynamic_cast<ast::LiteralExpr*>(expr));
			}
			case ast::AstExprType::BodyExpr:
			{
				return generate_code(dynamic_cast<ast::BodyExpr*>(expr));
			}
			case ast::AstExprType::VariableDeclarationExpr:
			{
				return generate_code(dynamic_cast<ast::VariableDeclarationExpr*>(expr));
			}
			case ast::AstExprType::VariableReferenceExpr:
			{
				return generate_code(dynamic_cast<ast::VariableReferenceExpr*>(expr));
			}
			case ast::AstExprType::BinaryExpr:
			{
				return generate_code(dynamic_cast<ast::BinaryExpr*>(expr));
			}
			case ast::AstExprType::CallExpr:
			{
				return generate_code(dynamic_cast<ast::CallExpr*>(expr));
			}
		}
		null_end;
		return nullptr;
	}

	void LLVMBuilder::diagnostic_handler_callback(const llvm::DiagnosticInfo& di, void* context)
	{
		llvm::DiagnosticPrinterRawOStream printer(llvm::outs());

		di.print(printer);
	}

}
