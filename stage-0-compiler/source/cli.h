#pragma once

#include <filesystem>
#include <memory>

#include "ast/ast.h"
#include "ast/builder.h"
#include "ast/module.h"

namespace cli
{
	class CLI
	{
		enum class OutputType
		{
			IR,
			OBJ,
		};

	public:
		CLI(int argc, char** argv);

		bool run();

	private:
		bool parse_file();
		bool check_modules();
		bool check_ast();
		bool build_ast();
		bool output_llvm_ir();
		bool output_object_file();

	private:
		bool parsed = false;
		std::vector<std::filesystem::path> input_files;
		std::filesystem::path output_file;
		builder::LLVMBuilder llvm_builder;
		OutputType output_type = OutputType::IR;
		int current_module;
		std::vector<int> build_files_order;
	};
}
