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
		ExternFunction,
		IfStatement,
		ElseStatement,
		ForStatement,
		WhileStatement,
		ReturnStatement,
		ContinueStatement,
		BreakStatement,
		BodyStart,
		BodyEnd,
		ParenStart,
		ParenEnd,
		Comma,
		Comment,
		None,
	};

	class LineInfo
	{
	public:
		int line_count = 0;
		int line_pos_start = 0;
		int line_pos = 0;
		std::string line;
	};

	class Parser
	{
	public:
		Parser(std::ifstream& input_file, std::string file_name);

		shared_ptr<ast::BaseExpr> parse_file();
		shared_ptr<ast::FunctionDefinition> parse_file_as_func();
		shared_ptr<ast::BodyExpr> parse_file_as_body();
		static const std::unordered_map<operators::BinaryOp, int> binop_precedence;
	private:
		char get_char();
		char peek_char();
		Token get_next_token();
		shared_ptr<ast::BodyExpr> parse_body(ast::BodyType body_type, bool is_top_level, bool has_curly_brackets);
		shared_ptr<ast::FunctionDefinition> parse_top_level();
		shared_ptr<ast::BaseExpr> parse_expression(bool for_call, bool for_if_cond);
		shared_ptr<ast::BaseExpr> parse_primary();
		shared_ptr<ast::BaseExpr> parse_binop_rhs(int precedence, shared_ptr<ast::BaseExpr> lhs);
		shared_ptr<ast::BaseExpr> parse_literal();
		shared_ptr<ast::BaseExpr> parse_variable_declaration();
		shared_ptr<ast::BaseExpr> parse_variable_reference();
		shared_ptr<ast::BaseExpr> parse_parenthesis();
		ast::FunctionPrototype* parse_function_prototype();
		ast::FunctionPrototype* parse_extern();
		shared_ptr<ast::FunctionDefinition> parse_function_definition();
		shared_ptr<ast::BaseExpr> parse_if_else();
		shared_ptr<ast::BaseExpr> parse_for_loop();
		shared_ptr<ast::BaseExpr> parse_while_loop();
		shared_ptr<ast::BaseExpr> parse_comment();
		shared_ptr<ast::BaseExpr> parse_return();
		shared_ptr<ast::BaseExpr> parse_continue();
		shared_ptr<ast::BaseExpr> parse_break();
		int get_token_precedence();
		shared_ptr<ast::BaseExpr> log_error(std::string error_message);
		shared_ptr<ast::BodyExpr> log_error_body(std::string error_message);
		shared_ptr<ast::FunctionPrototype> log_error_prototype(std::string error_message);
		void log_error_empty(std::string error_message);
		void log_line_info();
	private:
		std::ifstream& input_file;
		std::string identifier_string;
		char last_char = '\0';
		Token curr_token = Token::None;
		types::Type curr_type = types::Type::None;
		std::vector<ast::BodyExpr*> bodies;
		LineInfo line_info;
		std::string file_name;
	};
}
