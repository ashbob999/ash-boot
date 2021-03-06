#include "cli.h"

#include <iostream>
#include <fstream>

#include "llvm/Support/FileSystem.h"
#include "llvm/IR/LegacyPassManager.h"

#include "ast/parser.h"
#include "ast/type_checker.h"

namespace cli
{
	CLI::CLI(int argc, char** argv)
	{
		// argv[0] is file name
		// argv[1] is input file
		// argv[2] is output file
		if (argc < 3)
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

			return;
		}

		std::filesystem::path input_file_path{ argv[1] };

		// check if the file path exists
		if (!std::filesystem::exists(input_file_path))
		{
			std::cout << "File: \"" << input_file_path.c_str() << "\" does not exist." << std::endl;
			return;
		}

		// check if the file path is an actual file
		if (!std::filesystem::is_regular_file(input_file_path))
		{
			std::cout << "File: \"" << input_file_path.c_str() << "\" must be a regular text file." << std::endl;;
			return;
		}

		input_files.push_back(input_file_path);

		std::filesystem::path output_file_path{ argv[2] };
		output_file = output_file_path;

		// argv[3] is output type
		if (argc >= 4)
		{
			for (int i = 3; i < argc; i++)
			{
				std::string type_str{ argv[i] };

				if (type_str.rfind("--output-type=", 0) == 0) // --output-type=[ir|obj]
				{
					std::string option = type_str.substr(14);

					if (option == "ir")
					{
						output_type = OutputType::IR;
					}
					else if (option == "obj")
					{
						output_type = OutputType::OBJ;
					}
					else
					{
						std::cout << "Invalid value for --output-type option: " << option << std::endl;
						std::cout << "Valid values are: ir or obj" << std::endl;
						return;
					}
				}
				else if (type_str.rfind("--input=", 0) == 0) // --input=filename
				{
					std::string option = type_str.substr(8);
					std::filesystem::path input_file_path{ option };

					// check if the file path exists
					if (!std::filesystem::exists(input_file_path))
					{
						std::cout << "File: \"" << input_file_path.c_str() << "\" does not exist." << std::endl;
						return;
					}

					// check if the file path is an actual file
					if (!std::filesystem::is_regular_file(input_file_path))
					{
						std::cout << "File: \"" << input_file_path.c_str() << "\" must be a regular text file." << std::endl;;
						return;
					}

					input_files.push_back(input_file_path);
				}
				else
				{
					std::cout << "Invalid option: " << type_str << std::endl;
					return;
				}
			}
		}

		parsed = true;
	}

	bool CLI::run()
	{
		if (!parsed)
		{
			return false;
		}

		if (!parse_file())
		{
			return false;
		}

		if (!check_modules())
		{
			return false;
		}

		if (!check_ast())
		{
			return false;
		}

		if (!build_ast())
		{
			return false;
		}

		switch (output_type)
		{
			case OutputType::IR:
			{
				if (!output_llvm_ir())
				{
					return false;
				}
				break;
			}
			case OutputType::OBJ:
			{
				if (!output_object_file())
				{
					return false;
				}
				break;
			}
		}

		return true;
	}

	bool CLI::parse_file()
	{
		for (auto& file : input_files)
		{
			// open the input file stream
			// TODO: use llvm MemoryBuffer or SourceManager
			std::ifstream file_stream;

			file_stream.open(file);

			if (!file_stream.is_open())
			{
				std::cout << "File: \"" << file.c_str() << "\" could not be opened." << std::endl;
				return false;
			}

			// parse the file
			parser::Parser parser{ file_stream , file.string() };
			shared_ptr<ast::BodyExpr> body_ast = parser.parse_file_as_body();
			current_module = parser.get_module();

			file_stream.close();

			if (body_ast == nullptr)
			{
				std::cout << std::endl;
				std::cout << "Failed To Parse Code." << std::endl;
				return false;
			}

			module::ModuleManager::add_ast(current_module, body_ast);
		}

		std::cout << "File Was Parsed Successfully" << std::endl;

		return true;
	}

	bool CLI::check_modules()
	{
		if (!module::ModuleManager::check_modules())
		{
			std::cout << "File Failed Module Checks" << std::endl;
			return false;
		}

		// get the build file order
		build_files_order = module::ModuleManager::get_build_files_order();
		if (build_files_order.size() == 0)
		{
			return false;
		}

		std::cout << "File Passed Module Checks" << std::endl;

		return true;
	}

	bool CLI::check_ast()
	{
		// check for top level prototypes in module
		for (auto& f : build_files_order)
		{
			type_checker::TypeChecker tc;

			tc.set_file_id(f);

			ast::BodyExpr* body_ast = module::ModuleManager::get_ast(f);

			if (!tc.check_prototypes(body_ast))
			{
				std::cout << "File Failed Type Checks" << std::endl;
				return false;
			}
		}

		for (auto& f : build_files_order)
		{
			// do type checking
			type_checker::TypeChecker tc;

			tc.set_file_id(f);

			ast::BaseExpr* body_ast = module::ModuleManager::get_ast(f);

			if (!tc.check_types(body_ast))
			{
				std::cout << "File Failed Type Checks" << std::endl;
				return false;
			}
		}

		std::cout << "File Passed Type Checks" << std::endl;

		return true;
	}

	bool CLI::build_ast()
	{
		if (!llvm_builder.set_target())
		{
			std::cout << "Failed to set target" << std::endl;
			return false;
		}

		//llvm_builder.llvm_module->setSourceFileName(input_files[0].string());

		for (auto& f : build_files_order)
		{
			ast::BodyExpr* body_ast = module::ModuleManager::get_ast(f);

			// generate all of the function prototypes
			for (auto& p : body_ast->function_prototypes)
			{
				auto proto = llvm_builder.generate_function_prototype(p.second);

				if (proto == nullptr)
				{
					std::cout << "Failed To Generate LLVM IR Code For Function Prototype: " << module::StringManager::get_string(p.second->name_id) << std::endl;

					return false;
				}
			}
		}

		for (auto& f : build_files_order)
		{
			ast::BodyExpr* body_ast = module::ModuleManager::get_ast(f);

			// generate all of the top level functions
			for (auto& f : body_ast->functions)
			{
				auto func = llvm_builder.generate_function_definition(f.get());

				if (func == nullptr)
				{
					std::cout << "Failed To Generate LLVM IR Code For Function: " << module::StringManager::get_string(f->prototype->name_id) << std::endl;

					return false;
				}

				//std::cout << std::endl << f->to_string(0) << std::endl << std::endl;
			}
		}

		std::cout << "Successfully Generated LLVM IR Code" << std::endl;

		return true;
	}

	bool CLI::output_llvm_ir()
	{
		std::error_code error_code;

		// create the raw fd stream
		llvm::raw_fd_ostream output_file_stream(output_file.string(), error_code, llvm::sys::fs::CreationDisposition::CD_CreateAlways, llvm::sys::fs::FileAccess::FA_Write, llvm::sys::fs::OpenFlags::OF_TextWithCRLF);

		if (error_code)
		{
			// error
			std::cout << "Error Opening Output File Stream: " << error_code.message() << std::endl;
			return false;
		}

		// write llvm ir to file
		llvm_builder.llvm_module->print(output_file_stream, nullptr);

		std::cout << "LLVM IR Was Successfully Written To File" << std::endl;

		// close file stream
		output_file_stream.close();

		return true;
	}

	bool CLI::output_object_file()
	{
		std::error_code error_code;

		// create the raw fd stream
		llvm::raw_fd_ostream output_file_stream(output_file.string(), error_code, llvm::sys::fs::CreationDisposition::CD_CreateAlways, llvm::sys::fs::FileAccess::FA_Write, llvm::sys::fs::OpenFlags::OF_TextWithCRLF);

		if (error_code)
		{
			// error
			std::cout << "Error Opening Output File Stream: " << error_code.message() << std::endl;
			return false;
		}

		llvm::legacy::PassManager pass;
		llvm::CodeGenFileType file_type = llvm::CodeGenFileType::CGFT_ObjectFile;

		if (llvm_builder.target_machine->addPassesToEmitFile(pass, output_file_stream, nullptr, file_type))
		{
			std::cout << "Target machine cannot emit a file of this type" << std::endl;
			return false;
		}

		pass.run(*llvm_builder.llvm_module);

		// close file stream
		output_file_stream.close();

		std::cout << "Object Code Was Successfully Written To File" << std::endl;

		return true;
	}

}
