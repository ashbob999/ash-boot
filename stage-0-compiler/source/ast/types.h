#pragma once

#include <string>
#include <utility>

#include "llvm/IR/Constants.h"

namespace types
{
	// the available supported native types
	enum class Type
	{
		None,
		Int, // 32-bit signed int
		Float, // 32-bit floating-point number
		Void,
	};

	Type is_valid_type(std::string str);

	std::pair<bool, Type> check_type_string(std::string str);

	llvm::Type* get_llvm_type(llvm::LLVMContext& llvm_context, Type type);

	llvm::Value* get_default_value(llvm::LLVMContext& llvm_context, Type type);

	std::string to_string(Type type);

	union type_value
	{
		int _int;
		float _float;
	};

	class BaseType
	{
	public:
		virtual llvm::ConstantData* get_value(llvm::LLVMContext* llvm_context) = 0;
		virtual ~BaseType() = default;
		virtual std::string to_string() = 0;

	public:
		static BaseType* create_type(Type curr_type, std::string str);
		static bool is_sign_char(char c);
		static bool is_digit(char c);
	};

	class IntType : public BaseType
	{
	public:
		IntType();
		IntType(std::string);
		llvm::ConstantData* get_value(llvm::LLVMContext* llvm_context) override;
		std::string to_string() override;
	private:
		int data;
	};

	class FloatType : public BaseType
	{
	public:
		FloatType();
		FloatType(std::string);
		llvm::ConstantData* get_value(llvm::LLVMContext* llvm_context) override;
		std::string to_string() override;
	private:
		float data;
	};
};