#include <regex>
#include <iostream>

#include "types.h"

using std::make_shared;

namespace types
{
	Type is_valid_type(std::string& str)
	{
		if (str == "int")
		{
			return Type::Int;
		}
		else if (str == "float")
		{
			return Type::Float;
		}
		else if (str == "void")
		{
			return Type::Void;
		}
		else if (str == "bool")
		{
			return Type::Bool;
		}
		return Type::None;
	}

	std::pair<bool, Type> check_type_string(std::string& str)
	{
		// type formats
		// int: [+-]?[0-9][0-9]*
		// float: [+-]?([0-9][0-9]*)[.]([0-9][0-9]*)[f]?

		std::regex int_regex{ "[+-]?[0-9][0-9]*" };
		std::regex float_regex{ "[+-]?[0-9][0-9]*[.][0-9][0-9]*[f]?" };
		std::regex bool_regex{ "(true|false)" };

		std::smatch match;

		if (std::regex_match(str, match, int_regex))
		{
			return { true, Type::Int };
		}
		else if (std::regex_match(str, match, float_regex))
		{
			return { true, Type::Float };
		}

		else if (std::regex_match(str, match, bool_regex))
		{
			return { true, Type::Bool };
		}

		return { false, Type::None };
	}

	llvm::Type* get_llvm_type(llvm::LLVMContext& llvm_context, Type type)
	{
		switch (type)
		{
			case Type::Int:
			{
				return llvm::Type::getInt32Ty(llvm_context);
			}
			case Type::Float:
			{
				return llvm::Type::getFloatTy(llvm_context);
			}
			case Type::Void:
			{
				return llvm::Type::getVoidTy(llvm_context);
			}
			case Type::Bool:
			{
				return llvm::Type::getInt1Ty(llvm_context);
			}
			default:
			{
				return nullptr;
			}
		}
	}

	llvm::Value* get_default_value(llvm::LLVMContext& llvm_context, Type type)
	{
		switch (type)
		{
			case Type::Int:
			{
				return llvm::ConstantInt::get(llvm_context, llvm::APInt(32, 0, false));
			}
			case Type::Float:
			{
				return llvm::ConstantFP::get(llvm_context, llvm::APFloat((float) 0));
			}
			case Type::Bool:
			{
				return llvm::ConstantInt::get(llvm_context, llvm::APInt(1, 0, false));
			}
			default:
			{
				return nullptr;
			}
		}
	}

	std::string to_string(Type type)
	{
		switch (type)
		{
			case Type::None:
			{
				return "None";
			}
			case Type::Int:
			{
				return "Int";
			}
			case Type::Float:
			{
				return "Float";
			}
			case Type::Void:
			{
				return "Void";
			}
			case Type::Bool:
			{
				return "Bool";
			}
		}
	}

	shared_ptr<BaseType> BaseType::create_type(Type curr_type, std::string& str)
	{
		switch (curr_type)
		{
			case types::Type::Int:
			{
				return make_shared<IntType>(str);
			}
			case types::Type::Float:
			{
				return make_shared<FloatType>(str);
			}
			case types::Type::Bool:
			{
				return make_shared<BoolType>(str);
			}
		}

		return nullptr;
	}

	bool BaseType::is_sign_char(char c)
	{
		return c == '+' || c == '-';
	}

	bool BaseType::is_digit(char c)
	{
		return c >= '0' && c <= '9';
	}

	IntType::IntType()
	{
		this->data = 0;
	}

	IntType::IntType(std::string& str)
	{
		bool is_negative = false;

		int i = 0;

		// does the number have a sign
		if (is_sign_char(str[i]))
		{
			if (str[i] == '-')
			{
				is_negative = true;
			}
			i++;
		}

		int value = 0;

		// parse the numbers
		for (; i < str.length(); i++)
		{
			value = value * 10 + (str[i] - '0');
		}

		// replace the sign value
		if (is_negative)
		{
			value *= -1;
		}

		//std::cout << "int " << value << std::endl;

		data = value;
	}

	llvm::ConstantData* IntType::get_value(llvm::LLVMContext* llvm_context)
	{
		// create 32 bit signed int type
		return llvm::ConstantInt::get(*llvm_context, llvm::APInt(32, data, false));
	}

	std::string IntType::to_string()
	{
		return std::to_string(this->data);
	}

	FloatType::FloatType()
	{
		this->data = 0.0f;
	}

	FloatType::FloatType(std::string& str)
	{
		bool is_negative = false;
		int whole = 0;
		int fraction = 0;
		int decimal_power = 1;
		bool in_fraction = false;

		int i = 0;

		// does the number have a sign
		if (is_sign_char(str[i]))
		{
			if (str[0] == '-')
			{
				is_negative = true;
			}
			i++;
		}

		//float value = 0;

		// parse the numbers
		for (; i < str.length() - 1; i++)
		{
			if (str[i] == '.')
			{
				if (!in_fraction)
				{
					in_fraction = true;
				}
			}
			else
			{
				if (!in_fraction)
				{
					whole = whole * 10 + (str[i] - '0');
				}
				else
				{
					fraction = fraction * 10 + (str[i] - '0');
					decimal_power *= 10;
				}

			}
		}

		// handle last char either 0-9 or f
		if (is_digit(str[i]))
		{
			if (!in_fraction)
			{
				whole = whole * 10 + (str[i] - '0');
			}
			else
			{
				fraction = fraction * 10 + (str[i] - '0');
				decimal_power *= 10;
			}
		}

		float value = (float) whole;
		value += (float) fraction / (float) decimal_power;

		// replace the sign value
		if (is_negative)
		{
			value *= -1;
		}

		//std::cout << "float " << value << std::endl;

		data = value;
	}
	llvm::ConstantData* FloatType::get_value(llvm::LLVMContext* llvm_context)
	{
		// create 32 bit floating point number
		return llvm::ConstantFP::get(*llvm_context, llvm::APFloat(data));
	}
	std::string FloatType::to_string()
	{
		return std::to_string(this->data);
	}

	BoolType::BoolType()
	{
		this->data = false;
	}

	BoolType::BoolType(std::string& str)
	{
		bool value = false;

		if (str == "true")
		{
			value = true;
		}
		else if (str == "false")
		{
			value = false;
		}

		data = value;
	}

	llvm::ConstantData* BoolType::get_value(llvm::LLVMContext* llvm_context)
	{
		if (data)
		{
			return llvm::ConstantInt::get(*llvm_context, llvm::APInt(1, 1, false));
		}
		else
		{
			return llvm::ConstantInt::get(*llvm_context, llvm::APInt(1, 0, false));
		}
	}

	std::string BoolType::to_string()
	{
		if (data)
		{
			return "true";
		}
		else
		{
			return "false";
		}
	}
}
