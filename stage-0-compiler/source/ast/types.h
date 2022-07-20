#pragma once

#include <string>
#include <utility>
#include <memory>

#include "llvm/IR/Constants.h"

using std::shared_ptr;

namespace types
{
	// the available supported native types
	enum class TypeEnum
	{
		None,
		Int, // 32-bit signed int
		Float, // 32-bit floating-point number
		Void,
		Bool, // either true (1), false (0)
		Char, // 8-bit signed integer
	};

	struct Type
	{
		TypeEnum type_enum;
		int data;

		Type();
		Type(TypeEnum type_enum);
		bool operator==(const Type& other);
		bool operator!=(const Type& other);
	};

	Type is_valid_type(std::string& str);

	std::pair<bool, Type> check_type_string(std::string& str);

	llvm::Type* get_llvm_type(llvm::LLVMContext& llvm_context, Type type);

	llvm::Value* get_default_value(llvm::LLVMContext& llvm_context, Type type);

	std::string to_string(Type type);

	bool check_range(std::string& literal_string, Type type);

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
		virtual void negate_value();

	public:
		static shared_ptr<BaseType> create_type(Type curr_type, std::string& str);
		static bool is_sign_char(char c);
		static bool is_digit(char c);
	};

	class IntType : public BaseType
	{
	public:
		IntType();
		IntType(std::string& str);
		llvm::ConstantData* get_value(llvm::LLVMContext* llvm_context) override;
		std::string to_string() override;
		virtual void negate_value() override;

	public:
		static bool check_range(std::string& literal_string);
	private:
		uint64_t data;
	};

	class FloatType : public BaseType
	{
	public:
		FloatType();
		FloatType(std::string& str);
		llvm::ConstantData* get_value(llvm::LLVMContext* llvm_context) override;
		std::string to_string() override;
		virtual void negate_value() override;
	private:
		float data;
	};

	class BoolType : public BaseType
	{
	public:
		BoolType();
		BoolType(std::string& str);
		llvm::ConstantData* get_value(llvm::LLVMContext* llvm_context) override;
		std::string to_string() override;
	private:
		bool data;
	};

	class CharType : public BaseType
	{
	public:
		CharType();
		CharType(std::string& str);
		llvm::ConstantData* get_value(llvm::LLVMContext* llvm_context) override;
		std::string to_string() override;
	private:
		char data;
	};
};
