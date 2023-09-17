#pragma once

#include <string>
#include <utility>
#include <memory>

#include "llvm/IR/Constants.h"

#include "../config.h"

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

	class Type
	{
	public:
		Type();
		explicit Type(TypeEnum type_enum);
		Type(TypeEnum type_enum, int size, bool is_signed);
		Type(const std::string& literal, TypeEnum type_enum);
		bool operator==(const Type& other) const;
		bool operator!=(const Type& other) const;

		TypeEnum get_type_enum() const { return this->type_enum; }
		int get_size() const { return this->size; }
		bool is_signed() const { return this->signed_value; }
		std::string to_string() const;

	private:
		TypeEnum type_enum;
		int size = 0;
		bool signed_value = false;
	};

	Type is_valid_type(std::string& str);

	std::pair<bool, Type> check_type_string(std::string& str);

	llvm::Type* get_llvm_type(llvm::LLVMContext& llvm_context, Type type);

	llvm::Value* get_default_value(llvm::LLVMContext& llvm_context, Type type);

	bool check_range(std::string& literal_string, Type type);

	bool is_cast_valid(Type from, Type target);

	bool is_numeric(TypeEnum type);

	class BaseType
	{
	protected:
		Type type;

	public:
		virtual llvm::ConstantData* get_value(llvm::LLVMContext* llvm_context) = 0;
		virtual ~BaseType() = default;
		virtual std::string to_string() = 0;
		virtual void negate_value();

	private:
		virtual void set_type(Type& type) final;

	public:
		static ptr_type<BaseType> create_type(Type curr_type, std::string& str);
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
		bool operator==(const IntType& other)const;

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
		double data;
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
