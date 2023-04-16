#include <regex>
#include <iostream>

#include "types.h"

using std::make_shared;

namespace types
{
	Type::Type() : type_enum(TypeEnum::None), data(0)
	{}

	Type::Type(TypeEnum type_enum) : type_enum(type_enum), data(0)
	{}

	bool Type::operator==(const Type& other)
	{
		if (this->type_enum == other.type_enum && this->data == other.data)
		{
			return true;
		}
		return false;
	}

	bool Type::operator!=(const Type& other)
	{
		return !(*this == other);
	}

	Type::Type(TypeEnum type_enum, int size, bool is_signed) : type_enum(type_enum)
	{
		// data bit-0: sign bit
		// data bit1-31: size bits
		data = size << 1;
		if (is_signed)
		{
			data |= 1;
		}
	}

	int Type::get_size()
	{
		return data >> 1;
	}

	bool Type::is_signed()
	{
		return (data & 1) == 1;
	}

	Type is_valid_type(std::string& str)
	{
		if (str == "int")
		{
			return get_default_type(TypeEnum::Int);
		}
		else if ((str[0] == 'i' || str[0] == 'u') && str.length() > 1)
		{
			int size = std::stoi(std::string{ str.begin() + 1, str.end() });
			if (size >= 8 && size <= 64 && (size & (size - 1)) == 0)
			{
				return { TypeEnum::Int, size, str[0] == 'i' };
			}
		}
		else if (str == "float")
		{
			return get_default_type(TypeEnum::Float);
		}
		else if (str[0] == 'f' && str.length() > 1)
		{
			int size = std::stoi(std::string{ str.begin() + 1, str.end() });
			if (size >= 32 && size <= 64 && (size & (size - 1)) == 0)
			{
				return { TypeEnum::Float, size, true };
			}
		}
		else if (str == "void")
		{
			return TypeEnum::Void;
		}
		else if (str == "bool")
		{
			return { TypeEnum::Bool, 1, false };
		}
		else if (str == "char")
		{
			return { TypeEnum::Char, 8, true };
		}
		return TypeEnum::None;
	}

	std::pair<int, bool> get_literal_data(std::string& str, TypeEnum type)
	{
		char c1 = '\0';
		char c2 = '\0';
		if (type == TypeEnum::Float)
		{
			c1 = 'f';
			c2 = c1;
		}
		else if (type == TypeEnum::Int)
		{
			c1 = 'i';
			c2 = 'u';
		}
		else
		{
			Type t = get_default_type(type);
			return { t.get_size(), t.is_signed() };
		}

		int i = str.length() - 1;
		while (i > 0 && str[i] != c1 && str[i] != c2)
		{
			i--;
		}

		if (i == 0 || i == str.length() - 1)
		{
			Type t = get_default_type(type);
			return { t.get_size(), t.is_signed() };
		}

		return { std::stoi(std::string{ str.begin() + i + 1, str.end() }), str[i] != 'u' };
	}

	std::pair<bool, Type> check_type_string(std::string& str)
	{
		// type formats
		// int: [0-9][0-9]*(i(8|16|32|64)?)?
		// float: ([0-9][0-9]*)[.]([0-9][0-9]*)(f(32|64)?)?
		// char: '([^']|\\.)'

		std::regex int_regex{ "[0-9][0-9]*((i|u)(8|16|32|64)?)?" };
		std::regex float_regex{ "[0-9][0-9]*[.][0-9][0-9]*(f(32|64)?)?" };
		std::regex bool_regex{ "(true|false)" };
		std::regex char_regex{ "'([^']|\\\\.)'" };

		std::smatch match;

		if (std::regex_match(str, match, int_regex))
		{
			std::pair<int, bool> data = get_literal_data(str, TypeEnum::Int);
			return { true, {TypeEnum::Int, data.first, data.second} };
		}
		else if (std::regex_match(str, match, float_regex))
		{
			std::pair<int, bool> data = get_literal_data(str, TypeEnum::Float);
			return { true, {TypeEnum::Float, data.first, true} };
		}
		else if (std::regex_match(str, match, bool_regex))
		{
			return { true, get_default_type(TypeEnum::Bool) };
		}
		else if (std::regex_match(str, match, char_regex))
		{
			return { true, get_default_type(TypeEnum::Char) };
		}

		return { false, get_default_type(TypeEnum::None) };
	}

	llvm::Type* get_llvm_type(llvm::LLVMContext& llvm_context, Type type)
	{
		switch (type.type_enum)
		{
			case TypeEnum::Int:
			case TypeEnum::Bool:
			case TypeEnum::Char:
			{
				return llvm::IntegerType::get(llvm_context, type.get_size());
			}
			case TypeEnum::Float:
			{
				if (type.get_size() == 32)
				{
					return llvm::Type::getFloatTy(llvm_context);
				}
				else if (type.get_size() == 64)
				{
					return llvm::Type::getDoubleTy(llvm_context);
				}
				break;
			}
			case TypeEnum::Void:
			{
				return llvm::Type::getVoidTy(llvm_context);
			}
			default:
			{
				return nullptr;
			}
		}
		return nullptr;
	}

	llvm::Value* get_default_value(llvm::LLVMContext& llvm_context, Type type)
	{
		switch (type.type_enum)
		{
			case TypeEnum::Int:
			case TypeEnum::Bool:
			case TypeEnum::Char:
			{
				return llvm::ConstantInt::get(llvm_context, llvm::APInt(type.get_size(), 0, type.is_signed()));
			}
			case TypeEnum::Float:
			{
				if (type.get_size() == 32)
				{
					return llvm::ConstantFP::get(llvm_context, llvm::APFloat(0.0f));
				}
				else if (type.get_size() == 64)
				{
					return llvm::ConstantFP::get(llvm_context, llvm::APFloat(0.0));
				}
				break;
			}
			default:
			{
				return nullptr;
			}
		}
	}

	std::string to_string(Type type)
	{
		switch (type.type_enum)
		{
			case TypeEnum::None:
			{
				return "None";
			}
			case TypeEnum::Int:
			{
				if (type.is_signed())
				{
					return "i" + std::to_string(type.get_size());
				}
				else
				{
					return "u" + std::to_string(type.get_size());
				}
			}
			case TypeEnum::Float:
			{
				return "f" + std::to_string(type.get_size());
			}
			case TypeEnum::Void:
			{
				return "Void";
			}
			case TypeEnum::Bool:
			{
				return "Bool";
			}
			case TypeEnum::Char:
			{
				return "Char";
			}
		}
	}

	bool check_range(std::string& literal_string, Type type)
	{
		switch (type.type_enum)
		{
			case types::TypeEnum::Int:
			{
				return IntType::check_range(literal_string);
			}
			case types::TypeEnum::Float:
			{
				// TODO: do an actual range check for floats
				return true;
			}
			case types::TypeEnum::Bool:
			{
				return true;
			}
			case types::TypeEnum::Char:
			{
				return true;
			}
		}
		return false;
	}

	Type get_default_type(TypeEnum type_enum)
	{
		switch (type_enum)
		{
			case TypeEnum::Int:
			{
				return { TypeEnum::Int, 32, true };
			}
			case TypeEnum::Float:
			{
				return { TypeEnum::Float, 32, true };
			}
			case TypeEnum::Bool:
			{
				return { TypeEnum::Bool, 1, false };
			}
			case TypeEnum::Char:
			{
				return { TypeEnum::Char, 8, true };
			}
			default:
			{
				return type_enum;
			}
		}
	}

	bool is_cast_valid(Type from, Type target)
	{
		switch (from.type_enum)
		{
			case TypeEnum::Int: // int -> numeric
			{
				switch (target.type_enum)
				{
					case TypeEnum::Bool:
					case TypeEnum::Char:
					{
						return true;
					}
					case TypeEnum::Float:
					{
						return true;
					}
					case TypeEnum::Int:
					{
						bool sign_diff = from.is_signed() != target.is_signed();
						bool size_diff = from.get_size() != target.get_size();

						// only allow either size or sign cast not both
						if (sign_diff && size_diff)
						{
							return false;
						}
						else
						{
							return true;
						}
					}
					default:
					{
						return false;
					}
				}
			}
			case TypeEnum::Float: // float -> numeric
			{
				if (target.type_enum == TypeEnum::Bool)
				{
					return false;
				}
				else
				{
					return is_numeric(target.type_enum);
				}
			}
			case TypeEnum::Bool: // bool -> numeric
			{
				return is_numeric(target.type_enum);
			}
			case TypeEnum::Char: // char -> numeric
			{
				return is_numeric(target.type_enum);
			}
			default:
			{
				return false;
			}
		}
	}

	bool is_numeric(TypeEnum type)
	{
		switch (type)
		{
			case TypeEnum::Int:
			case TypeEnum::Bool:
			case TypeEnum::Char:
			case TypeEnum::Float:
			{
				return true;
			}
			default:
			{
				return false;
			}
		}
	}

	void BaseType::negate_value()
	{
		assert(false && "Negation not supported");
	}

	void BaseType::set_type(Type& type)
	{
		this->type = type;
	}

	shared_ptr<BaseType> BaseType::create_type(Type curr_type, std::string& str)
	{
		shared_ptr<BaseType> type = nullptr;

		switch (curr_type.type_enum)
		{
			case TypeEnum::Int:
			{
				type = make_shared<IntType>(str);
				break;
			}
			case TypeEnum::Float:
			{
				type = make_shared<FloatType>(str);
				break;
			}
			case TypeEnum::Bool:
			{
				type = make_shared<BoolType>(str);
				break;
			}
			case TypeEnum::Char:
			{
				type = make_shared<CharType>(str);
				break;
			}
		}

		type->set_type(curr_type);

		return type;
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
		int i = 0;

		uint64_t value = 0;

		// parse the numbers
		for (; i < str.length() && std::isdigit(str[i]); i++)
		{
			value = value * 10 + (str[i] - '0');
		}

		//std::cout << "int " << value << std::endl;

		data = value;
	}

	llvm::ConstantData* IntType::get_value(llvm::LLVMContext* llvm_context)
	{
		// create 32 bit signed int type
		return llvm::ConstantInt::get(*llvm_context, llvm::APInt(type.get_size(), data, type.is_signed()));
	}

	std::string IntType::to_string()
	{
		return std::to_string((int64_t) this->data);
	}

	void IntType::negate_value()
	{
		this->data = -data;
	}

	bool IntType::check_range(std::string& str)
	{
		std::pair<int, bool> data = get_literal_data(str, TypeEnum::Int);

		std::string int_min;
		std::string int_max;
		int length;

		// TODO: issue with signed range, eg. for i8: 128 represents -128

		switch (data.first)
		{
			case 8:
			{
				if (data.second) // signed
				{
					int_min = "000";
					int_max = "128";
				}
				else  // unsigned
				{
					int_min = "000";
					int_max = "255";
				}
				length = 3;
				break;
			}
			case 16:
			{
				if (data.second) // signed
				{
					int_min = "00000";
					int_max = "32768";
				}
				else  // unsigned
				{
					int_min = "00000";
					int_max = "65535";
				}
				length = 5;
				break;
			}
			case 32:
			{
				if (data.second) // signed
				{
					int_min = "0000000000";
					int_max = "2147483648";
				}
				else  // unsigned
				{
					int_min = "0000000000";
					int_max = "4294967295";
				}
				length = 10;
				break;
			}
			case 64:
			{
				if (data.second) // signed
				{
					int_min = "0000000000000000000";
					int_max = "9223372036854775808";
					length = 19;
				}
				else  // unsigned
				{
					int_min = "00000000000000000000";
					int_max = "18446744073709551615";
					length = 20;
				}

				break;
			}
			default:
			{
				return false;
			}
		}

		std::string literal_string;
		for (auto& c : str)
		{
			if (!std::isdigit(c))
			{
				break;
			}
			else
			{
				literal_string += c;
			}
		}

		int j = 2147483649L;

		// strip leading zeroes
		while (literal_string[0] == '0' && literal_string.size() > 1)
		{
			literal_string.erase(literal_string.begin());
		}

		if (literal_string.size() < length)
		{
			return true;
		}

		if (literal_string.size() > length)
		{
			return false;
		}

		int cmp = literal_string.compare(int_max);
		return cmp <= 0;

		return false;
	}

	FloatType::FloatType()
	{
		this->data = 0.0f;
	}

	FloatType::FloatType(std::string& str)
	{
		int whole = 0;
		int fraction = 0;
		int decimal_power = 1;
		bool in_fraction = false;

		int i = 0;

		// parse the numbers
		for (; str[i] != 'f' && i < str.length(); i++)
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

		double value = (double) whole;
		value += (double) fraction / (double) decimal_power;

		//std::cout << "float " << value << std::endl;

		data = value;
	}

	llvm::ConstantData* FloatType::get_value(llvm::LLVMContext* llvm_context)
	{
		if (this->type.get_size() == 32)
		{
			// create 32 bit floating point number
			return llvm::ConstantFP::get(*llvm_context, llvm::APFloat((float) data));
		}
		else if (this->type.get_size() == 64)
		{
			// create 64 bit floating point number
			return llvm::ConstantFP::get(*llvm_context, llvm::APFloat(data));
		}
	}

	std::string FloatType::to_string()
	{
		return std::to_string(this->data);
	}

	void FloatType::negate_value()
	{
		this->data = -data;
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

	CharType::CharType()
	{
		this->data = '\0';
	}

	CharType::CharType(std::string& str)
	{
		char value;

		if (str[1] != '\\')
		{
			value = str[1];
		}
		else
		{
			if (str[2] == '\'')
			{
				value = '\'';
			}
			else if (str[2] == '\"')
			{
				value = '"';
			}
			else if (str[2] == '\\')
			{
				value = '\\';
			}
			else if (str[2] == 'a')
			{
				value = '\a';
			}
			else if (str[2] == 'b')
			{
				value = '\b';
			}
			else if (str[2] == 'f')
			{
				value = '\f';
			}
			else if (str[2] == 'n')
			{
				value = '\n';
			}
			else if (str[2] == 'r')
			{
				value = '\r';
			}
			else if (str[2] == 't')
			{
				value = '\t';
			}
			else if (str[2] == 'v')
			{
				value = '\v';
			}
			else if (str[2] == '0')
			{
				value = '\0';
			}
			else
			{
				value = str[2];
			}
		}

		data = value;
	}

	llvm::ConstantData* CharType::get_value(llvm::LLVMContext* llvm_context)
	{
		return llvm::ConstantInt::get(*llvm_context, llvm::APInt(8, data, true));
	}

	std::string CharType::to_string()
	{
		return std::string{ data };
	}

}
