#pragma once

#include <map>
#include <type_traits>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"
#include "llvm/Target/TargetMachine.h"

#include "ast.h"

namespace builder
{
	class LLVMBuilder
	{
	public:
		LLVMBuilder();
		~LLVMBuilder();

		bool set_target();
		llvm::Function* generate_function_definition(ast::FunctionDefinition* function);
		llvm::Function* generate_function_prototype(ast::FunctionPrototype* prototype);
		llvm::Value* log_error_value(const std::string& str);

	private:
		static llvm::AllocaInst* create_entry_block_alloca(llvm::Function* the_function, llvm::Type* type, llvm::StringRef name);
		llvm::Value* generate_code_dispatch(ast::BaseExpr* expr);
		template<class T, typename = std::enable_if_t<std::is_base_of_v<ast::BaseExpr, T>>>
		llvm::Value* generate_code(T* expr);
		static void diagnostic_handler_callback(const llvm::DiagnosticInfo& di, void* context);

	public:
		llvm::LLVMContext* llvm_context;
		llvm::Module* llvm_module;
		llvm::IRBuilder<>* llvm_ir_builder;
		//std::map<std::string, llvm::AllocaInst*> llvm_named_values;
		//std::map<std::string, llvm::Type*> llvm_named_types;
		llvm::TargetMachine* target_machine = nullptr;

	private:
		std::vector<llvm::BasicBlock*> continue_blocks;
		std::vector<llvm::BasicBlock*> break_blocks;
	};
}
