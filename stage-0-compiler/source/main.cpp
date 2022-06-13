
#include <fstream>
#include <iostream>
#include <string>
#include <filesystem>

#include "llvm/Support/FileSystem.h"

#include "ast/parser.h"
#include "ast/builder.h"

int main(int argc, char** argv)
{
	// argv[0] is file name
	// argv[1] is input file
	// argv[2] is output file
	if (argc >= 3 || true) // TODO: temp
	{
		std::string file_name{ argv[1] };

		std::filesystem::path file_path{ file_name };
		file_path = std::filesystem::canonical(file_name);

		// check if the file path exists
		if (!std::filesystem::exists(file_path))
		{
			std::cout << "File: \"" << file_path.c_str() << "\" does not exist.";
			return 1;
		}

		// check if the file path is an actual file
		if (!std::filesystem::is_regular_file(file_path))
		{
			std::cout << "File: \"" << file_path.c_str() << "\" must be a regular text file.";
			return 1;
		}

		// open the input file stream
		// TODO: use llvm MemoryBuffer or SourceManager
		std::ifstream file_stream;

		file_stream.open(file_path);

		if (!file_stream.is_open())
		{
			std::cout << "File: \"" << file_path.c_str() << "\" could not be opened.";
			return 1;
		}

		// TODO: handle parse errors
		parser::Parser parser{ file_stream };
		//ast::BaseExpr* file_ast = parser.parse_file();
		ast::FunctionDefinition* file_ast = parser.parse_file_as_func();

		file_stream.close();

		//std::cout << std::endl;

		if (file_ast == nullptr)
		{
			std::cout << std::endl;
			std::cout << "Failed To Parse Code." << std::endl;
			return 1;
		}

		std::cout << "File Was Parsed Successfully" << std::endl;

		//std::cout << std::endl << file_ast->to_string(0) << std::endl << std::endl;

		builder::LLVMBuilder llvm_builder;
		llvm_builder.llvm_module->setSourceFileName(file_name);

		llvm::Function* func = nullptr;
		func = llvm_builder.generate_function_definition(file_ast);

		if (func == nullptr)
		{
			std::cout << "Failed To Generate LLVM IR Code" << std::endl;
			return 1;
		}
		else
		{
			std::cout << "Successfully Generated LLVM IR Code" << std::endl;

			//std::cout << std::endl;
			//llvm_builder.llvm_module->print(llvm::outs(), nullptr);
			//std::cout << std::endl;

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
		}

		if (file_ast != nullptr)
		{
			delete file_ast;
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
