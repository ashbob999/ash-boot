#pragma once

#include <filesystem>
#include <memory>

#include "ast/ast.h"
#include "ast/builder.h"

namespace cli
{
	class CLI
	{
	public:
		CLI(int argc, char** argv);

		bool run();

	private:
		bool parse_file();
		bool check_ast();
		bool build_ast();
		bool output_llvm_ir();

	private:
		bool parsed = false;
		std::filesystem::path input_file;
		std::filesystem::path output_file;
		std::shared_ptr<ast::BodyExpr> body_ast;
		builder::LLVMBuilder llvm_builder;
	};
}
