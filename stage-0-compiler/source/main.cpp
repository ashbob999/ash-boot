
#include <fstream>
#include <iostream>
#include <string>
#include <filesystem>

#include "llvm/Support/FileSystem.h"

#include "ast/parser.h"
#include "ast/builder.h"
#include "ast/type_checker.h"

int main(int argc, char** argv)
{
	// argv[0] is file name
	// argv[1] is input file
	// argv[2] is output file
	if (argc >= 3 || true) // TODO: temp
	{
		std::string file_name{ argv[1] };

		std::filesystem::path file_path{ file_name };

		// check if the file path exists
		if (!std::filesystem::exists(file_path))
		{
			std::cout << "File: \"" << file_path.c_str() << "\" does not exist." << std::endl;
			return 1;
		}

		// check if the file path is an actual file
		if (!std::filesystem::is_regular_file(file_path))
		{
			std::cout << "File: \"" << file_path.c_str() << "\" must be a regular text file." << std::endl;;
			return 1;
		}

		// open the input file stream
		// TODO: use llvm MemoryBuffer or SourceManager
		std::ifstream file_stream;

		file_stream.open(file_path);

		if (!file_stream.is_open())
		{
			std::cout << "File: \"" << file_path.c_str() << "\" could not be opened." << std::endl;
			return 1;
		}

		// TODO: handle parse errors
		parser::Parser parser{ file_stream };
		shared_ptr<ast::BodyExpr> body_ast = parser.parse_file_as_body();

		file_stream.close();

		if (body_ast == nullptr)
		{
			std::cout << std::endl;
			std::cout << "Failed To Parse Code." << std::endl;
			return 1;
		}

		std::cout << "File Was Parsed Successfully" << std::endl;

		// do type checking
		type_checker::TypeChecker tc;

		if (!tc.check_types(body_ast.get()))
		{
			std::cout << "File Failed Type Checks" << std::endl;
			return 1;
		}

		std::cout << "File Passed Type Checks" << std::endl;

		builder::LLVMBuilder llvm_builder;
		llvm_builder.llvm_module->setSourceFileName(file_name);

		llvm::Function* func = nullptr;

		// generate all of the function prototypes
		for (auto& p : body_ast->function_prototypes)
		{
			auto proto = llvm_builder.generate_function_prototype(p.second);

			if (proto == nullptr)
			{
				std::cout << "Failed To Generate LLVM IR Code For Function Prototype: " << p.second->name << std::endl;

				return 1;
			}
		}

		// generate all of the top level functions
		for (auto& f : body_ast->functions)
		{
			auto func = llvm_builder.generate_function_definition(f.get());

			if (func == nullptr)
			{
				std::cout << "Failed To Generate LLVM IR Code For Function: " << f->prototype->name << std::endl;

				//delete body_ast;

				return 1;
			}

			//std::cout << std::endl << f->to_string(0) << std::endl << std::endl;
		}

		std::cout << "Successfully Generated LLVM IR Code" << std::endl;

		std::string output_file_name{ argv[2] };

		std::error_code error_code;

		// create the raw fd stream
		llvm::raw_fd_ostream output_file_stream(output_file_name, error_code, llvm::sys::fs::CreationDisposition::CD_CreateAlways, llvm::sys::fs::FileAccess::FA_Write, llvm::sys::fs::OpenFlags::OF_TextWithCRLF);

		if (error_code)
		{
			// error
			std::cout << "Error Opening Output File Stream: " << error_code.message() << std::endl;
			return 1;
		}
		else
		{
			// write llvm ir to file
			llvm_builder.llvm_module->print(output_file_stream, nullptr);

			std::cout << "LLVM IR Was Successfully Written To File" << std::endl;

			// close file stream
			output_file_stream.close();
		}

		if (body_ast != nullptr)
		{
			//delete body_ast;
		}
	}
	else
	{
		if (argc < 2)
		{
			std::cout << "No input file specified." << std::endl;
		}
		std::cout << "No ouput file specified." << std::endl;

		std::cout << "Usage: ";

#ifdef _WIN64
		std::cout << "ash-boot-stage0.exe";
#endif
#ifdef __linux__
		std::cout << "./ash-boot-stage0";
#endif

		std::cout << " input-file output-file" << std::endl;
	}

	return 0;
}
