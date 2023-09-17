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

	Type is_valid_type(const std::string& str);

	std::pair<bool, Type> check_type_string(const std::string& str);

	llvm::Type* get_llvm_type(llvm::LLVMContext& llvm_context, const Type& type);

	llvm::Value* get_default_value(llvm::LLVMContext& llvm_context, const Type& type);

	bool check_range(const std::string& literal_string, const Type& type);

	bool is_cast_valid(const Type& from, const Type& target);

	bool is_numeric(TypeEnum type);

	class BaseType
	{
	protected:
		Type type;

	public:
		virtual llvm::ConstantData* get_value(llvm::LLVMContext* llvm_context) const = 0;
		virtual ~BaseType() = default;
		virtual std::string to_string() const = 0;
		virtual void negate_value();

	private:
		virtual void set_type(const Type& type) final;

	public:
		static ptr_type<BaseType> create_type(const Type& curr_type, const std::string& str);
		static bool is_digit(char c);
	};

	class IntType : public BaseType
	{
	public:
		IntType();
		IntType(const std::string& str);
		llvm::ConstantData* get_value(llvm::LLVMContext* llvm_context) const override;
		std::string to_string() const override;
		virtual void negate_value() override;
		bool operator==(const IntType& other) const;

	public:
		static bool check_range(const std::string& literal_string);
	private:
		uint64_t data;
	};

	class FloatType : public BaseType
	{
	public:
		FloatType();
		FloatType(const std::string& str);
		llvm::ConstantData* get_value(llvm::LLVMContext* llvm_context) const override;
		std::string to_string() const override;
		virtual void negate_value() override;
	private:
		double data;
	};

	class BoolType : public BaseType
	{
	public:
		BoolType();
		BoolType(const std::string& str);
		llvm::ConstantData* get_value(llvm::LLVMContext* llvm_context) const override;
		std::string to_string() const override;
	private:
		bool data;
	};

	class CharType : public BaseType
	{
	public:
		CharType();
		CharType(const std::string& str);
		llvm::ConstantData* get_value(llvm::LLVMContext* llvm_context) const override;
		std::string to_string() const override;
	private:
		char data;
	};
};
