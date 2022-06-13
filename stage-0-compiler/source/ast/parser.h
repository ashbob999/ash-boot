#pragma once

#include <filesystem>
#include <fstream>
#include <unordered_map>

#include "ast.h"

namespace parser
{
	enum class Token
	{
		EndOfFile,
		EndOfExpression,
		VariableDeclaration,
		VariableReference,
		LiteralValue,
		BinaryOperator,
		FunctionDefinition,
		BodyStart,
		BodyEnd,
		ParenStart,
		ParenEnd,
		Comma,
		None,
	};

	class LineInfo
	{
	public:
		int line_count = 0;
		int line_pos = 0;
		std::string line;
	};

	class Parser
	{
	public:
		Parser(std::ifstream& input_file);

		ast::BaseExpr* parse_file();
		ast::FunctionDefinition* parse_file_as_func();
		ast::BodyExpr* parse_file_as_body();
		static const std::unordered_map<char, int> binop_precedence;
	private:
		char get_char();
		char peek_char();
		Token get_next_token();
		ast::BodyExpr* parse_body(bool is_top_level, bool has_curly_brackets);
		ast::FunctionDefinition* parse_top_level();
		ast::BaseExpr* parse_expression(bool for_call);
		ast::BaseExpr* parse_primary();
		ast::BaseExpr* parse_binop_rhs(int precedence, ast::BaseExpr* lhs);
		ast::BaseExpr* parse_literal();
		ast::BaseExpr* parse_variable_declaration();
		ast::BaseExpr* parse_variable_reference();
		ast::BaseExpr* parse_parenthesis();
		ast::FunctionPrototype* parse_function_prototype();
		ast::FunctionDefinition* parse_function_definition();
		int get_token_precedence();
		ast::BaseExpr* log_error(std::string error_message);
		ast::BodyExpr* log_error_body(std::string error_message);
		ast::FunctionPrototype* log_error_prototype(std::string error_message);
		void log_line_info();
	private:
		std::ifstream& input_file;
		std::string identifier_string;
		char last_char = '\0';
		Token curr_token = Token::None;
		types::Type curr_type = types::Type::None;
		std::vector<ast::BodyExpr*> bodies;
		LineInfo line_info;
	};
}
