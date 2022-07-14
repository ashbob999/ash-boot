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
		else if (c == '%')
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
		else if (c == '!')
		{
			return true;
		}
		else if (c == '&')
		{
			return true;
		}
		else if (c == '|')
		{
			return true;
		}
		else if (c == '^')
		{
			return true;
		}
		else if (c == ':')
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
		else if (c == '&')
		{
			return true;
		}
		else if (c == '|')
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
		else if (c == ':')
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
				else if (c == '%')
				{
					return BinaryOp::Modulo;
				}
				else if (c == '<')
				{
					return BinaryOp::LessThan;
				}
				else if (c == '>')
				{
					return BinaryOp::GreaterThan;
				}
				else if (c == '&')
				{
					return BinaryOp::BitwiseAnd;
				}
				else if (c == '|')
				{
					return BinaryOp::BitwiseOr;
				}
				else if (c == '^')
				{
					return BinaryOp::BitwiseXor;
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
				else if (c1 == '!')
				{
					return BinaryOp::NotEqualTo;
				}
				else if (c1 == '+')
				{
					return BinaryOp::AssignmentAddition;
				}
				else if (c1 == '-')
				{
					return BinaryOp::AssignmentSubtraction;
				}
				else if (c1 == '*')
				{
					return BinaryOp::AssignmentMultiplication;
				}
				else if (c1 == '%')
				{
					return BinaryOp::AssignmentModulo;
				}
				else if (c1 == '/')
				{
					return BinaryOp::AssignmentDivision;
				}
				else if (c1 == '&')
				{
					return BinaryOp::AssignmentBitwiseAnd;
				}
				else if (c1 == '|')
				{
					return BinaryOp::AssignmentBitwiseOr;
				}
				else if (c1 == '^')
				{
					return BinaryOp::AssignmentBitwiseXor;
				}
			}
			else if (c2 == '&')
			{
				if (c1 == '&')
				{
					return BinaryOp::BooleanAnd;
				}
			}
			else if (c2 == '|')
			{
				if (c1 == '|')
				{
					return BinaryOp::BooleanOr;
				}
			}
			else if (c2 == '<')
			{
				if (c1 == '<')
				{
					return BinaryOp::BitwiseShiftLeft;
				}
			}
			else if (c2 == '>')
			{
				if (c1 == '>')
				{
					return BinaryOp::BitwiseShiftRight;
				}
			}
			else if (c2 == ':')
			{
				if (c1 == ':')
				{
					return BinaryOp::ModuleScope;
				}
			}
		}

		return BinaryOp::None;
	}

	UnaryOp is_unary_op(char c)
	{
		if (c == '+')
		{
			return UnaryOp::Plus;
		}
		else if (c == '-')
		{
			return UnaryOp::Minus;
		}
		else if (c == '!')
		{
			return UnaryOp::BooleanNot;
		}
		else if (c == '~')
		{
			return UnaryOp::BitwiseNot;
		}
		return UnaryOp::None;
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
			case BinaryOp::NotEqualTo:
			{
				return true;
			}
			default:
			{
				return false;
			}
		}
	}

	bool is_boolean_operator(BinaryOp op)
	{
		switch (op)
		{
			case BinaryOp::BooleanAnd:
			case BinaryOp::BooleanOr:
			{
				return true;
			}
		}
		return false;
	}

	bool is_boolean_operator(UnaryOp op)
	{
		switch (op)
		{
			case UnaryOp::BooleanNot:
			{
				return true;
			}
		}
		return false;
	}

	bool is_bitwise_operator(BinaryOp op)
	{
		switch (op)
		{
			case BinaryOp::BitwiseAnd:
			case BinaryOp::BitwiseOr:
			case BinaryOp::BitwiseXor:
			case BinaryOp::BitwiseShiftLeft:
			case BinaryOp::BitwiseShiftRight:
			{
				return true;
			}
		}
		return false;
	}

	bool is_bitwise_operator(UnaryOp op)
	{
		switch (op)
		{
			case UnaryOp::BitwiseNot:
			{
				return true;
			}
		}
		return false;
	}

	bool is_assignemnt(BinaryOp op)
	{
		switch (op)
		{
			case BinaryOp::Assignment:
			case BinaryOp::AssignmentAddition:
			case BinaryOp::AssignmentSubtraction:
			case BinaryOp::AssignmentMultiplication:
			case BinaryOp::AssignmentModulo:
			case BinaryOp::AssignmentDivision:
			case BinaryOp::AssignmentBitwiseAnd:
			case BinaryOp::AssignmentBitwiseOr:
			case BinaryOp::AssignmentBitwiseXor:
			{
				return true;
			}
		}
		return false;
	}

	bool is_compound_assignemnt(BinaryOp op)
	{
		switch (op)
		{
			case BinaryOp::AssignmentAddition:
			case BinaryOp::AssignmentSubtraction:
			case BinaryOp::AssignmentMultiplication:
			case BinaryOp::AssignmentModulo:
			case BinaryOp::AssignmentDivision:
			case BinaryOp::AssignmentBitwiseAnd:
			case BinaryOp::AssignmentBitwiseOr:
			case BinaryOp::AssignmentBitwiseXor:
			{
				return true;
			}
		}
		return false;
	}

	BinaryOp extract_compound_assignment_operator(BinaryOp op)
	{
		switch (op)
		{
			case BinaryOp::AssignmentAddition:
			{
				return BinaryOp::Addition;
			}
			case BinaryOp::AssignmentSubtraction:
			{
				return BinaryOp::Subtraction;
			}
			case BinaryOp::AssignmentMultiplication:
			{
				return BinaryOp::Multiplication;
			}
			case BinaryOp::AssignmentModulo:
			{
				return BinaryOp::Modulo;
			}
			case BinaryOp::AssignmentDivision:
			{
				return BinaryOp::Division;
			}
			case BinaryOp::AssignmentBitwiseAnd:
			{
				return BinaryOp::BitwiseAnd;
			}
			case BinaryOp::AssignmentBitwiseOr:
			{
				return BinaryOp::BitwiseOr;
			}
			case BinaryOp::AssignmentBitwiseXor:
			{
				return BinaryOp::BitwiseXor;
			}
		}
		assert(false && "Invalid compound assignment binary operator");
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
			case BinaryOp::AssignmentAddition:
			{
				return "AssignmentAddition (+=)";
			}
			case BinaryOp::AssignmentSubtraction:
			{
				return "AssignmentSubtraction (-=)";
			}
			case BinaryOp::AssignmentMultiplication:
			{
				return "AssignmentMultiplication (*=)";
			}
			case BinaryOp::AssignmentModulo:
			{
				return "AssignmentModulo (%=)";
			}
			case BinaryOp::AssignmentDivision:
			{
				return "AssignmentDivision (/=)";
			}
			case BinaryOp::AssignmentBitwiseAnd:
			{
				return "AssignmentBitwiseAnd (&=)";
			}
			case BinaryOp::AssignmentBitwiseOr:
			{
				return "AssignmentBitwiseOr (|=)";
			}
			case BinaryOp::AssignmentBitwiseXor:
			{
				return "AssignmentBitwiseXor (^=)";
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
			case BinaryOp::Modulo:
			{
				return "Modulo (%)";
			}
			case BinaryOp::Division:
			{
				return "Division (/)";
			}
			case BinaryOp::LessThan:
			{
				return "LessThan (<)";
			}
			case BinaryOp::LessThanEqual:
			{
				return "LessThanEqual (<=)";
			}
			case BinaryOp::GreaterThan:
			{
				return "LessThan (<)";
			}
			case BinaryOp::GreaterThanEqual:
			{
				return "GreaterThanEqual (<=)";
			}
			case BinaryOp::EqualTo:
			{
				return "EqualTo (==)";
			}
			case BinaryOp::NotEqualTo:
			{
				return "NotEqualTo (!=)";
			}
			case BinaryOp::BooleanAnd:
			{
				return "BooleanAnd (&&)";
			}
			case BinaryOp::BooleanOr:
			{
				return "BooleanOr (||)";
			}
			case BinaryOp::BitwiseAnd:
			{
				return "BitwiseOr (&)";
			}
			case BinaryOp::BitwiseOr:
			{
				return "BitwiseOr (|)";
			}
			case BinaryOp::BitwiseXor:
			{
				return "BitwiseXor (^)";
			}
			case BinaryOp::BitwiseShiftLeft:
			{
				return "BitwiseShiftLeft (<<)";
			}
			case BinaryOp::BitwiseShiftRight:
			{
				return "BitwiseShiftRight (^)";
			}
			case BinaryOp::ModuleScope:
			{
				return "ModuleScope (::)";
			}
		}
	}

	std::string to_string(UnaryOp unop)
	{
		switch (unop)
		{
			case UnaryOp::None:
			{
				return "None";
			}
			case UnaryOp::Plus:
			{
				return "Plus (+)";
			}
			case UnaryOp::Minus:
			{
				return "Minus (-)";
			}
			case UnaryOp::BooleanNot:
			{
				return "BooleanNot (!)";
			}
			case UnaryOp::BitwiseNot:
			{
				return "BitwiseNot (~)";
			}
		}
	}

	bool is_type_supported(BinaryOp binop, types::Type type)
	{
		if (binop == BinaryOp::ModuleScope)
		{
			return false; // module scope operator does not apply to types
		}
		switch (type)
		{
			case types::Type::Int:
			{
				if (!is_boolean_operator(binop))
				{
					return true; // all operators supported, apart from boolean operators
				}
				else
				{
					return false;
				}
			}
			case types::Type::Float:
			{
				if (!is_boolean_operator(binop) && !is_bitwise_operator(binop))
				{
					return true; // all operators supported, apart from boolean/bitwise operators
				}
				else
				{
					return false;
				}
			}
			case types::Type::Bool:
			{
				if (is_binary_comparision(binop) || binop == BinaryOp::Assignment || is_boolean_operator(binop))
				{
					return true; // only support comparison, assignment, and boolean ops
				}
				else
				{
					return false;
				}
			}
			case types::Type::Char:
			{
				if (is_binary_comparision(binop) || binop == BinaryOp::Assignment)
				{
					return true; // only support comparison and assignment
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

	bool is_type_supported(UnaryOp unop, types::Type type)
	{
		switch (type)
		{
			case types::Type::Int:
			{
				if (!is_boolean_operator(unop))
				{
					return true; // all operators supported, apart from boolean operators
				}
				else
				{
					return false;
				}
			}
			case types::Type::Float:
			{
				if (!is_boolean_operator(unop) && !is_bitwise_operator(unop))
				{
					return true; // all operators supported, apart from boolean/bitwise operators
				}
				else
				{
					return false;
				}
			}
			case types::Type::Bool:
			{
				if (is_boolean_operator(unop))
				{
					return true; // only support boolean operators
				}
				else
				{
					return false;
				}
			}
			case types::Type::Char:
			{
				return false;
			}
			default:
			{
				return false;
			}
		}
	}

}
