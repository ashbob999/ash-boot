#include <iostream>
#include <optional>

#include "llvm/IR/Verifier.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Host.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/ADT/APInt.h"

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

		//llvm_context->setDiagnosticHandlerCallBack(&this->diagnostic_handler_callback);
	}

	LLVMBuilder::~LLVMBuilder()
	{
		delete llvm_ir_builder;
		delete llvm_module;
		delete llvm_context;

		if (target_machine != nullptr)
		{
			delete target_machine;
		}
	}

	bool LLVMBuilder::set_target()
	{
		// setup targets
		std::string target_triple = llvm::sys::getDefaultTargetTriple();

		llvm::InitializeAllTargetInfos();
		llvm::InitializeAllTargets();
		llvm::InitializeAllTargetMCs();
		llvm::InitializeAllAsmParsers();
		llvm::InitializeAllAsmPrinters();

		std::string error;
		const llvm::Target* target = llvm::TargetRegistry::lookupTarget(target_triple, error);

		if (!target)
		{
			std::cout << error << std::endl;
			return false;
		}

		const char* cpu = "generic";
		const char* features = "";

		llvm::TargetOptions opt;
		std::optional<llvm::Reloc::Model> rec = std::optional<llvm::Reloc::Model>();

		target_machine = target->createTargetMachine(target_triple, cpu, features, opt, rec);

		llvm_module->setDataLayout(target_machine->createDataLayout());
		llvm_module->setTargetTriple(target_triple);

		return true;
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

		std::string proto_name;

		if (prototype->is_extern)
		{
			module::mangled_data data = module::Mangler::demangle(prototype->name_id);

			proto_name = data.function_name;

			llvm::Function* the_function = llvm_module->getFunction(proto_name);
			if (the_function != nullptr)
			{
				return the_function; // extern function def already exists
			}
		}
		else
		{
			proto_name = module::StringManager::get_string(prototype->name_id);
		}

		llvm::Function* f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, proto_name, llvm_module);

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
				if (function_definition->prototype->return_type.type_enum == types::TypeEnum::Void)
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

		llvm::Value* lhs = nullptr;
		llvm::Value* rhs = nullptr;

		// don't pre generate the code for the lhs & rhs if the binop is a boolean operator, or a module scope binop
		if ((!operators::is_boolean_operator(expr->binop) && expr->binop != operators::BinaryOp::ModuleScope) || expr->is_constant())
		{
			lhs = generate_code_dispatch(expr->lhs.get());
			rhs = generate_code_dispatch(expr->rhs.get());

			if (lhs == nullptr || rhs == nullptr)
			{
				null_end;
				return nullptr;
			}
		}

		llvm::Constant* lhs_constant = nullptr;
		llvm::Constant* rhs_constant = nullptr;

		// constant check
		if (expr->is_constant())
		{
			if (!llvm::isa<llvm::Constant>(lhs) || !llvm::isa<llvm::Constant>(rhs))
			{
				return log_error_value("BinaryExpr (Constant): lhs or rhs is not constant");
			}

			lhs_constant = llvm::dyn_cast<llvm::Constant>(lhs);
			rhs_constant = llvm::dyn_cast<llvm::Constant>(rhs);

			if (lhs_constant == nullptr || rhs_constant == nullptr)
			{
				return log_error_value("BinaryExpr (Constant): lhs or rhs could not be cast to a constant");
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
				switch (expr->get_result_type().type_enum)
				{
					case types::TypeEnum::Int:
					{
						if (expr->is_constant())
						{
							return llvm::ConstantExpr::getAdd(lhs_constant, rhs_constant);
						}
						return llvm_ir_builder->CreateAdd(lhs, rhs, "add");
					}
					case types::TypeEnum::Float:
					{
						if (expr->is_constant())
						{
							return llvm::ConstantExpr::get(llvm::BinaryOperator::FAdd, lhs_constant, rhs_constant);
						}
						return llvm_ir_builder->CreateFAdd(lhs, rhs, "add");
					}
					default:
					{
						return log_error_value("Unsupported type: " + types::to_string(expr->get_result_type()) + ", for operator: " + operators::to_string(expr->binop));
					}
				}
			}
			case operators::BinaryOp::Subtraction:
			{
				switch (expr->get_result_type().type_enum)
				{
					case types::TypeEnum::Int:
					{
						if (expr->is_constant())
						{
							return llvm::ConstantExpr::getSub(lhs_constant, rhs_constant);
						}
						return llvm_ir_builder->CreateSub(lhs, rhs, "sub");
					}
					case types::TypeEnum::Float:
					{
						if (expr->is_constant())
						{
							return llvm::ConstantExpr::get(llvm::BinaryOperator::FSub, lhs_constant, rhs_constant);
						}
						return llvm_ir_builder->CreateFSub(lhs, rhs, "sub");
					}
					default:
					{
						return log_error_value("Unsupported type: " + types::to_string(expr->get_result_type()) + ", for operator: " + operators::to_string(expr->binop));
					}
				}
			}
			case operators::BinaryOp::Multiplication:
			{
				switch (expr->get_result_type().type_enum)
				{
					case types::TypeEnum::Int:
					{
						if (expr->is_constant())
						{
							return llvm::ConstantExpr::getMul(lhs_constant, rhs_constant);
						}
						return llvm_ir_builder->CreateMul(lhs, rhs, "mul");
					}
					case types::TypeEnum::Float:
					{
						if (expr->is_constant())
						{
							return llvm::ConstantExpr::get(llvm::BinaryOperator::FMul, lhs_constant, rhs_constant);
						}
						return llvm_ir_builder->CreateFMul(lhs, rhs, "mul");
					}
					default:
					{
						return log_error_value("Unsupported type: " + types::to_string(expr->get_result_type()) + ", for operator: " + operators::to_string(expr->binop));
					}
				}
			}
			case operators::BinaryOp::Division:
			{
				// TODO: deal with divide by zero errors
				switch (expr->get_result_type().type_enum)
				{
					case types::TypeEnum::Int:
					{
						if (expr->lhs->get_result_type().is_signed())
						{
							if (expr->is_constant())
							{
								return llvm::ConstantExpr::get(llvm::BinaryOperator::SDiv, lhs_constant, rhs_constant);
							}
							return llvm_ir_builder->CreateSDiv(lhs, rhs, "sdiv");
						}
						else
						{
							if (expr->is_constant())
							{
								return llvm::ConstantExpr::get(llvm::BinaryOperator::UDiv, lhs_constant, rhs_constant);
							}
							return llvm_ir_builder->CreateUDiv(lhs, rhs, "udiv");
						}
					}
					case types::TypeEnum::Float:
					{
						if (expr->is_constant())
						{
							return llvm::ConstantExpr::get(llvm::BinaryOperator::FDiv, lhs_constant, rhs_constant);
						}
						return llvm_ir_builder->CreateFDiv(lhs, rhs, "fdiv");
					}
					default:
					{
						return log_error_value("Unsupported type: " + types::to_string(expr->get_result_type()) + ", for operator: " + operators::to_string(expr->binop));
					}
				}
			}
			case operators::BinaryOp::Modulo:
			{
				// TODO: deal with divide by zero errors
				switch (expr->get_result_type().type_enum)
				{
					case types::TypeEnum::Int:
					{
						if (expr->lhs->get_result_type().is_signed())
						{
							if (expr->is_constant())
							{
								return llvm::ConstantExpr::get(llvm::BinaryOperator::SRem, lhs_constant, rhs_constant);
							}
							return llvm_ir_builder->CreateSRem(lhs, rhs, "srem");
						}
						else
						{
							if (expr->is_constant())
							{
								return llvm::ConstantExpr::get(llvm::BinaryOperator::URem, lhs_constant, rhs_constant);
							}
							return llvm_ir_builder->CreateURem(lhs, rhs, "urem");
						}
					}
					case types::TypeEnum::Float:
					{
						if (expr->is_constant())
						{
							return llvm::ConstantExpr::get(llvm::BinaryOperator::FRem, lhs_constant, rhs_constant);
						}
						return llvm_ir_builder->CreateFRem(lhs, rhs, "frem");
					}
					default:
					{
						return log_error_value("Unsupported type: " + types::to_string(expr->get_result_type()) + ", for operator: " + operators::to_string(expr->binop));
					}
				}
			}
			case operators::BinaryOp::LessThan:
			{
				switch (expr->lhs->get_result_type().type_enum)
				{
					case types::TypeEnum::Int:
					case types::TypeEnum::Char:
					{
						if (expr->lhs->get_result_type().is_signed())
						{
							if (expr->is_constant())
							{
								return llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_SLT, lhs_constant, rhs_constant);
							}
							return llvm_ir_builder->CreateICmpSLT(lhs, rhs, "slt");
						}
						else
						{
							if (expr->is_constant())
							{
								return llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_ULT, lhs_constant, rhs_constant);
							}
							return llvm_ir_builder->CreateICmpULT(lhs, rhs, "ult");
						}
					}
					case types::TypeEnum::Float:
					{
						if (expr->is_constant())
						{
							return llvm::ConstantExpr::getFCmp(llvm::CmpInst::FCMP_OLT, lhs_constant, rhs_constant);
						}
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
				switch (expr->lhs->get_result_type().type_enum)
				{
					case types::TypeEnum::Int:
					case types::TypeEnum::Char:
					{
						if (expr->lhs->get_result_type().is_signed())
						{
							if (expr->is_constant())
							{
								return llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_SLE, lhs_constant, rhs_constant);
							}
							return llvm_ir_builder->CreateICmpSLE(lhs, rhs, "slte");
						}
						else
						{
							if (expr->is_constant())
							{
								return llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_ULE, lhs_constant, rhs_constant);
							}
							return llvm_ir_builder->CreateICmpULE(lhs, rhs, "ulte");
						}
					}
					case types::TypeEnum::Float:
					{
						if (expr->is_constant())
						{
							return llvm::ConstantExpr::getFCmp(llvm::CmpInst::FCMP_OLE, lhs_constant, rhs_constant);
						}
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
				switch (expr->lhs->get_result_type().type_enum)
				{
					case types::TypeEnum::Int:
					case types::TypeEnum::Char:
					{
						if (expr->lhs->get_result_type().is_signed())
						{
							if (expr->is_constant())
							{
								return llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_SGT, lhs_constant, rhs_constant);
							}
							return llvm_ir_builder->CreateICmpSGT(lhs, rhs, "sgt");
						}
						else
						{
							if (expr->is_constant())
							{
								return llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_UGT, lhs_constant, rhs_constant);
							}
							return llvm_ir_builder->CreateICmpUGT(lhs, rhs, "ugt");
						}
					}
					case types::TypeEnum::Float:
					{
						if (expr->is_constant())
						{
							return llvm::ConstantExpr::getFCmp(llvm::CmpInst::FCMP_OGT, lhs_constant, rhs_constant);
						}
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
				switch (expr->lhs->get_result_type().type_enum)
				{
					case types::TypeEnum::Int:
					case types::TypeEnum::Char:
					{
						if (expr->lhs->get_result_type().is_signed())
						{
							if (expr->is_constant())
							{
								return llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_SGE, lhs_constant, rhs_constant);
							}
							return llvm_ir_builder->CreateICmpSGE(lhs, rhs, "sgte");
						}
						else
						{
							if (expr->is_constant())
							{
								return llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_UGE, lhs_constant, rhs_constant);
							}
							return llvm_ir_builder->CreateICmpSGE(lhs, rhs, "ugte");
						}
					}
					case types::TypeEnum::Float:
					{
						if (expr->is_constant())
						{
							return llvm::ConstantExpr::getFCmp(llvm::CmpInst::FCMP_OGE, lhs_constant, rhs_constant);
						}
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
				switch (expr->lhs->get_result_type().type_enum)
				{
					case types::TypeEnum::Int:
					case types::TypeEnum::Bool:
					case types::TypeEnum::Char:
					{
						if (expr->is_constant())
						{
							return llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_EQ, lhs_constant, rhs_constant);
						}
						return llvm_ir_builder->CreateICmpEQ(lhs, rhs, "eq");
					}
					case types::TypeEnum::Float:
					{
						if (expr->is_constant())
						{
							return llvm::ConstantExpr::getFCmp(llvm::CmpInst::FCMP_OEQ, lhs_constant, rhs_constant);
						}
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
				switch (expr->lhs->get_result_type().type_enum)
				{
					case types::TypeEnum::Int:
					case types::TypeEnum::Bool:
					case types::TypeEnum::Char:
					{
						if (expr->is_constant())
						{
							return llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_NE, lhs_constant, rhs_constant);
						}
						return llvm_ir_builder->CreateICmpNE(lhs, rhs, "ne");
					}
					case types::TypeEnum::Float:
					{
						if (expr->is_constant())
						{
							return llvm::ConstantExpr::getFCmp(llvm::CmpInst::FCMP_ONE, lhs_constant, rhs_constant);
						}
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
				if (expr->is_constant())
				{
					// using bitwise 'and' only works is bool size == 1bit
					// using bool true == 1
					return llvm::ConstantExpr::getAnd(lhs_constant, rhs_constant);
				}

				llvm::Function* func = llvm_ir_builder->GetInsertBlock()->getParent();

				// create the lhs entry block
				llvm::BasicBlock* lhs_block = llvm::BasicBlock::Create(*llvm_context, "and.lhs.start", func);

				// create the lhs exit block
				llvm::BasicBlock* lhs_end_block = llvm::BasicBlock::Create(*llvm_context, "and.lhs.end");

				// create the rhs entry block
				llvm::BasicBlock* rhs_block = llvm::BasicBlock::Create(*llvm_context, "and.rhs.start");

				// create the rhs exit block
				llvm::BasicBlock* rhs_end_block = llvm::BasicBlock::Create(*llvm_context, "and.rhs.end");

				// create the end block
				llvm::BasicBlock* end_block = llvm::BasicBlock::Create(*llvm_context, "and.end");

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

				func->insert(func->end(), lhs_end_block);
				// set insertion point to lhs end block
				llvm_ir_builder->SetInsertPoint(lhs_end_block);

				// create branch to rhs
				llvm_ir_builder->CreateCondBr(lhs, rhs_block, end_block);

				func->insert(func->end(), rhs_block);
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

				func->insert(func->end(), rhs_end_block);
				// set insertion point to rhs end block
				llvm_ir_builder->SetInsertPoint(rhs_end_block);

				// create fallthrough to end block
				llvm_ir_builder->CreateBr(end_block);

				func->insert(func->end(), end_block);
				// set insertion point to end block
				llvm_ir_builder->SetInsertPoint(end_block);

				llvm::PHINode* phi_node = llvm_ir_builder->CreatePHI(types::get_llvm_type(*llvm_context, expr->get_result_type()), 2, "and.res");

				phi_node->addIncoming(types::get_default_value(*llvm_context, types::get_default_type(types::TypeEnum::Bool)), lhs_end_block);
				phi_node->addIncoming(rhs, rhs_end_block);
				return phi_node;
			}
			case operators::BinaryOp::BooleanOr:
			{
				if (expr->is_constant())
				{
					// using bitwise 'or' only works is bool size == 1bit
					// using bool true == 1
					return llvm::ConstantExpr::getOr(lhs_constant, rhs_constant);
				}

				llvm::Function* func = llvm_ir_builder->GetInsertBlock()->getParent();

				// create the lhs entry block
				llvm::BasicBlock* lhs_block = llvm::BasicBlock::Create(*llvm_context, "or.lhs.start", func);

				// create the lhs exit block
				llvm::BasicBlock* lhs_end_block = llvm::BasicBlock::Create(*llvm_context, "or.lhs.end");

				// create the rhs entry block
				llvm::BasicBlock* rhs_block = llvm::BasicBlock::Create(*llvm_context, "or.rhs.start");

				// create the rhs exit block
				llvm::BasicBlock* rhs_end_block = llvm::BasicBlock::Create(*llvm_context, "or.rhs.end");

				// create the end block
				llvm::BasicBlock* end_block = llvm::BasicBlock::Create(*llvm_context, "or.end");

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

				func->insert(func->end(), lhs_end_block);
				// set insertion point to lhs end block
				llvm_ir_builder->SetInsertPoint(lhs_end_block);

				// create branch to rhs
				llvm_ir_builder->CreateCondBr(lhs, end_block, rhs_block);

				func->insert(func->end(), rhs_block);
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

				func->insert(func->end(), rhs_end_block);
				// set insertion point to rhs end block
				llvm_ir_builder->SetInsertPoint(rhs_end_block);

				// create fallthrough to end block
				llvm_ir_builder->CreateBr(end_block);

				func->insert(func->end(), end_block);
				// set insertion point to end block
				llvm_ir_builder->SetInsertPoint(end_block);

				llvm::PHINode* phi_node = llvm_ir_builder->CreatePHI(types::get_llvm_type(*llvm_context, expr->get_result_type()), 2, "or.res");

				phi_node->addIncoming(llvm::ConstantInt::get(types::get_llvm_type(*llvm_context, types::get_default_type(types::TypeEnum::Bool)), 1, false), lhs_end_block);
				phi_node->addIncoming(rhs, rhs_end_block);
				return phi_node;
			}
			case operators::BinaryOp::BitwiseAnd:
			{
				switch (expr->lhs->get_result_type().type_enum)
				{
					case types::TypeEnum::Int:
					{
						if (expr->is_constant())
						{
							return llvm::ConstantExpr::getAnd(lhs_constant, rhs_constant);
						}
						return llvm_ir_builder->CreateAnd(lhs, rhs, "bitwise_and");
					}
					default:
					{
						return log_error_value("Unsupported type: " + types::to_string(expr->get_result_type()) + ", for operator: " + operators::to_string(expr->binop));
					}
				}
			}
			case operators::BinaryOp::BitwiseOr:
			{
				switch (expr->lhs->get_result_type().type_enum)
				{
					case types::TypeEnum::Int:
					{
						if (expr->is_constant())
						{
							return llvm::ConstantExpr::getOr(lhs_constant, rhs_constant);
						}
						return llvm_ir_builder->CreateOr(lhs, rhs, "bitwise_or");
					}
					default:
					{
						return log_error_value("Unsupported type: " + types::to_string(expr->get_result_type()) + ", for operator: " + operators::to_string(expr->binop));
					}
				}
			}
			case operators::BinaryOp::BitwiseXor:
			{
				switch (expr->lhs->get_result_type().type_enum)
				{
					case types::TypeEnum::Int:
					{
						if (expr->is_constant())
						{
							return llvm::ConstantExpr::getXor(lhs_constant, rhs_constant);
						}
						return llvm_ir_builder->CreateXor(lhs, rhs, "bitwise_xor");
					}
					default:
					{
						return log_error_value("Unsupported type: " + types::to_string(expr->get_result_type()) + ", for operator: " + operators::to_string(expr->binop));
					}
				}
			}
			case operators::BinaryOp::BitwiseShiftLeft:
			{
				switch (expr->lhs->get_result_type().type_enum)
				{
					case types::TypeEnum::Int:
					{
						if (expr->is_constant())
						{
							return llvm::ConstantExpr::getShl(lhs_constant, rhs_constant);
						}
						return llvm_ir_builder->CreateShl(lhs, rhs, "shift_left");
					}
					default:
					{
						return log_error_value("Unsupported type: " + types::to_string(expr->get_result_type()) + ", for operator: " + operators::to_string(expr->binop));
					}
				}
			}
			case operators::BinaryOp::BitwiseShiftRight:
			{
				switch (expr->lhs->get_result_type().type_enum)
				{
					case types::TypeEnum::Int:
					{
						if (expr->lhs->get_result_type().is_signed())
						{
							if (expr->is_constant())
							{
								return llvm::ConstantExpr::getAShr(lhs_constant, rhs_constant);
							}
							return llvm_ir_builder->CreateAShr(lhs, rhs, "s_shift_right");
						}
						else
						{
							if (expr->is_constant())
							{
								return llvm::ConstantExpr::getLShr(lhs_constant, rhs_constant);
							}
							return llvm_ir_builder->CreateLShr(lhs, rhs, "u_shift_right");
						}
					}
					default:
					{
						return log_error_value("Unsupported type: " + types::to_string(expr->get_result_type()) + ", for operator: " + operators::to_string(expr->binop));
					}
				}
			}
			case operators::BinaryOp::ModuleScope:
			{
				return generate_code_dispatch(expr->rhs.get());
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
		std::string name;
		if (expr->is_extern)
		{
			module::mangled_data data = module::Mangler::demangle(expr->callee_id);
			name = data.function_name;
		}
		else
		{
			name = module::StringManager::get_string(expr->callee_id);
		}

		llvm::Function* callee_func = llvm_module->getFunction(name);

		if (callee_func == nullptr)
		{
			return log_error_value("unknown function referenced: " + name);
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
		if (expr->get_result_type().type_enum == types::TypeEnum::Void)
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
		const bool has_else_body = expr->else_body != nullptr;
		// TODO: neaten up code for handling if with/without else body

		// the condition to branch on, should always be a bool
		llvm::Value* cond_value = generate_code_dispatch(expr->condition.get());

		if (cond_value == nullptr)
		{
			null_end;
			return nullptr;
		}

		llvm::Function* func = llvm_ir_builder->GetInsertBlock()->getParent();

		// create a block for the if and else cases
		llvm::BasicBlock* if_block = llvm::BasicBlock::Create(*llvm_context, "if.body", func);

		llvm::BasicBlock* else_block = nullptr;
		if (has_else_body)
		{
			else_block = llvm::BasicBlock::Create(*llvm_context, "else.body");
		}

		llvm::BasicBlock* merge_block = llvm::BasicBlock::Create(*llvm_context, "if.end");

		if (has_else_body)
		{
			llvm_ir_builder->CreateCondBr(cond_value, if_block, else_block);
		}
		else
		{
			llvm_ir_builder->CreateCondBr(cond_value, if_block, merge_block);
		}

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
		llvm::Value* else_value = nullptr;
		if (has_else_body)
		{
			func->insert(func->end(), else_block);
			llvm_ir_builder->SetInsertPoint(else_block);

			else_value = generate_code_dispatch(expr->else_body.get());

			if (else_value == nullptr)
			{
				null_end;
				return nullptr;
			}

			llvm_ir_builder->CreateBr(merge_block);
			else_block = llvm_ir_builder->GetInsertBlock();
		}

		// emit merge block
		func->insert(func->end(), merge_block);
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
		llvm::BasicBlock* condition_block = llvm::BasicBlock::Create(*llvm_context, "for.cond", func);

		// create the step block
		llvm::BasicBlock* step_block = llvm::BasicBlock::Create(*llvm_context, "for.step", func);

		// make a new block for the loop body
		llvm::BasicBlock* loop_body_block = llvm::BasicBlock::Create(*llvm_context, "for.body", func);

		// create the end loop block
		llvm::BasicBlock* end_block = llvm::BasicBlock::Create(*llvm_context, "for.end");

		// insert a fallthrougth from the current block into the loop body block
		llvm_ir_builder->CreateBr(loop_body_block);

		// start insertion into the loop body block
		llvm_ir_builder->SetInsertPoint(loop_body_block);

		// push the step block for continue statements
		this->continue_blocks.push_back(step_block);

		// push the end block for break statements
		this->break_blocks.push_back(end_block);

		// emit the body of the loop
		if (generate_code_dispatch(expr->for_body.get()) == nullptr)
		{
			null_end;
			return nullptr;
		}

		// create fallthrough into step block
		llvm_ir_builder->CreateBr(step_block);

		// pop the step block for continue statements
		this->continue_blocks.pop_back();

		// pop the condition block for break statements
		this->break_blocks.pop_back();

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

		// insert the conditional branch
		llvm_ir_builder->CreateCondBr(end_condition, loop_body_block, end_block);

		// insert the end loop block
		func->insert(func->end(), end_block);

		// any new code will be inserted in the end block
		llvm_ir_builder->SetInsertPoint(end_block);

		return llvm::ConstantTokenNone::get(*llvm_context);
	}

	template<>
	llvm::Value* LLVMBuilder::generate_code<ast::WhileExpr>(ast::WhileExpr* expr)
	{
		llvm::Function* func = llvm_ir_builder->GetInsertBlock()->getParent();

		// create the condition block
		llvm::BasicBlock* condition_block = llvm::BasicBlock::Create(*llvm_context, "while.cond", func);

		// insert a fallthrougth from the current block into the condition block
		llvm_ir_builder->CreateBr(condition_block);

		// create the loop body block
		llvm::BasicBlock* loop_body_block = llvm::BasicBlock::Create(*llvm_context, "while.body", func);

		// create the loop end block
		llvm::BasicBlock* loop_end_block = llvm::BasicBlock::Create(*llvm_context, "while.end");

		// set insert to loop body block
		llvm_ir_builder->SetInsertPoint(loop_body_block);

		// push the condition block for continue statements
		this->continue_blocks.push_back(condition_block);

		// push the loop end block for break statements
		this->break_blocks.push_back(loop_end_block);

		// emit the body of the loop
		if (generate_code_dispatch(expr->while_body.get()) == nullptr)
		{
			null_end;
			return nullptr;
		}

		// create fallthrough into condition block
		llvm_ir_builder->CreateBr(condition_block);

		// pop the condition block for continue statements
		this->continue_blocks.pop_back();

		// pop the condition block for break statements
		this->break_blocks.pop_back();

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
		llvm_ir_builder->CreateCondBr(end_condition, loop_body_block, loop_end_block);

		// insert the end loop block
		func->insert(func->end(), loop_end_block);

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
		llvm_ir_builder->CreateBr(this->continue_blocks.back());

		return llvm::ConstantTokenNone::get(*llvm_context);
	}

	template<>
	llvm::Value* LLVMBuilder::generate_code<ast::BreakExpr>(ast::BreakExpr* expr)
	{
		llvm_ir_builder->CreateBr(this->break_blocks.back());

		return llvm::ConstantTokenNone::get(*llvm_context);
	}

	template<>
	llvm::Value* LLVMBuilder::generate_code<ast::UnaryExpr>(ast::UnaryExpr* expr)
	{
		llvm::Value* expr_value = generate_code_dispatch(expr->expr.get());

		llvm::Constant* constant_value = nullptr;

		// constant check
		if (expr->is_constant())
		{
			if (!llvm::isa<llvm::Constant>(expr_value))
			{
				return log_error_value("UnaryExpr (Constant): expr is not constant");
			}

			constant_value = llvm::dyn_cast<llvm::Constant>(expr_value);

			if (constant_value == nullptr)
			{
				return log_error_value("UnaryExpr (Constant): expr could not be cast to a constant");
			}
		}

		switch (expr->unop)
		{
			case operators::UnaryOp::Plus:
			{
				return expr_value;
			}
			case operators::UnaryOp::Minus:
			{
				switch (expr->expr->get_result_type().type_enum)
				{
					case types::TypeEnum::Int:
					{
						if (expr->is_constant())
						{
							return llvm::ConstantExpr::getNeg(constant_value);
						}
						return llvm_ir_builder->CreateNeg(expr_value, "neg");
					}
					case types::TypeEnum::Float:
					{
						if (expr->is_constant())
						{
							return llvm::ConstantExpr::getSub(llvm::ConstantFP::get(constant_value->getType(), 0), constant_value);
						}
						return llvm_ir_builder->CreateFNeg(expr_value, "fneg");
					}
					default:
					{
						return log_error_value("Unsupported type: " + types::to_string(expr->get_result_type()) + ", for operator: " + operators::to_string(expr->unop));
					}
				}
			}
			case operators::UnaryOp::BooleanNot:
			{
				// using bitwise 'not' only works while bool size == 1bit
				if (expr->is_constant())
				{
					return llvm::ConstantExpr::getNot(constant_value);
				}
				return llvm_ir_builder->CreateNot(expr_value, "boolean_not");
			}
			case operators::UnaryOp::BitwiseNot:
			{
				if (expr->is_constant())
				{
					return llvm::ConstantExpr::getNot(constant_value);
				}
				return llvm_ir_builder->CreateNot(expr_value, "bitwise_not");
			}
			default:
			{
				return log_error_value("invalid unary operator");
			}
		}
	}

	template<>
	llvm::Value* LLVMBuilder::generate_code<ast::CastExpr>(ast::CastExpr* expr)
	{
		llvm::Value* expr_value = generate_code_dispatch(expr->expr.get());

		types::Type from_type = expr->expr->get_result_type();
		types::Type target_type = expr->get_result_type();

		if (from_type == target_type)
		{
			return expr_value;
		}

		llvm::Type* llvm_target_type = types::get_llvm_type(*llvm_context, target_type);

		llvm::Constant* constant_value = nullptr;

		// constant check
		if (expr->is_constant())
		{
			if (!llvm::isa<llvm::Constant>(expr_value))
			{
				return log_error_value("CastExpr (Constant): expr is not constant");
			}

			constant_value = llvm::dyn_cast<llvm::Constant>(expr_value);

			if (constant_value == nullptr)
			{
				return log_error_value("CastExpr (Constant): expr could not be cast to a constant");
			}
		}

		switch (from_type.type_enum)
		{
			case types::TypeEnum::Int:
			case types::TypeEnum::Bool:
			case types::TypeEnum::Char:
			{
				switch (target_type.type_enum)
				{
					case types::TypeEnum::Int:
					case types::TypeEnum::Char:
					{
						if (from_type.get_size() == target_type.get_size()) // same size
						{
							if (from_type.is_signed() != target_type.is_signed()) // sign cast
							{
								// TODO: maybe use bitcast instead
								return expr_value;
							}
							else // identical cast
							{
								return expr_value;
							}
						}
						else // size cast
						{
							if (from_type.get_size() > target_type.get_size()) // truncate
							{
								if (expr->is_constant())
								{
									return llvm::ConstantExpr::getTrunc(constant_value, llvm_target_type);
								}
								return llvm_ir_builder->CreateTrunc(expr_value, llvm_target_type, "cast_int_trunc");
							}
							else // extend
							{
								if (from_type.is_signed()) // signed
								{
									if (expr->is_constant())
									{
										return llvm::ConstantExpr::getSExt(constant_value, llvm_target_type);
									}
									return llvm_ir_builder->CreateSExt(expr_value, llvm_target_type, "cast_si_ext");
								}
								else // unsigned
								{
									if (expr->is_constant())
									{
										return llvm::ConstantExpr::getZExt(constant_value, llvm_target_type);
									}
									return llvm_ir_builder->CreateZExt(expr_value, llvm_target_type, "cast_ui_ext");
								}
							}
						}
					}
					case types::TypeEnum::Bool:
					{
						if (expr->is_constant())
						{
							return llvm::ConstantExpr::getTrunc(constant_value, llvm_target_type);
							return llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_NE, constant_value, llvm::ConstantInt::get(constant_value->getType(), 0, from_type.is_signed()));
						}
						return llvm_ir_builder->CreateICmpNE(expr_value, llvm::ConstantInt::get(types::get_llvm_type(*llvm_context, types::get_default_type(from_type.type_enum)), 0, from_type.is_signed()), "convert_to_bool");
					}
					case types::TypeEnum::Float:
					{
						if (from_type.is_signed()) // signed
						{
							if (expr->is_constant())
							{
								return llvm::ConstantExpr::getSIToFP(constant_value, llvm_target_type);
							}
							return llvm_ir_builder->CreateSIToFP(expr_value, llvm_target_type, "cast_si_fp");
						}
						else // unsigned
						{
							if (expr->is_constant())
							{
								return llvm::ConstantExpr::getUIToFP(constant_value, llvm_target_type);
							}
							return llvm_ir_builder->CreateUIToFP(expr_value, llvm_target_type, "cast_ui_fp");
						}
					}
					default:
					{
						return log_error_value("invalid target cast type");
					}
				}
			}
			case types::TypeEnum::Float:
			{
				switch (target_type.type_enum)
				{
					case types::TypeEnum::Int:
					case types::TypeEnum::Char:
					{
						// TODO: deal with numbers out of int range (use saturation intrinsics)
						if (target_type.is_signed()) // signed
						{
							if (expr->is_constant())
							{
								return llvm::ConstantExpr::getFPToSI(constant_value, llvm_target_type);
							}
							return llvm_ir_builder->CreateFPToSI(expr_value, llvm_target_type, "cast_fp_si");
						}
						else // unsigned
						{
							if (expr->is_constant())
							{
								return llvm::ConstantExpr::getFPToUI(constant_value, llvm_target_type);
							}
							return llvm_ir_builder->CreateFPToUI(expr_value, llvm_target_type, "cast_fp_ui");
						}
					}
					case types::TypeEnum::Float:
					{
						if (from_type.get_size() > target_type.get_size()) // truncate
						{
							if (expr->is_constant())
							{
								return llvm::ConstantExpr::getFPTrunc(constant_value, llvm_target_type);
							}
							return llvm_ir_builder->CreateFPTrunc(expr_value, llvm_target_type, "cast_fp_trunc");
						}
						else // extend
						{
							if (expr->is_constant())
							{
								return llvm::ConstantExpr::getFPExtend(constant_value, llvm_target_type);
							}
							return llvm_ir_builder->CreateFPExt(expr_value, llvm_target_type, "cast_fp_ext");
						}
					}
					default:
					{
						return log_error_value("invalid target cast type");
					}
				}
			}
			default:
			{
				return log_error_value("invalid cast");
			}
		}
		null_end;
		return nullptr;
	}

	template<>
	llvm::Value* LLVMBuilder::generate_code<ast::SwitchExpr>(ast::SwitchExpr* expr)
	{
		llvm::Function* func = llvm_ir_builder->GetInsertBlock()->getParent();

		// create the condition block
		llvm::BasicBlock* switch_end_block = llvm::BasicBlock::Create(*llvm_context, "switch.end");

		// get the switch value
		llvm::Value* switch_value = generate_code_dispatch(expr->value_expr.get());
		if (switch_value == nullptr)
		{
			null_end;
			return nullptr;
		}

		// create the switch instruction
		llvm::SwitchInst* switch_instruction = llvm_ir_builder->CreateSwitch(switch_value, switch_end_block, static_cast<unsigned int>(expr->cases.size()));

		std::vector<llvm::BasicBlock*> case_blocks;

		for (auto& case_expr : expr->cases)
		{
			llvm::BasicBlock* case_block = llvm::BasicBlock::Create(*llvm_context, "switch.case", func);
			case_blocks.push_back(case_block);

			llvm::ConstantInt* case_value = nullptr;

			if (case_expr->default_case)
			{
				case_block->setName("case.default");
			}
			else
			{
				llvm::Value* value = generate_code_dispatch(case_expr->case_expr.get());
				if (value == nullptr)
				{
					null_end;
					return nullptr;
				}

				case_value = llvm::dyn_cast<llvm::ConstantInt>(value);
				if (case_value == nullptr)
				{
					null_end;
					return nullptr;
				}
			}

			if (case_expr->default_case)
			{
				switch_instruction->setDefaultDest(case_block);
			}
			else
			{
				switch_instruction->addCase(case_value, case_block);
			}
		}

		this->break_blocks.push_back(switch_end_block);

		for (size_t i = 0; i < case_blocks.size(); i++)
		{
			llvm_ir_builder->SetInsertPoint(case_blocks[i]);

			llvm::Value* case_body = generate_code_dispatch(expr->cases[i]->case_body.get());
			if (case_body == nullptr)
			{
				null_end;
				return nullptr;
			}

			// skip adding of default br instruction, if last expression in case body is a break
			ast::BodyExpr* body = dynamic_cast<ast::BodyExpr*>(expr->cases[i]->case_body.get());
			if (body != nullptr && body->expressions.size() > 0)
			{
				ast::BreakExpr* break_expr = dynamic_cast<ast::BreakExpr*>(body->expressions.back().get());
				if (break_expr != nullptr)
				{
					continue;
				}
			}

			// add branch to next case or end
			if (i == case_blocks.size() - 1)
			{
				llvm_ir_builder->CreateBr(switch_end_block);
			}
			else
			{
				// TODO: do something about case fallthrough
				llvm_ir_builder->CreateBr(case_blocks[i + 1]);
			}
		}

		this->break_blocks.pop_back();

		// insert the end block
		func->insert(func->end(), switch_end_block);

		// set insert to switch end
		llvm_ir_builder->SetInsertPoint(switch_end_block);

		return llvm::ConstantTokenNone::get(*llvm_context);
	}

	template<>
	llvm::Value* LLVMBuilder::generate_code<ast::CaseExpr>(ast::CaseExpr* expr)
	{
		if (generate_code_dispatch(expr->case_body.get()) == nullptr)
		{
			null_end;
			return nullptr;
		}

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
			case ast::AstExprType::BreakExpr:
			{
				return generate_code(dynamic_cast<ast::BreakExpr*>(expr));
			}
			case ast::AstExprType::UnaryExpr:
			{
				return generate_code(dynamic_cast<ast::UnaryExpr*>(expr));
			}
			case ast::AstExprType::CastExpr:
			{
				return generate_code(dynamic_cast<ast::CastExpr*>(expr));
			}
			case ast::AstExprType::SwitchExpr:
			{
				return generate_code(dynamic_cast<ast::SwitchExpr*>(expr));
			}
			case ast::AstExprType::CaseExpr:
			{
				return generate_code(dynamic_cast<ast::CaseExpr*>(expr));
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
