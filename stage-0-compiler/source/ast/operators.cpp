#include "operators.h"

namespace operators
{
	bool is_first_char_valid(char c)
	{
		if (c == '=')
		{
			return true;
		}
		else if (c == '+')
		{
			return true;
		}
		else if (c == '-')
		{
			return true;
		}
		else if (c == '*')
		{
			return true;
		}
		else if (c == '/')
		{
			return true;
		}
		else if (c == '<')
		{
			return true;
		}
		else if (c == '>')
		{
			return true;
		}
		return false;
	}

	bool is_second_char_valid(char c)
	{
		if (c == '=')
		{
			return true;
		}
		return false;
	}

	BinaryOp is_binary_op(std::string& str)
	{
		if (str.length() == 1)
		{
			if (is_first_char_valid(str[0]))
			{
				char c = str[0];

				if (c == '=')
				{
					return BinaryOp::Assignment;
				}
				else if (c == '+')
				{
					return BinaryOp::Addition;
				}
				else if (c == '-')
				{
					return BinaryOp::Subtraction;
				}
				else if (c == '*')
				{
					return BinaryOp::Multiplication;
				}
				else if (c == '/')
				{
					return BinaryOp::Division;
				}
				else if (c == '<')
				{
					return BinaryOp::LessThan;
				}
				else if (c == '>')
				{
					return BinaryOp::GreaterThan;
				}
			}
		}
		else if (str.length() == 2)
		{
			char c1 = str[0];
			char c2 = str[1];

			if (c2 == '=')
			{
				if (c1 == '<')
				{
					return BinaryOp::LessThanEqual;
				}
				else if (c1 == '>')
				{
					return BinaryOp::GreaterThanEqual;
				}
				else if (c1 == '=')
				{
					return BinaryOp::EqualTo;
				}
			}
		}

		return BinaryOp::None;
	}

	bool is_binary_comparision(BinaryOp op)
	{
		switch (op)
		{
			case BinaryOp::LessThan:
			case BinaryOp::LessThanEqual:
			case BinaryOp::GreaterThan:
			case BinaryOp::GreaterThanEqual:
			case BinaryOp::EqualTo:
			{
				return true;
			}
			default:
			{
				return false;
			}
		}
	}

	std::string to_string(BinaryOp binop)
	{
		switch (binop)
		{
			case BinaryOp::None:
			{
				return "None";
			}
			case BinaryOp::Assignment:
			{
				return "Assignment (=)";
			}
			case BinaryOp::Addition:
			{
				return "Addition (+)";
			}
			case BinaryOp::Subtraction:
			{
				return "Subtraction (-)";
			}
			case BinaryOp::Multiplication:
			{
				return "Multiplication (*)";
			}
			case BinaryOp::Division:
			{
				return "Division (/)";
			}
			case BinaryOp::LessThan:
			{
				return "LessThan (<)";
			}
			case BinaryOp::GreaterThan:
			{
				return "LessThan (<)";
			}
		}
	}

	bool is_type_supported(BinaryOp binop, types::Type type)
	{
		switch (type)
		{
			case types::Type::Int:
			case types::Type::Float:
			{
				return true; // all operators supported
			}
			case types::Type::Bool:
			{
				if (is_binary_comparision(binop) || binop == BinaryOp::Assignment)
				{
					return true; // only support comparisons for bools
				}
				else
				{
					return false;
				}
			}
			default:
			{
				return false;
			}
		}
	}

}
