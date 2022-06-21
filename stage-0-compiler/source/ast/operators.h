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
		Addition,
		Subtraction,
		Multiplication,
		Division,
		LessThan,
		LessThanEqual,
		GreaterThan,
		GreaterThanEqual,
		EqualTo,
		NotEqualTo,
		BooleanAnd,
		BooleanOr,
	};

	bool is_first_char_valid(char c);
	bool is_second_char_valid(char c);
	BinaryOp is_binary_op(std::string& str);
	bool is_binary_comparision(BinaryOp op);
	bool is_boolean_operator(BinaryOp op);

	std::string to_string(BinaryOp binop);

	bool is_type_supported(BinaryOp binop, types::Type type);
}
