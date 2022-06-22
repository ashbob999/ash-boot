#include <iostream>

#include "llvm/IR/Verifier.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/DiagnosticInfo.h"

#include "builder.h"
#include "scope_checker.h"
#include "module.h"

using std::dynamic_pointer_cast;

#define __debug

#ifdef __debug
#ifdef _WIN64
#define __print_function__ __FUNCSIG__
#endif
#ifdef __linux__
#define __print_function__ __PRETTY_FUNCTION__
#endif

#define null_end std::cout << "value is null\n" << __print_function__ << std::endl;
#define here_ std::cout << "here" << std::endl;
#else
#define null_end
#define here_
#endif

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

		llvm::Function* f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, module::StringManager::get_string(prototype->name_id), llvm_module);

		// set names for all arguments

		int index = 0;
		for (auto& arg : f->args())
		{
			arg.setName(module::StringManager::get_string(prototype->args[index]));
			index++;
		}

		return f;
	}

	llvm::Function* LLVMBuilder::generate_function_definition(ast::FunctionDefinition* function_definition)
	{
		// create all of the function prototypes inside the current function
		for (auto& proto : function_definition->body->function_prototypes)
		{
			llvm::Function* p = generate_function_prototype(proto.second);
			if (p == nullptr)
			{
				null_end;
				return nullptr;
			}
		}

		// create all of the functions defined inside the current function
		for (auto& func : function_definition->body->functions)
		{
			llvm::Function* f = generate_function_definition(func.get());
			if (f == nullptr)
			{
				null_end;
				return nullptr;
			}
		}

		// check for existing function
		llvm::Function* the_function = llvm_module->getFunction(module::StringManager::get_string(function_definition->prototype->name_id));

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
			std::string name{ arg.getName() };
			function_definition->body->llvm_named_values[module::StringManager::get_id(name)] = alloca;
			function_definition->body->llvm_named_types[module::StringManager::get_id(name)] = arg.getType();
		}

		llvm::Value* return_value = generate_code_dispatch(function_definition->body.get());

		if (return_value != nullptr)
		{
			// finish off the function

			// add return instruction if the last body expression was not a retrun expression
			if (function_definition->body->expressions.size() > 0 && function_definition->body->expressions.back()->get_type() == ast::AstExprType::ReturnExpr)
			{
				// do nothing
			}
			else
			{
				// check for void return type
				if (function_definition->prototype->return_type == types::Type::Void)
				{
					llvm_ir_builder->CreateRetVoid();
				}
				else
				{
					llvm_ir_builder->CreateRet(return_value);
				}
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
			//return llvm::Constant::getNullValue(types::get_llvm_type(*llvm_context, types::Type::Int));
			return llvm::ConstantTokenNone::get(*llvm_context);
		}

		for (auto& e : expr->expressions)
		{
			value = generate_code_dispatch(e.get());
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
		const int var_id = expr->name_id;
		ast::BaseExpr* init = expr->expr.get();

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

		llvm::AllocaInst* alloca = create_entry_block_alloca(the_function, types::get_llvm_type(*llvm_context, expr->curr_type), module::StringManager::get_string(expr->name_id));
		llvm_ir_builder->CreateStore(init_value, alloca);

		// remember the old varibale binding so we can restore when the function ends
		//old_binding = llvm_named_values[var_name];
		// remember the new binding
		//llvm_named_values[var_name] = alloca;
		//llvm_named_types[var_name] = types::get_llvm_type(*llvm_context, expr->curr_type);
		expr->get_body()->llvm_named_values[var_id] = alloca;
		expr->get_body()->llvm_named_types[var_id] = types::get_llvm_type(*llvm_context, expr->curr_type);

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
			return log_error_value("unknown variable name: " + module::StringManager::get_string(expr->name_id));
		}

		auto f = b->llvm_named_values.find(expr->name_id);

		// load the value
		//return llvm_ir_builder->CreateLoad(llvm_named_types[f->first], f->second, f->first.c_str());
		//return llvm_ir_builder->CreateLoad(expr->get_body()->llvm_named_types[f->first], f->second, f->first.c_str());
		//std::string name = ;
		return llvm_ir_builder->CreateLoad(b->get_llvm_type(*llvm_context, f->first), f->second, module::StringManager::get_string(f->first));
	}

	template<>
	llvm::Value* LLVMBuilder::generate_code<ast::BinaryExpr>(ast::BinaryExpr* expr)
	{
		// lhs is not an expression for assignment
		if (expr->binop == operators::BinaryOp::Assignment)
		{
			// lhs must be a variable definition or declaration
			ast::VariableReferenceExpr* lhs_expr = dynamic_cast<ast::VariableReferenceExpr*>(expr->lhs.get());
			if (lhs_expr == nullptr)
			{
				return log_error_value("destination of '=' must be an identifier");
			}

			// create the rhs code
			llvm::Value* rhs = generate_code_dispatch(expr->rhs.get());
			if (rhs == nullptr)
			{
				null_end;
				return nullptr;
			}

			// look up the name
			//auto f = llvm_named_values.find(lhs_expr->name);
			//auto f = lhs_expr->get_body()->llvm_named_values.find(lhs_expr->name_id);
			ast::BodyExpr* scope = scope::get_scope(lhs_expr);
			auto f = scope->llvm_named_values.find(lhs_expr->name_id);
			if (f == scope->llvm_named_values.end())
			{
				return log_error_value("unknown variable name: " + module::StringManager::get_string(lhs_expr->name_id));
			}

			llvm_ir_builder->CreateStore(rhs, f->second);
			return rhs;
		}

		llvm::Value* lhs;
		llvm::Value* rhs;

		// don't pre generate the code for the lhs & rhs if the binop is a boolean operator
		if (!operators::is_boolean_operator(expr->binop))
		{
			lhs = generate_code_dispatch(expr->lhs.get());
			rhs = generate_code_dispatch(expr->rhs.get());

			if (lhs == nullptr || rhs == nullptr)
			{
				null_end;
				return nullptr;
			}
		}

		switch (expr->binop)
		{
			case operators::BinaryOp::Assignment:
			{
				null_end;
				return nullptr;
			}
			case operators::BinaryOp::Addition:
			{
				return llvm_ir_builder->CreateAdd(lhs, rhs, "add");
			}
			case operators::BinaryOp::Subtraction:
			{
				return llvm_ir_builder->CreateSub(lhs, rhs, "sub");
			}
			case operators::BinaryOp::Multiplication:
			{
				return llvm_ir_builder->CreateMul(lhs, rhs, "mul");
			}
			case operators::BinaryOp::Division:
			{
				// TODO: deal with divide by zero errors
				switch (expr->get_result_type())
				{
					case types::Type::Int:
					{
						return llvm_ir_builder->CreateSDiv(lhs, rhs, "sdiv");
					}
					case types::Type::Float:
					{
						return llvm_ir_builder->CreateFDiv(lhs, rhs, "fdiv");
					}
					default:
					{
						return log_error_value("Unsupported type: " + types::to_string(expr->get_result_type()) + ", for operator: " + operators::to_string(expr->binop));
					}
				}
			}
			case operators::BinaryOp::LessThan:
			{
				switch (expr->lhs->get_result_type())
				{
					case types::Type::Int:
					case types::Type::Char:
					{
						return llvm_ir_builder->CreateICmpSLT(lhs, rhs, "lt");
					}
					case types::Type::Float:
					{
						return llvm_ir_builder->CreateFCmpOLT(lhs, rhs, "lt");
					}
					default:
					{
						return log_error_value("Unsupported type: " + types::to_string(expr->get_result_type()) + ", for operator: " + operators::to_string(expr->binop));
					}
				}
			}
			case operators::BinaryOp::LessThanEqual:
			{
				switch (expr->lhs->get_result_type())
				{
					case types::Type::Int:
					case types::Type::Char:
					{
						return llvm_ir_builder->CreateICmpSLE(lhs, rhs, "lte");
					}
					case types::Type::Float:
					{
						return llvm_ir_builder->CreateFCmpOLE(lhs, rhs, "lte");
					}
					default:
					{
						return log_error_value("Unsupported type: " + types::to_string(expr->get_result_type()) + ", for operator: " + operators::to_string(expr->binop));
					}
				}
			}
			case operators::BinaryOp::GreaterThan:
			{
				switch (expr->lhs->get_result_type())
				{
					case types::Type::Int:
					case types::Type::Char:
					{
						return llvm_ir_builder->CreateICmpSGT(lhs, rhs, "gt");
					}
					case types::Type::Float:
					{
						return llvm_ir_builder->CreateFCmpOGT(lhs, rhs, "gt");
					}
					default:
					{
						return log_error_value("Unsupported type: " + types::to_string(expr->get_result_type()) + ", for operator: " + operators::to_string(expr->binop));
					}
				}
			}
			case operators::BinaryOp::GreaterThanEqual:
			{
				switch (expr->lhs->get_result_type())
				{
					case types::Type::Int:
					case types::Type::Char:
					{
						return llvm_ir_builder->CreateICmpSGE(lhs, rhs, "gte");
					}
					case types::Type::Float:
					{
						return llvm_ir_builder->CreateFCmpOGE(lhs, rhs, "gte");
					}
					default:
					{
						return log_error_value("Unsupported type: " + types::to_string(expr->get_result_type()) + ", for operator: " + operators::to_string(expr->binop));
					}
				}
			}
			case operators::BinaryOp::EqualTo:
			{
				switch (expr->lhs->get_result_type())
				{
					case types::Type::Int:
					case types::Type::Bool:
					case types::Type::Char:
					{
						return llvm_ir_builder->CreateICmpEQ(lhs, rhs, "eq");
					}
					case types::Type::Float:
					{
						return llvm_ir_builder->CreateFCmpOEQ(lhs, rhs, "eq");
					}
					default:
					{
						return log_error_value("Unsupported type: " + types::to_string(expr->get_result_type()) + ", for operator: " + operators::to_string(expr->binop));
					}
				}
			}
			case operators::BinaryOp::NotEqualTo:
			{
				switch (expr->lhs->get_result_type())
				{
					case types::Type::Int:
					case types::Type::Bool:
					case types::Type::Char:
					{
						return llvm_ir_builder->CreateICmpNE(lhs, rhs, "ne");
					}
					case types::Type::Float:
					{
						return llvm_ir_builder->CreateFCmpONE(lhs, rhs, "ne");
					}
					default:
					{
						return log_error_value("Unsupported type: " + types::to_string(expr->get_result_type()) + ", for operator: " + operators::to_string(expr->binop));
					}
				}
			}
			case operators::BinaryOp::BooleanAnd:
			{
				llvm::Function* func = llvm_ir_builder->GetInsertBlock()->getParent();

				// create the lhs entry block
				llvm::BasicBlock* lhs_block = llvm::BasicBlock::Create(*llvm_context, "and_lhs_start", func);

				// create the lhs exit block
				llvm::BasicBlock* lhs_end_block = llvm::BasicBlock::Create(*llvm_context, "and_lhs_end");

				// create the rhs entry block
				llvm::BasicBlock* rhs_block = llvm::BasicBlock::Create(*llvm_context, "and_rhs_start");

				// create the rhs exit block
				llvm::BasicBlock* rhs_end_block = llvm::BasicBlock::Create(*llvm_context, "and_rhs_end");

				// create the end block
				llvm::BasicBlock* end_block = llvm::BasicBlock::Create(*llvm_context, "andend");

				// create fallthrough into lhs block
				llvm_ir_builder->CreateBr(lhs_block);

				// set insertion point to lhs block
				llvm_ir_builder->SetInsertPoint(lhs_block);

				// emit the lhs code
				lhs = generate_code_dispatch(expr->lhs.get());

				if (lhs == nullptr)
				{
					null_end;
					return nullptr;
				}

				// create fallthrough to lhs end block
				llvm_ir_builder->CreateBr(lhs_end_block);

				func->getBasicBlockList().push_back(lhs_end_block);
				// set insertion point to lhs end block
				llvm_ir_builder->SetInsertPoint(lhs_end_block);

				// create branch to rhs
				llvm_ir_builder->CreateCondBr(lhs, rhs_block, end_block);

				func->getBasicBlockList().push_back(rhs_block);
				// set insertion point to rhs block
				llvm_ir_builder->SetInsertPoint(rhs_block);

				// emit the rhs code
				rhs = generate_code_dispatch(expr->rhs.get());

				if (rhs == nullptr)
				{
					null_end;
					return nullptr;
				}

				// create fallthrough to rhs end block
				llvm_ir_builder->CreateBr(rhs_end_block);

				func->getBasicBlockList().push_back(rhs_end_block);
				// set insertion point to rhs end block
				llvm_ir_builder->SetInsertPoint(rhs_end_block);

				// create fallthrough to end block
				llvm_ir_builder->CreateBr(end_block);

				func->getBasicBlockList().push_back(end_block);
				// set insertion point to end block
				llvm_ir_builder->SetInsertPoint(end_block);

				llvm::PHINode* phi_node = llvm_ir_builder->CreatePHI(types::get_llvm_type(*llvm_context, expr->get_result_type()), 2, "ifres");

				phi_node->addIncoming(types::get_default_value(*llvm_context, types::Type::Bool), lhs_end_block);
				phi_node->addIncoming(rhs, rhs_end_block);
				return phi_node;
			}
			case operators::BinaryOp::BooleanOr:
			{
				llvm::Function* func = llvm_ir_builder->GetInsertBlock()->getParent();

				// create the lhs entry block
				llvm::BasicBlock* lhs_block = llvm::BasicBlock::Create(*llvm_context, "or_lhs_statr", func);

				// create the lhs exit block
				llvm::BasicBlock* lhs_end_block = llvm::BasicBlock::Create(*llvm_context, "or_lhs_end");

				// create the rhs entry block
				llvm::BasicBlock* rhs_block = llvm::BasicBlock::Create(*llvm_context, "or_rhs_start");

				// create the rhs exit block
				llvm::BasicBlock* rhs_end_block = llvm::BasicBlock::Create(*llvm_context, "or_rhs_end");

				// create the end block
				llvm::BasicBlock* end_block = llvm::BasicBlock::Create(*llvm_context, "or_end");

				// create fallthrough into lhs block
				llvm_ir_builder->CreateBr(lhs_block);

				// set insertion point to lhs block
				llvm_ir_builder->SetInsertPoint(lhs_block);

				// emit the lhs code
				lhs = generate_code_dispatch(expr->lhs.get());

				if (lhs == nullptr)
				{
					null_end;
					return nullptr;
				}

				// create fallthrough to lhs end block
				llvm_ir_builder->CreateBr(lhs_end_block);

				func->getBasicBlockList().push_back(lhs_end_block);
				// set insertion point to lhs end block
				llvm_ir_builder->SetInsertPoint(lhs_end_block);

				// create branch to rhs
				llvm_ir_builder->CreateCondBr(lhs, end_block, rhs_block);

				func->getBasicBlockList().push_back(rhs_block);
				// set insertion point to rhs block
				llvm_ir_builder->SetInsertPoint(rhs_block);

				// emit the rhs code
				rhs = generate_code_dispatch(expr->rhs.get());

				if (rhs == nullptr)
				{
					null_end;
					return nullptr;
				}

				// create fallthrough to rhs end block
				llvm_ir_builder->CreateBr(rhs_end_block);

				func->getBasicBlockList().push_back(rhs_end_block);
				// set insertion point to rhs end block
				llvm_ir_builder->SetInsertPoint(rhs_end_block);

				// create fallthrough to end block
				llvm_ir_builder->CreateBr(end_block);

				func->getBasicBlockList().push_back(end_block);
				// set insertion point to end block
				llvm_ir_builder->SetInsertPoint(end_block);

				llvm::PHINode* phi_node = llvm_ir_builder->CreatePHI(types::get_llvm_type(*llvm_context, expr->get_result_type()), 2, "ifres");

				phi_node->addIncoming(llvm::ConstantInt::get(types::get_llvm_type(*llvm_context, types::Type::Bool), 1, false), lhs_end_block);
				phi_node->addIncoming(rhs, rhs_end_block);
				return phi_node;
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
		llvm::Function* callee_func = llvm_module->getFunction(module::StringManager::get_string(expr->callee_id));

		if (callee_func == nullptr)
		{
			return log_error_value("unknown function referenced: " + module::StringManager::get_string(expr->callee_id));
		}

		if (callee_func->arg_size() != expr->args.size())
		{
			return log_error_value("incorrect number of arguemnts passed");
		}

		std::vector<llvm::Value*> args;
		for (int i = 0; i < expr->args.size(); i++)
		{
			args.push_back(generate_code_dispatch(expr->args[i].get()));
			if (args.back() == nullptr)
			{
				null_end;
				return nullptr;
			}
		}

		// void return types cannot have a name
		if (expr->get_result_type() == types::Type::Void)
		{
			return llvm_ir_builder->CreateCall(callee_func, args);
		}
		else
		{
			return llvm_ir_builder->CreateCall(callee_func, args, "call");
		}
	}

	template<>
	llvm::Value* LLVMBuilder::generate_code<ast::IfExpr>(ast::IfExpr* expr)
	{
		llvm::Value* cond_value = generate_code_dispatch(expr->condition.get());

		if (cond_value == nullptr)
		{
			null_end;
			return nullptr;
		}

		// convert condition to a bool, by comparing it to zero
		// TODO: fix
		cond_value = llvm_ir_builder->CreateICmpEQ(cond_value, llvm::ConstantInt::get(types::get_llvm_type(*llvm_context, types::Type::Bool), 1, false), "ifcond");

		llvm::Function* func = llvm_ir_builder->GetInsertBlock()->getParent();

		// create a block for the if and else cases
		llvm::BasicBlock* if_block = llvm::BasicBlock::Create(*llvm_context, "if_body", func);
		llvm::BasicBlock* else_block = llvm::BasicBlock::Create(*llvm_context, "else_body");
		llvm::BasicBlock* merge_block = llvm::BasicBlock::Create(*llvm_context, "if_end");

		llvm_ir_builder->CreateCondBr(cond_value, if_block, else_block);

		// emit the if body value
		llvm_ir_builder->SetInsertPoint(if_block);

		llvm::Value* if_value = generate_code_dispatch(expr->if_body.get());

		if (if_value == nullptr)
		{
			null_end;
			return nullptr;
		}

		llvm_ir_builder->CreateBr(merge_block);

		if_block = llvm_ir_builder->GetInsertBlock();

		// emit the else block
		func->getBasicBlockList().push_back(else_block);
		llvm_ir_builder->SetInsertPoint(else_block);

		llvm::Value* else_value = generate_code_dispatch(expr->else_body.get());

		if (else_value == nullptr)
		{
			null_end;
			return nullptr;
		}

		llvm_ir_builder->CreateBr(merge_block);
		else_block = llvm_ir_builder->GetInsertBlock();

		// emit merge block
		func->getBasicBlockList().push_back(merge_block);
		llvm_ir_builder->SetInsertPoint(merge_block);

		if (expr->should_return_value)
		{
			llvm::PHINode* phi_node = llvm_ir_builder->CreatePHI(types::get_llvm_type(*llvm_context, expr->get_result_type()), 2, "ifres");

			phi_node->addIncoming(if_value, if_block);
			phi_node->addIncoming(else_value, else_block);
			return phi_node;
		}
		else
		{
			return llvm::ConstantTokenNone::get(*llvm_context);
		}
	}

	template<>
	llvm::Value* LLVMBuilder::generate_code<ast::ForExpr>(ast::ForExpr* expr)
	{
		llvm::Function* func = llvm_ir_builder->GetInsertBlock()->getParent();

		// create an alloc in the start block
		llvm::AllocaInst* alloca = create_entry_block_alloca(func, types::get_llvm_type(*llvm_context, expr->var_type), module::StringManager::get_string(expr->name_id));

		// emit the start code
		llvm::Value* start_value = generate_code_dispatch(expr->start_expr.get());

		if (start_value == nullptr)
		{
			null_end;
			return nullptr;
		}

		// store the value
		llvm_ir_builder->CreateStore(start_value, alloca);

		expr->for_body->llvm_named_values[expr->name_id] = alloca;

		// create the condition block
		llvm::BasicBlock* condition_block = llvm::BasicBlock::Create(*llvm_context, "loopcond", func);

		// make a new block for the start header
		llvm::BasicBlock* loop_block = llvm::BasicBlock::Create(*llvm_context, "loopbody", func);

		// create the step block
		llvm::BasicBlock* step_block = llvm::BasicBlock::Create(*llvm_context, "loopstep", func);

		// insert a fallthrougth from the current block into the loop block
		llvm_ir_builder->CreateBr(loop_block);

		// start insertion into the loop block
		llvm_ir_builder->SetInsertPoint(loop_block);

		// push the step block for continue statements
		this->loop_continue_blocks.push_back(step_block);

		// emit the body of the loop
		if (generate_code_dispatch(expr->for_body.get()) == nullptr)
		{
			null_end;
			return nullptr;
		}

		// create fallthrough into step block
		llvm_ir_builder->CreateBr(step_block);

		// pop the step block for continue statements
		this->loop_continue_blocks.pop_back();

		// start insertion into the step block
		llvm_ir_builder->SetInsertPoint(step_block);

		// emit the step value
		llvm::Value* step_value = nullptr;

		if (expr->step_expr != nullptr)
		{
			step_value = generate_code_dispatch(expr->step_expr.get());

			if (step_value == nullptr)
			{
				null_end;
				return nullptr;
			}
		}
		else
		{
			assert(false && "empty step not implemented yet");
		}

		// create jump to loop condition block
		llvm_ir_builder->CreateBr(condition_block);

		// set insertion into condition block
		llvm_ir_builder->SetInsertPoint(condition_block);

		// compute the end condition
		llvm::Value* end_condition = generate_code_dispatch(expr->end_expr.get());

		if (end_condition == nullptr)
		{
			null_end;
			return nullptr;
		}

		// create the after loop block, and insert it
		llvm::BasicBlock* after_block = llvm::BasicBlock::Create(*llvm_context, "afterloop", func);

		// insert the conditional branch
		llvm_ir_builder->CreateCondBr(end_condition, loop_block, after_block);

		// any new code will be inserted in the after block
		llvm_ir_builder->SetInsertPoint(after_block);

		return llvm::ConstantTokenNone::get(*llvm_context);
	}

	template<>
	llvm::Value* LLVMBuilder::generate_code<ast::WhileExpr>(ast::WhileExpr* expr)
	{
		llvm::Function* func = llvm_ir_builder->GetInsertBlock()->getParent();

		// create the condition block
		llvm::BasicBlock* condition_block = llvm::BasicBlock::Create(*llvm_context, "loopcond", func);

		// insert a fallthrougth from the current block into the condition block
		llvm_ir_builder->CreateBr(condition_block);

		// create the loop block
		llvm::BasicBlock* loop_block = llvm::BasicBlock::Create(*llvm_context, "loop", func);

		// set insert to loop block
		llvm_ir_builder->SetInsertPoint(loop_block);

		// push the condition block for continue statements
		this->loop_continue_blocks.push_back(condition_block);

		// emit the body of the loop
		if (generate_code_dispatch(expr->while_body.get()) == nullptr)
		{
			null_end;
			return nullptr;
		}

		// create fallthrough into condition block
		llvm_ir_builder->CreateBr(condition_block);

		// pop the condition block for continue statements
		this->loop_continue_blocks.pop_back();

		// create the loop end block
		llvm::BasicBlock* loop_end_block = llvm::BasicBlock::Create(*llvm_context, "loopend", func);

		// set insertion into the condition block
		llvm_ir_builder->SetInsertPoint(condition_block);

		// create the conditon value
		llvm::Value* end_condition = generate_code_dispatch(expr->end_expr.get());

		if (end_condition == nullptr)
		{
			null_end;
			return nullptr;
		}

		// insert the conditional branch
		llvm_ir_builder->CreateCondBr(end_condition, loop_block, loop_end_block);

		// set insert to loop end
		llvm_ir_builder->SetInsertPoint(loop_end_block);

		return llvm::ConstantTokenNone::get(*llvm_context);
	}

	template<>
	llvm::Value* LLVMBuilder::generate_code<ast::CommentExpr>(ast::CommentExpr* expr)
	{
		return nullptr;
	}

	template<>
	llvm::Value* LLVMBuilder::generate_code<ast::ReturnExpr>(ast::ReturnExpr* expr)
	{
		if (expr->ret_expr == nullptr)
		{
			llvm_ir_builder->CreateRetVoid();
		}
		else
		{
			llvm::Value* ret_value = generate_code_dispatch(expr->ret_expr.get());

			if (ret_value == nullptr)
			{
				null_end;
				return nullptr;
			}

			llvm_ir_builder->CreateRet(ret_value);
		}

		return llvm::ConstantTokenNone::get(*llvm_context);
	}

	template<>
	llvm::Value* LLVMBuilder::generate_code<ast::ContinueExpr>(ast::ContinueExpr* expr)
	{
		llvm_ir_builder->CreateBr(this->loop_continue_blocks.back());

		return llvm::ConstantTokenNone::get(*llvm_context);
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
			case ast::AstExprType::IfExpr:
			{
				return generate_code(dynamic_cast<ast::IfExpr*>(expr));
			}
			case ast::AstExprType::ForExpr:
			{
				return generate_code(dynamic_cast<ast::ForExpr*>(expr));
			}
			case ast::AstExprType::WhileExpr:
			{
				return generate_code(dynamic_cast<ast::WhileExpr*>(expr));
			}
			case ast::AstExprType::CommentExpr:
			{
				return generate_code(dynamic_cast<ast::CommentExpr*>(expr));
			}
			case ast::AstExprType::ReturnExpr:
			{
				return generate_code(dynamic_cast<ast::ReturnExpr*>(expr));
			}
			case ast::AstExprType::ContinueExpr:
			{
				return generate_code(dynamic_cast<ast::ContinueExpr*>(expr));
			}
		}
		assert(false && "Missing Type Specialisation");
		null_end;
		return nullptr;
	}

	void LLVMBuilder::diagnostic_handler_callback(const llvm::DiagnosticInfo& di, void* context)
	{
		llvm::DiagnosticPrinterRawOStream printer(llvm::outs());

		di.print(printer);
	}

}
