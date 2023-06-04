#pragma once

#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

#include "../config.h"
#include "ast.h"
#include "module.h"

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
		SwitchStatement,
		CaseStatement,
		DefaultStatement,
		ModuleStatement,
		UsingStatement,
		BodyStart,
		BodyEnd,
		ParenStart,
		ParenEnd,
		AngleStart,
		AngleEnd,
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

		ptr_type<ast::BaseExpr> parse_file();
		ptr_type<ast::FunctionDefinition> parse_file_as_func();
		ptr_type<ast::BodyExpr> parse_file_as_body();
		int get_module();
		static const std::unordered_map<operators::BinaryOp, int> binop_precedence;
	private:
		char get_char();
		char peek_char();
		void skip_char();
		Token peek_next_token();
		Token get_next_token();
		ptr_type<ast::BodyExpr> parse_body(ast::BodyType body_type, bool is_top_level, bool has_curly_brackets);
		ptr_type<ast::FunctionDefinition> parse_top_level();
		ptr_type<ast::BaseExpr> parse_expression(bool for_call, bool middle_expression);
		ptr_type<ast::BaseExpr> parse_primary();
		ptr_type<ast::BaseExpr> parse_binop_rhs(int precedence, ptr_type<ast::BaseExpr> lhs);
		ptr_type<ast::BaseExpr> parse_unary();
		ptr_type<ast::BaseExpr> parse_literal();
		ptr_type<ast::BaseExpr> parse_variable_declaration();
		ptr_type<ast::BaseExpr> parse_variable_reference();
		ptr_type<ast::BaseExpr> parse_parenthesis();
		ast::FunctionPrototype* parse_function_prototype();
		ast::FunctionPrototype* parse_extern();
		ptr_type<ast::FunctionDefinition> parse_function_definition();
		ptr_type<ast::BaseExpr> parse_if_else();
		ptr_type<ast::BaseExpr> parse_for_loop();
		ptr_type<ast::BaseExpr> parse_while_loop();
		ptr_type<ast::BaseExpr> parse_comment();
		ptr_type<ast::BaseExpr> parse_return();
		ptr_type<ast::BaseExpr> parse_continue();
		ptr_type<ast::BaseExpr> parse_break();
		ptr_type<ast::BaseExpr> parse_cast(ptr_type<ast::BaseExpr> expr);
		ptr_type<ast::BaseExpr> parse_switch_case();
		bool parse_module();
		bool parse_using();
		int get_token_precedence();
		void update_current_module();
		ptr_type<ast::BaseExpr> log_error(std::string error_message);
		ptr_type<ast::BodyExpr> log_error_body(std::string error_message);
		ptr_type<ast::FunctionPrototype> log_error_prototype(std::string error_message);
		void log_error_empty(std::string error_message);
		void log_line_info();
	private:
		std::ifstream& input_file;
		std::string identifier_string;
		char last_char = '\0';
		Token curr_token = Token::None;
		types::Type curr_type = types::TypeEnum::None;
		std::vector<ast::BodyExpr*> bodies;
		LineInfo line_info;
		std::string file_name;
		int filename_id;
		int current_module = -1;
		bool finished_parsing_modules = false;
		std::unordered_set<int> using_modules;
	};
}
