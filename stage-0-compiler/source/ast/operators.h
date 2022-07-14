#pragma once

#include <string>

#include "types.h"

namespace operators
{
	// The different binary operators
	enum class BinaryOp
	{
		None,
		Assignment,
		AssignmentAddition,
		AssignmentSubtraction,
		AssignmentMultiplication,
		AssignmentModulo,
		AssignmentDivision,
		AssignmentBitwiseAnd,
		AssignmentBitwiseOr,
		AssignmentBitwiseXor,
		Addition,
		Subtraction,
		Multiplication,
		Modulo,
		Division,
		LessThan,
		LessThanEqual,
		GreaterThan,
		GreaterThanEqual,
		EqualTo,
		NotEqualTo,
		BooleanAnd,
		BooleanOr,
		BitwiseAnd,
		BitwiseOr,
		BitwiseXor,
		BitwiseShiftLeft,
		BitwiseShiftRight,
		ModuleScope,
	};

	enum class UnaryOp
	{
		None,
		Plus,
		Minus,
		BooleanNot,
		BitwiseNot,
	};

	bool is_first_char_valid(char c);
	bool is_second_char_valid(char c);
	BinaryOp is_binary_op(std::string& str);
	UnaryOp is_unary_op(char c);
	bool is_binary_comparision(BinaryOp op);
	bool is_boolean_operator(BinaryOp op);
	bool is_boolean_operator(UnaryOp op);
	bool is_bitwise_operator(BinaryOp op);
	bool is_bitwise_operator(UnaryOp op);
	bool is_assignemnt(BinaryOp op);
	bool is_compound_assignemnt(BinaryOp op);
	BinaryOp extract_compound_assignment_operator(BinaryOp op);

	std::string to_string(BinaryOp binop);
	std::string to_string(UnaryOp unop);

	bool is_type_supported(BinaryOp binop, types::Type type);
	bool is_type_supported(UnaryOp unop, types::Type type);
}
