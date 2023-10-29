#include "parser.h"

#include "../utils.h"
#include "mangler.h"
#include "module_manager.h"
#include "string_manager.h"

#include <iostream>

namespace parser
{
	// clang-format off
	// the operator precedences, higher binds tighter, same binds to the left
	const std::unordered_map<operators::BinaryOp, int> Parser::binop_precedence = {
		{operators::BinaryOp::Assignment,                10},
		{operators::BinaryOp::AssignmentAddition,        10},
		{operators::BinaryOp::AssignmentSubtraction,     10},
		{operators::BinaryOp::AssignmentMultiplication,  10},
		{operators::BinaryOp::AssignmentModulo,          10},
		{operators::BinaryOp::AssignmentDivision,        10},
		{operators::BinaryOp::AssignmentBitwiseAnd,      10},
		{operators::BinaryOp::AssignmentBitwiseOr,       10},
		{operators::BinaryOp::AssignmentBitwiseXor,      10},
		{operators::BinaryOp::BooleanOr,                 20},
		{operators::BinaryOp::BooleanAnd,                30},
		{operators::BinaryOp::BitwiseOr,                 40},
		{operators::BinaryOp::BitwiseXor,                50},
		{operators::BinaryOp::BitwiseAnd,                60},
		{operators::BinaryOp::EqualTo,                   70},
		{operators::BinaryOp::NotEqualTo,                80},
		{operators::BinaryOp::LessThan,                 100},
		{operators::BinaryOp::LessThanEqual,            100},
		{operators::BinaryOp::GreaterThan,              100},
		{operators::BinaryOp::GreaterThanEqual,         100},
		{operators::BinaryOp::BitwiseShiftLeft,         110},
		{operators::BinaryOp::BitwiseShiftRight,        110},
		{operators::BinaryOp::Addition,                 120},
		{operators::BinaryOp::Subtraction,              120},
		{operators::BinaryOp::Multiplication,           140},
		{operators::BinaryOp::Division,                 140},
		{operators::BinaryOp::Modulo,                   140},
		{operators::BinaryOp::ModuleScope,              200},
	};
	// clang-format on

	Parser::Parser(std::ifstream& input_file, const std::string& file_name) :
		input_file(input_file),
		file_name(file_name)
	{
		filename_id = moduleManager::get_file_as_module(file_name);
	}

	ptr_type<ast::BaseExpr> Parser::parse_file()
	{
		ptr_type<ast::BaseExpr> file_body = parse_body(ast::BodyType::Global, true, false);

		return file_body;
	}

	ptr_type<ast::FunctionDefinition> Parser::parse_file_as_func()
	{
		ptr_type<ast::FunctionDefinition> func = parse_top_level();

		return func;
	}

	ptr_type<ast::BodyExpr> Parser::parse_file_as_body()
	{
		bodies.push_back(nullptr);
		ptr_type<ast::BodyExpr> body = parse_body(ast::BodyType::Global, true, false);
		return body;
	}

	int Parser::get_module()
	{
		return this->filename_id;
	}

	char Parser::get_char()
	{
		char c = input_file.get();
		if (c == '\n')
		{
			// reset the line
			line_info.line = "";
			// increment the line number
			line_info.line_count++;
			// reset line pos
			line_info.line_pos = 0;
		}
		else if (c == '\r')
		{
			// ignore
		}
		else if (c == '\t')
		{
			// convert tab to space
			line_info.line += ' ';
			// increment line pos
			line_info.line_pos++;
		}
		else
		{
			// add char to line
			line_info.line += c;
			// increment line pos
			line_info.line_pos++;
		}

		return c;
	}

	char Parser::peek_char()
	{
		return input_file.peek();
	}

	void Parser::skip_char()
	{
		last_char = get_char();
		line_info.line_pos_start = line_info.line_pos;
		identifier_string = last_char;
		curr_token = Token::None;
	}

	Token Parser::peek_next_token()
	{
		// store state
		char last_c = last_char;
		std::string id_str = identifier_string;
		Token curr_tok = curr_token;
		types::Type curr_ty = curr_type;
		LineInfo line_inf = line_info;
		int pos = this->input_file.tellg();

		Token next_tok = get_next_token();

		// restore state
		last_char = last_c;
		identifier_string = id_str;
		curr_token = curr_tok;
		curr_type = curr_ty;
		line_info = line_inf;
		this->input_file.seekg(pos);

		return next_tok;
	}

	Token Parser::get_next_token()
	{
		identifier_string = "";
		curr_token = Token::None;

		last_char = get_char();
		// skip whitespace
		while (std::isspace(last_char))
		{
			last_char = get_char();
		}

		line_info.line_pos_start = line_info.line_pos;

		// identifier: [a-zA-Z_][a-zA-Z0-9_]*
		if (std::isalpha(last_char) || last_char == '_')
		{
			identifier_string = last_char;
			char next_char = peek_char();
			while (std::isalnum(next_char) || next_char == '_')
			{
				last_char = get_char();
				identifier_string += last_char;
				next_char = peek_char();
			}

			if (identifier_string == "function")
			{
				curr_token = Token::FunctionDefinition;
				return Token::FunctionDefinition;
			}
			else if (identifier_string == "extern")
			{
				curr_token = Token::ExternFunction;
				return curr_token;
			}
			else if (identifier_string == "if")
			{
				curr_token = Token::IfStatement;
				return curr_token;
			}
			else if (identifier_string == "else")
			{
				curr_token = Token::ElseStatement;
				return curr_token;
			}
			else if (identifier_string == "var")
			{
				curr_token = Token::VariableDeclaration;
				return curr_token;
			}
			else if (identifier_string == "for")
			{
				curr_token = Token::ForStatement;
				return curr_token;
			}
			else if (identifier_string == "while")
			{
				curr_token = Token::WhileStatement;
				return curr_token;
			}
			else if (identifier_string == "return")
			{
				curr_token = Token::ReturnStatement;
				return curr_token;
			}
			else if (identifier_string == "continue")
			{
				curr_token = Token::ContinueStatement;
				return curr_token;
			}
			else if (identifier_string == "break")
			{
				curr_token = Token::BreakStatement;
				return curr_token;
			}
			else if (identifier_string == "module")
			{
				curr_token = Token::ModuleStatement;
				return curr_token;
			}
			else if (identifier_string == "using")
			{
				curr_token = Token::UsingStatement;
				return curr_token;
			}
			else if (this->identifier_string == "switch")
			{
				this->curr_token = Token::SwitchStatement;
				return this->curr_token;
			}
			else if (this->identifier_string == "case")
			{
				this->curr_token = Token::CaseStatement;
				return this->curr_token;
			}
			else if (this->identifier_string == "default")
			{
				this->curr_token = Token::DefaultStatement;
				return this->curr_token;
			}
			else
			{
				// check if identifier is a bool
				std::pair<bool, types::Type> res = types::check_type_string(identifier_string);
				if (res.first && res.second.get_type_enum() == types::TypeEnum::Bool)
				{
					curr_type = res.second;
					curr_token = Token::LiteralValue;
					return curr_token;
				}

				curr_token = Token::VariableReference;
				return Token::VariableReference;
			}
		}

		// literal:
		//		int: [0-9][0-9]*((i|u)(8|16|32|64)?)?
		//		float: ([0-9][0-9]*)[.]([0-9][0-9]*)(f(32|64)?)?
		if (std::isdigit(last_char))
		{
			identifier_string = last_char;
			char next_char = peek_char();
			while (std::isdigit(next_char) || next_char == '.' || next_char == 'f' || next_char == 'i' ||
				   next_char == 'u')
			{
				last_char = get_char();
				identifier_string += last_char;
				next_char = peek_char();
			}

			std::pair<bool, types::Type> res = types::check_type_string(identifier_string);

			if (res.first)
			{
				curr_type = res.second;
				curr_token = Token::LiteralValue;
				return curr_token;
			}
		}

		// literal:
		//		char: '.'
		if (last_char == '\'')
		{
			identifier_string = last_char;
			char next_char = get_char();
			while (next_char != '\'' || identifier_string.back() == '\\')
			{
				last_char = next_char;
				identifier_string += last_char;
				next_char = get_char();
			}

			last_char = next_char;
			identifier_string += last_char;

			std::pair<bool, types::Type> res = types::check_type_string(identifier_string);

			if (res.first)
			{
				curr_type = res.second;
				curr_token = Token::LiteralValue;
				return curr_token;
			}
		}

		// end of file
		if (last_char == std::ifstream::traits_type::eof())
		{
			curr_token = Token::EndOfFile;
			return curr_token;
		}

		// end of expression
		if (last_char == ';')
		{
			curr_token = Token::EndOfExpression;
			return curr_token;
		}

		// start of body
		if (last_char == '{')
		{
			curr_token = Token::BodyStart;
			return curr_token;
		}

		// end of body
		if (last_char == '}')
		{
			curr_token = Token::BodyEnd;
			return curr_token;
		}

		// paren start
		if (last_char == '(')
		{
			curr_token = Token::ParenStart;
			return curr_token;
		}

		// paren end
		if (last_char == ')')
		{
			curr_token = Token::ParenEnd;
			return curr_token;
		}

		// binary operator
		if (operators::is_first_char_valid(last_char))
		{
			std::string chars;
			chars += last_char;
			char next_char = peek_char();
			if (operators::is_second_char_valid(next_char))
			{
				chars += next_char;
			}

			if (operators::is_binary_op(chars) != operators::BinaryOp::None)
			{
				if (chars.length() == 2)
				{
					last_char = get_char();
				}
				identifier_string = chars;
				last_char = chars.back();
				curr_token = Token::BinaryOperator;
				return curr_token;
			}
		}

		// angle start
		if (last_char == '<')
		{
			curr_token = Token::AngleStart;
			return curr_token;
		}

		// angle end
		if (last_char == '>')
		{
			curr_token = Token::AngleEnd;
			return curr_token;
		}

		// comma
		if (last_char == ',')
		{
			curr_token = Token::Comma;
			return curr_token;
		}

		if (last_char == '#')
		{
			curr_token = Token::Comment;
			return curr_token;
		}

		// std::cout << "identifier " << identifier_string << " " << last_char << std::endl;

		// anything else
		curr_token = Token::None;
		return curr_token;
	}

	/// top ::= (definition | expression)*
	ptr_type<ast::BodyExpr> Parser::parse_body(ast::BodyType body_type, bool is_top_level, bool has_curly_brackets)
	{
		ptr_type<ast::BodyExpr> body = make_ptr<ast::BodyExpr>(bodies.back(), body_type);

		bool parse_success = this->parse_body_using_existing(body, is_top_level, has_curly_brackets);
		if (parse_success)
		{
			return body;
		}
		else
		{
			return nullptr;
		}
	}

	/// top ::= (definition | expression)*
	bool Parser::parse_body_using_existing(ptr_type<ast::BodyExpr>& body, bool is_top_level, bool has_curly_brackets)
	{
		if (has_curly_brackets)
		{
			if (curr_token != Token::BodyStart)
			{
				return log_error_bool("Body must start with a '{'");
			}
		}

		bodies.push_back(body.get());
		ScopeExit scope_exit([this]() { this->bodies.pop_back(); });

		curr_token = get_next_token();

		while (true)
		{
			/*curr_token = get_next_token();*/
			switch (curr_token)
			{
				case Token::EndOfFile:
				{
					if (has_curly_brackets)
					{
						return log_error_bool("Body must end with a '}'");
					}
					return true;
				}
				case Token::EndOfExpression:
				{
					// skip
					break;
				}
				case Token::VariableDeclaration:
				{
					if (is_top_level)
					{
						return log_error_bool("Expressions are not allowed in top level code");
					}

					ptr_type<ast::BaseExpr> base = parse_variable_declaration();

					if (base == nullptr)
					{
						return false;
					}

					body->add_base(std::move(base));

					break;
				}
				case Token::FunctionDefinition:
				{
					if (this->finished_parsing_modules == false)
					{
						this->update_current_module();
					}

					if (!is_top_level)
					{
						return log_error_bool("Nested Functions are currently disabled.");
					}

					ptr_type<ast::FunctionDefinition> fd = parse_function_definition();
					if (fd != nullptr)
					{
						body->add_function(std::move(fd));
					}
					else
					{
						return false;
					}

					break;
				}
				case Token::ExternFunction:
				{
					if (this->finished_parsing_modules == false)
					{
						this->update_current_module();
					}

					ast::FunctionPrototype* proto = parse_extern();

					if (proto == nullptr)
					{
						return false;
					}

					body->add_prototype(proto);
					break;
				}
				case Token::IfStatement:
				{
					if (is_top_level)
					{
						return log_error_bool("If statements are not allowed in top level code");
					}

					ptr_type<ast::BaseExpr> base = parse_if_else(false);

					if (base == nullptr)
					{
						return false;
					}

					ast::IfExpr* if_expr = dynamic_cast<ast::IfExpr*>(base.get());
					if_expr->should_return_value = false;

					body->add_base(std::move(base));

					break;
				}
				case Token::SwitchStatement:
				{
					if (is_top_level)
					{
						return log_error_bool("Switch statements are not allowed in top level code");
					}

					ptr_type<ast::BaseExpr> base = parse_switch_case();

					if (base == nullptr)
					{
						return false;
					}

					body->add_base(std::move(base));

					break;
				}
				case Token::ForStatement:
				{
					if (is_top_level)
					{
						return log_error_bool("For statements are not allowed in top level code");
					}

					ptr_type<ast::BaseExpr> base = parse_for_loop();

					if (base == nullptr)
					{
						return false;
					}

					body->add_base(std::move(base));

					break;
				}
				case Token::WhileStatement:
				{
					if (is_top_level)
					{
						return log_error_bool("While statements are not allowed in top level code");
					}

					ptr_type<ast::BaseExpr> base = parse_while_loop();

					if (base == nullptr)
					{
						return false;
					}

					body->add_base(std::move(base));

					break;
				}
				case Token::Comment:
				{
					parse_comment();
					break;
				}
				case Token::ReturnStatement:
				{
					if (is_top_level)
					{
						return log_error_bool("Return statements are not allowed in top level code");
					}

					ptr_type<ast::BaseExpr> base = parse_return();

					if (base == nullptr)
					{
						return false;
					}

					body->add_base(std::move(base));

					break;
				}
				case Token::ContinueStatement:
				{
					if (is_top_level)
					{
						return log_error_bool("Continue statements are not allowed in top level code");
					}

					ptr_type<ast::BaseExpr> base = parse_continue();

					if (base == nullptr)
					{
						return false;
					}

					body->add_base(std::move(base));

					break;
				}
				case Token::BreakStatement:
				{
					if (is_top_level)
					{
						return log_error_bool("Break statements are not allowed in top level code");
					}

					ptr_type<ast::BaseExpr> base = parse_break();

					if (base == nullptr)
					{
						return false;
					}

					body->add_base(std::move(base));

					break;
				}
				case Token::ModuleStatement:
				{
					if (!is_top_level)
					{
						return log_error_bool("Module Definition can only be in top level code");
					}

					if (this->finished_parsing_modules)
					{
						return log_error_bool("Module Definitions must be before any other code");
					}

					if (!parse_module())
					{
						return false;
					}

					break;
				}
				case Token::UsingStatement:
				{
					if (!is_top_level)
					{
						return log_error_bool("Using Statement can only be in top level code");
					}

					if (this->finished_parsing_modules)
					{
						return log_error_bool("Using Statements must be before any other code");
					}

					if (!parse_using())
					{
						return false;
					}

					break;
				}
				case Token::BodyStart:
				{
					// Body expressions used to create scope
					if (is_top_level)
					{
						return log_error_bool("Scope blocks are not allowed in top level code");
					}

					ptr_type<ast::BaseExpr> base = parse_body(ast::BodyType::ScopeBlock, false, true);

					if (base == nullptr)
					{
						return false;
					}

					body->add_base(std::move(base));

					// TODO: why is this really needed, should parse_body consume it?
					get_next_token();

					break;
				}
				case Token::BodyEnd:
				{
					return true;
				}
				default:
				{
					if (is_top_level)
					{
						return log_error_bool("Expressions are not allowed in top level code");
					}

					ptr_type<ast::BaseExpr> base = parse_expression(false, false);
					if (base == nullptr)
					{
						return false;
					}

					body->add_base(std::move(base));

					if (curr_token != Token::EndOfExpression)
					{
						return log_error_bool("Expected end of expression, missing ';'");
					}

					break;
				}
			}

			if (curr_token == Token::EndOfExpression)
			{
				curr_token = get_next_token();
			}
		}

		return false;
	}

	ptr_type<ast::FunctionDefinition> Parser::parse_top_level()
	{
		bodies.push_back(nullptr);
		ptr_type<ast::BodyExpr> expr = parse_body(ast::BodyType::Global, true, false);
		if (expr == nullptr)
		{
			return nullptr;
		}

		std::string name = "_";
		std::vector<types::Type> types;
		std::vector<int> arg_ids;

		ast::FunctionPrototype* proto =
			new ast::FunctionPrototype(name, types::Type{ types::TypeEnum::Void }, types, arg_ids);
		return make_ptr<ast::FunctionDefinition>(proto, std::move(expr));
	}

	/// expression
	///   ::= primary binop_rhs
	ptr_type<ast::BaseExpr> Parser::parse_expression(bool for_call, bool middle_expression)
	{
		ptr_type<ast::BaseExpr> lhs = parse_unary();
		if (lhs == nullptr)
		{
			return nullptr;
		}

		if (curr_token == Token::EndOfExpression)
		{
			return lhs;
		}

		if (curr_token == Token::BinaryOperator)
		{
			return parse_binop_rhs(0, std::move(lhs));
		}

		if (for_call)
		{
			if (curr_token == Token::Comma)
			{
				return lhs;
			}

			if (curr_token == Token::ParenEnd)
			{
				return lhs;
			}
		}

		if (middle_expression)
		{
			return lhs;
		}

		return log_error("Expected end of expression, missing ';'");
	}

	/// primary
	///   ::= variable_declaration_expr
	///   ::= variable_reference_expr
	///   ::= literal_expr
	///   ::= parenthesis_expr
	ptr_type<ast::BaseExpr> Parser::parse_primary()
	{
		ptr_type<ast::BaseExpr> expr;

		switch (curr_token)
		{
			case Token::VariableReference:
			{
				expr = parse_variable_reference();
				break;
			}
			case Token::LiteralValue:
			{
				expr = parse_literal();
				break;
			}
			case Token::ParenStart:
			{
				expr = parse_parenthesis();
				break;
			}
			case Token::IfStatement:
			{
				ptr_type<ast::BaseExpr> expr = parse_if_else(true);

				if (expr != nullptr)
				{
					ast::IfExpr* if_expr = dynamic_cast<ast::IfExpr*>(expr.get());
					if (if_expr != nullptr && if_expr->should_return_value == false)
					{
						return log_error("If expression must have an else statement");
					}
				}

				return expr;
			}
			default:
			{
				return log_error("Unknown token when expecting an expression");
			}
		}

		if (expr == nullptr)
		{
			return nullptr;
		}

		// allows cast to be chained
		while (true)
		{
			char next_char = peek_char();

			get_next_token();

			if (next_char == '<')
			{
				// parse cast
				expr = parse_cast(std::move(expr));
			}
			else
			{
				break;
			}
		}

		return expr;
	}

	/// binop_rhs
	///   ::= ('+' primary)*
	ptr_type<ast::BaseExpr> Parser::parse_binop_rhs(int expr_precedence, ptr_type<ast::BaseExpr> lhs)
	{
		while (true)
		{
			int token_precedence = get_token_precedence();

			// std::cout << "token p " << token_precedence << " exrp p " << expr_precedence << std::endl;

			// If this is a binop that binds at least as tightly as the current binop,
			// consume it, otherwise we are done.
			if (token_precedence < expr_precedence)
			{
				// std::cout << "ealry end" << std::endl;
				return std::move(lhs);
			}

			operators::BinaryOp binop_type = operators::is_binary_op(identifier_string);
			if (binop_type == operators::BinaryOp::None)
			{
				return log_error("Binary operator is invalid");
			}

			get_next_token();

			ptr_type<ast::BaseExpr> rhs = parse_unary();

			if (rhs == nullptr)
			{
				return nullptr;
			}

			// std::cout << "rhs type " << (int) rhs->get_type() << std::endl;

			// get_next_token();

			// If BinOp binds less tightly with RHS than the operator after RHS, let
			// the pending operator take RHS as its LHS.
			int next_precedence = get_token_precedence();

			// std::cout << "next token " << last_char << " next p " << next_precedence << std::endl;

			if (token_precedence < next_precedence)
			{
				// std::cout << "parse right first" << std::endl;
				rhs = parse_binop_rhs(token_precedence + 1, std::move(rhs));
				if (rhs == nullptr)
				{
					return nullptr;
				}
			}

			lhs = std::move(make_ptr<ast::BinaryExpr>(bodies.back(), binop_type, std::move(lhs), std::move(rhs)));
		}

		return nullptr;
	}

	/// unary_expr
	///   ::= primary
	///   ::= unop expression
	ptr_type<ast::BaseExpr> Parser::parse_unary()
	{
		// if current char is not a unary op, then it is a primary expression
		if (operators::is_unary_op(last_char) == operators::UnaryOp::None)
		{
			return parse_primary();
		}

		// get the unary operator
		operators::UnaryOp unop = operators::is_unary_op(last_char);

		get_next_token();

		ptr_type<ast::BaseExpr> expr = parse_unary();
		if (!expr)
		{
			return nullptr;
		}

		return make_ptr<ast::UnaryExpr>(bodies.back(), unop, std::move(expr));
	}

	/// literal_expr ::= 'curr_type'
	ptr_type<ast::BaseExpr> Parser::parse_literal()
	{
		if (curr_type.get_type_enum() == types::TypeEnum::None)
		{
			return log_error("Literal is not a valid type");
		}

		std::string literal_string = identifier_string;

		// check if literal if whithin the correct range
		// TODO: not ignore float
		if (!types::check_range(literal_string, curr_type))
		{
			return log_error("Literal value for type: " + curr_type.to_string() + " is out of range");
		}

		// next token will be eaten in parse_primary

		return make_ptr<ast::LiteralExpr>(bodies.back(), curr_type, literal_string);
	}

	/// variable_declaration_expr ::= var 'curr_type' identifier ('=' expression)?
	ptr_type<ast::BaseExpr> Parser::parse_variable_declaration()
	{
		get_next_token();

		types::Type var_type = types::is_valid_type(identifier_string);

		if (var_type.get_type_enum() == types::TypeEnum::None)
		{
			return log_error("Invalid type specified");
		}

		get_next_token();

		if (curr_token != Token::VariableReference)
		{
			return log_error("Expected identifier after type");
		}

		std::string name = identifier_string;
		get_next_token();

		ptr_type<ast::BaseExpr> expr = nullptr;

		if (last_char == '=')
		{
			get_next_token();

			expr = parse_expression(false, false);
			if (expr == nullptr)
			{
				return nullptr;
			}
		}

		int name_id = stringManager::get_id(name);

		expr->get_body()->named_types[name_id] = var_type;
		return make_ptr<ast::VariableDeclarationExpr>(bodies.back(), var_type, name_id, std::move(expr));
	}

	/// variable_reference_expr
	///   ::= identifier
	///   ::= identifier '(' expression* ')'
	ptr_type<ast::BaseExpr> Parser::parse_variable_reference()
	{
		int name_id = stringManager::get_id(identifier_string);

		// peek at next token
		Token next_token = peek_next_token();

		// simple variable reference
		if (next_token != Token::ParenStart)
		{
			// next token will be eaten in parse_primary
			return make_ptr<ast::VariableReferenceExpr>(bodies.back(), name_id);
		}

		// eat '(' token
		get_next_token();

		get_next_token();

		// function call
		std::vector<ptr_type<ast::BaseExpr>> args;
		if (curr_token != Token::ParenEnd)
		{
			while (true)
			{
				ptr_type<ast::BaseExpr> arg = parse_expression(true, false);
				if (arg != nullptr)
				{
					args.push_back(std::move(arg));
				}
				else
				{
					return nullptr;
				}

				if (curr_token == Token::ParenEnd)
				{
					break;
				}

				if (curr_token != Token::Comma)
				{
					return log_error("Expected ')' or ',' in function argument list");
				}

				get_next_token();
			}
		}

		// next token will be eaten in parse_primary

		return make_ptr<ast::CallExpr>(bodies.back(), name_id, args);
	}

	ptr_type<ast::BaseExpr> Parser::parse_parenthesis()
	{
		get_next_token();

		ptr_type<ast::BaseExpr> expr = parse_expression(false, true);

		if (curr_token != Token::ParenEnd)
		{
			return log_error("Expected ')' after expression");
		}

		if (expr == nullptr)
		{
			return nullptr;
		}

		// next token will be eaten in parse_primary

		return expr;
	}

	/// prototype
	///   ::= type id '(' [type id,]* ')'
	ast::FunctionPrototype* Parser::parse_function_prototype()
	{
		if (curr_token != Token::VariableReference)
		{
			log_error_empty("Return type for function prototype is invalid");
			return nullptr;
		}

		types::Type return_type = types::is_valid_type(identifier_string);

		if (return_type.get_type_enum() == types::TypeEnum::None)
		{
			log_error_empty("Return type for function prototype is invalid");
			return nullptr;
		}

		get_next_token();

		if (curr_token != Token::VariableReference)
		{
			log_error_empty("Expected function name in function prototype");
			return nullptr;
		}

		std::string name = identifier_string;

		get_next_token();

		if (curr_token != Token::ParenStart)
		{
			log_error_empty("Expected '(' in function prototype");
			return nullptr;
		}

		std::vector<types::Type> types;
		std::vector<int> args;

		get_next_token();

		if (curr_token != Token::ParenEnd)
		{
			while (true)
			{
				if (curr_token != Token::VariableReference)
				{
					log_error_empty("Expected variable declaration in function prototype");
					return nullptr;
				}

				types.push_back(types::is_valid_type(identifier_string));

				get_next_token();
				if (curr_token != Token::VariableReference)
				{
					log_error_empty("Expected a variable identifier in function prototype");
					return nullptr;
				}

				args.push_back(stringManager::get_id(identifier_string));

				get_next_token();

				if (curr_token == Token::ParenEnd)
				{
					break;
				}
				else
				{
					if (curr_token == Token::Comma)
					{
						get_next_token();
						continue;
					}
					else
					{
						log_error_empty("Invalid character in function prototype");
						return nullptr;
					}
				}
			}
		}

		// get_next_token();

		return new ast::FunctionPrototype(name, return_type, types, args);
	}

	/// external ::= 'extern' prototype
	ast::FunctionPrototype* Parser::parse_extern()
	{
		get_next_token();

		ast::FunctionPrototype* proto = parse_function_prototype();

		if (proto == nullptr)
		{
			return nullptr;
		}

		if (curr_token == Token::EndOfExpression)
		{
			log_error_empty("Expected end of expression, missing ';'");
			return nullptr;
		}

		get_next_token();

		proto->is_extern = true;

		return proto;
	}

	/// definition ::= 'function' prototype expression
	ptr_type<ast::FunctionDefinition> Parser::parse_function_definition()
	{
		get_next_token();

		ast::FunctionPrototype* proto = parse_function_prototype();
		if (proto == nullptr)
		{
			return nullptr;
		}

		get_next_token();

		ptr_type<ast::BodyExpr> body = parse_body(ast::BodyType::Function, false, true);
		if (body == nullptr)
		{
			delete proto;
			return nullptr;
		}

		for (int i = 0; i < proto->args.size(); i++)
		{
			body->named_types[proto->args[i]] = proto->types[i];
		}

		body->parent_function = proto;

		get_next_token();

		return make_ptr<ast::FunctionDefinition>(proto, std::move(body));
	}

	// ifexpr ::= 'if' '(' condition ')' '{' expression* '}' ('else' 'if' '(' condition ')' '{' expression* '}')* (else
	// '{' expression* '}')?
	ptr_type<ast::BaseExpr> Parser::parse_if_else(bool should_return_value)
	{
		get_next_token();

		ptr_type<ast::BaseExpr> if_cond = parse_expression(false, true);

		if (if_cond == nullptr)
		{
			return log_error("Expected if condition");
		}

		ptr_type<ast::BodyExpr> if_body = parse_body(ast::BodyType::Conditional, false, true);

		if (if_body == nullptr)
		{
			return log_error("Expected if body");
		}

		get_next_token();

		std::vector<ptr_type<ast::BaseExpr>> if_conditions;
		std::vector<ptr_type<ast::BaseExpr>> if_bodies;

		if_conditions.push_back(std::move(if_cond));
		if_bodies.push_back(std::move(if_body));

		ptr_type<ast::BaseExpr> else_body = nullptr;

		while (this->curr_token == Token::ElseStatement)
		{
			get_next_token();

			if (this->curr_token != Token::IfStatement)
			{
				if (this->curr_token == Token::BodyStart)
				{
					else_body = parse_body(ast::BodyType::Conditional, false, true);
				}

				if (else_body == nullptr)
				{
					return log_error("Expected else body");
				}

				get_next_token();

				break;
			}

			get_next_token();

			ptr_type<ast::BaseExpr> else_if_cond = parse_expression(false, true);

			if (else_if_cond == nullptr)
			{
				return log_error("Expected if condition");
			}

			ptr_type<ast::BodyExpr> else_if_body = parse_body(ast::BodyType::Conditional, false, true);

			if (else_if_body == nullptr)
			{
				return log_error("Expected if body");
			}

			get_next_token();

			if_conditions.push_back(std::move(else_if_cond));
			if_bodies.push_back(std::move(else_if_body));
		}

		if (else_body == nullptr)
		{
			should_return_value = false;
		}

		ptr_type<ast::BaseExpr> previous_if_expr = nullptr;
		for (int i = if_conditions.size() - 1; i >= 0; i--)
		{
			if (previous_if_expr != nullptr)
			{
				previous_if_expr = make_ptr<ast::IfExpr>(
					bodies.back(),
					std::move(if_conditions[i]),
					std::move(if_bodies[i]),
					std::move(previous_if_expr),
					should_return_value);
			}
			else
			{
				previous_if_expr = make_ptr<ast::IfExpr>(
					bodies.back(),
					std::move(if_conditions[i]),
					std::move(if_bodies[i]),
					std::move(else_body),
					should_return_value);
			}
		}

		return previous_if_expr;
	}

	/// forexpr ::= 'for' 'type' identifier '=' expr ';' expr (';' expr)? '{' expression* '}'
	ptr_type<ast::BaseExpr> Parser::parse_for_loop()
	{
		ptr_type<ast::BodyExpr> for_loop_body = make_ptr<ast::BodyExpr>(this->bodies.back(), ast::BodyType::Loop);
		this->bodies.push_back(for_loop_body.get());

		get_next_token();

		bool uses_parenthesis = false;
		if (this->curr_token == Token::ParenStart)
		{
			uses_parenthesis = true;
			get_next_token();
		}

		if (curr_token != Token::VariableReference)
		{
			return log_error("Expected type after for");
		}

		types::Type var_type = types::is_valid_type(identifier_string);

		if (var_type.get_type_enum() == types::TypeEnum::None)
		{
			return log_error("Invalid type after for");
		}

		get_next_token();

		if (curr_token != Token::VariableReference)
		{
			return log_error("Expected identifier after type");
		}

		int name_id = stringManager::get_id(identifier_string);

		get_next_token();

		if (last_char != '=')
		{
			return log_error("Expected '=' after identifier");
		}

		get_next_token();

		ptr_type<ast::BaseExpr> start_expr = parse_expression(false, false);

		if (start_expr == nullptr)
		{
			return nullptr;
		}

		get_next_token();

		ptr_type<ast::BaseExpr> end_expr = parse_expression(false, false);

		if (end_expr == nullptr)
		{
			return nullptr;
		}

		get_next_token();

		ptr_type<ast::BaseExpr> step_expr;

		// optional step expression
		if (curr_token != Token::BodyStart)
		{
			step_expr = parse_expression(false, false);

			if (step_expr == nullptr)
			{
				return nullptr;
			}
		}

		if (uses_parenthesis)
		{
			if (this->curr_token == Token::ParenEnd)
			{
				get_next_token();
			}
			else
			{
				return log_error("Expected ')' after statement");
			}
		}

		this->bodies.pop_back();

		bool parse_success = this->parse_body_using_existing(for_loop_body, false, true);

		if (!parse_success)
		{
			return nullptr;
		}

		get_next_token();

		start_expr->get_body()->named_types[name_id] = var_type;

		return make_ptr<ast::ForExpr>(
			bodies.back(),
			var_type,
			name_id,
			std::move(start_expr),
			std::move(end_expr),
			std::move(step_expr),
			std::move(for_loop_body));
	}

	/// whileexpr ::= 'while' expr '{' expression* '}'
	ptr_type<ast::BaseExpr> Parser::parse_while_loop()
	{
		get_next_token();

		ptr_type<ast::BaseExpr> end_expr = parse_expression(false, true);

		if (end_expr == nullptr)
		{
			return nullptr;
		}

		// get_next_token();

		ptr_type<ast::BodyExpr> while_body = parse_body(ast::BodyType::Loop, false, true);

		if (while_body == nullptr)
		{
			return nullptr;
		}

		get_next_token();

		return make_ptr<ast::WhileExpr>(bodies.back(), std::move(end_expr), std::move(while_body));
	}

	ptr_type<ast::BaseExpr> Parser::parse_comment()
	{
		// skip till end of line
		while (last_char != '\n')
		{
			last_char = get_char();
		}

		curr_token = Token::EndOfExpression;

		return make_ptr<ast::CommentExpr>(bodies.back());
	}

	/// returnexpr ::= 'return' expr?
	ptr_type<ast::BaseExpr> Parser::parse_return()
	{
		get_next_token();

		ptr_type<ast::BaseExpr> expr;

		if (curr_token != Token::EndOfExpression)
		{
			expr = parse_expression(false, false);

			if (expr == nullptr)
			{
				return log_error("Return statement expected expression");
			}
		}

		return make_ptr<ast::ReturnExpr>(bodies.back(), std::move(expr));
	}

	/// continueexpr ::= 'continue'
	ptr_type<ast::BaseExpr> Parser::parse_continue()
	{
		get_next_token();

		return make_ptr<ast::ContinueExpr>(bodies.back());
	}

	/// breakexpr ::= 'break'
	ptr_type<ast::BaseExpr> Parser::parse_break()
	{
		get_next_token();

		return make_ptr<ast::BreakExpr>(bodies.back());
	}

	/// cast_expr ::= expression<type>
	ptr_type<ast::BaseExpr> Parser::parse_cast(ptr_type<ast::BaseExpr> expr)
	{
		char next_char = peek_char();

		if (!std::isalpha(next_char))
		{
			skip_char(); // eat '<'
			return log_error("Cast Expression expected a type specifier after '<'");
		}

		get_next_token();

		if (curr_token != Token::VariableReference)
		{
			return log_error("Cast Expression expected a type specifier after '<'");
		}

		int type_id = stringManager::get_id(identifier_string);

		next_char = peek_char();

		skip_char(); // eat '>'

		if (next_char != '>')
		{
			return log_error("Cast Expression expected '>' after type specifier");
		}

		// next token will be eaten in parse_primary

		return make_ptr<ast::CastExpr>(bodies.back(), type_id, std::move(expr));
	}

	/// switchexpr ::= 'switch' '{' ('case:' body)* ('default:' body)? '}'
	ptr_type<ast::BaseExpr> Parser::parse_switch_case()
	{
		get_next_token();

		ptr_type<ast::BaseExpr> value_expr = parse_expression(false, true);

		if (value_expr == nullptr)
		{
			return log_error("Expected Switch value expression");
		}

		if (this->curr_token != Token::BodyStart)
		{
			return log_error("Expected switch body start");
		}

		get_next_token();

		std::vector<ptr_type<ast::CaseExpr>> cases;

		bool has_default_case = false;

		while (this->curr_token != Token::BodyEnd)
		{
			if (this->curr_token != Token::CaseStatement && this->curr_token != Token::DefaultStatement)
			{
				return log_error("Expected case or default");
			}

			ptr_type<ast::BaseExpr> value_expr = nullptr;

			bool is_default_case = this->curr_token == Token::DefaultStatement;

			if (is_default_case)
			{
				if (has_default_case)
				{
					return log_error("Switch cannot have multiple default cases");
				}

				has_default_case = true;

				// TODO: temp use empty Expression
				value_expr = make_ptr<ast::CommentExpr>(bodies.back());

				get_next_token();
			}
			else
			{
				get_next_token();

				value_expr = parse_expression(false, true);

				if (value_expr == nullptr)
				{
					return log_error("Expected case value expression");
				}

				// TODO: move into type_checker to allow for constant expressions
				if (value_expr->get_type() != ast::AstExprType::LiteralExpr)
				{
					return log_error("Case values must be literal expressions");
				}
			}

			ptr_type<ast::BaseExpr> body_expr = parse_body(ast::BodyType::Case, false, true);

			if (body_expr == nullptr)
			{
				return log_error("Expected case body");
			}

			get_next_token(); // needed?

			// TODO: check if bodies.back() is correct for this use case
			cases.push_back(
				make_ptr<ast::CaseExpr>(bodies.back(), std::move(value_expr), std::move(body_expr), is_default_case));
		}

		if (this->curr_token != Token::BodyEnd)
		{
			return log_error("Expected switch body end");
		}

		get_next_token();

		return make_ptr<ast::SwitchExpr>(bodies.back(), std::move(value_expr), cases);
	}

	/// moduleexpr ::= 'module' identifier
	bool Parser::parse_module()
	{
		get_next_token();

		if (curr_token != Token::VariableReference)
		{
			log_error_empty("Invalid identifier for module statement");
			return false;
		}

		int module_id = stringManager::get_id(identifier_string);

		get_next_token();

		if (curr_token != Token::EndOfExpression)
		{
			log_error_empty("Expected end of expression, missing ';'");
			return false;
		}

		if (this->current_module == -1)
		{
			this->current_module = mangler::add_module(-1, module_id);
		}
		else
		{
			this->current_module = mangler::add_module(this->current_module, module_id);
		}

		return true;
	}

	bool Parser::parse_using()
	{
		get_next_token();

		ptr_type<ast::BaseExpr> base_expr = parse_expression(false, false);

		if (curr_token != Token::EndOfExpression)
		{
			log_error_empty("Expected end of expression, missing ';'");
			return false;
		}

		if (base_expr->get_type() != ast::AstExprType::VariableReferenceExpr &&
			base_expr->get_type() != ast::AstExprType::BinaryExpr)
		{
			log_error_empty("Unexpected expression after using statement.");
			return false;
		}

		if (base_expr->get_type() == ast::AstExprType::VariableReferenceExpr)
		{
		}
		else
		{
			ast::BinaryExpr* expr = dynamic_cast<ast::BinaryExpr*>(base_expr.get());
			// lhs will contain any other scope binops and variable references
			// but rhs will only contain call expressions

			// recursively check lhs type
			ast::BinaryExpr* scope_expr = expr;
			while (true)
			{
				ast::AstExprType lhs_type = scope_expr->lhs->get_type();

				if (lhs_type != ast::AstExprType::VariableReferenceExpr && lhs_type != ast::AstExprType::BinaryExpr)
				{
					log_error_empty(
						"Binary Operator: " + operators::to_string(scope_expr->binop) +
						", lhs must be an identifier or another scoper operator.");
					return false;
				}
				else
				{
					// check rhs type is variable refernce, if were not at the top scope binop
					if (scope_expr != expr && scope_expr->rhs->get_type() != ast::AstExprType::VariableReferenceExpr)
					{
						log_error_empty(
							"Binary Operator: " + operators::to_string(scope_expr->binop) +
							", rhs must be an identifier.");
						return false;
					}
					if (lhs_type == ast::AstExprType::BinaryExpr)
					{
						scope_expr = dynamic_cast<ast::BinaryExpr*>(scope_expr->lhs.get());
						continue;
					}
					else
					{
						// do nothing for variable reference
						break;
					}
				}
			}

			// check rhs type
			ast::AstExprType rhs_type = expr->rhs->get_type();
			if (rhs_type != ast::AstExprType::VariableReferenceExpr)
			{
				log_error_empty(
					"Binary Operator: " + operators::to_string(expr->binop) + ", rhs must be an identifier.");
				return false;
			}
		}

		// get module id
		int module_id = -1;
		if (base_expr->get_type() == ast::AstExprType::BinaryExpr)
		{
			module_id =
				mangler::add_mangled_name(-1, mangler::mangle_using(dynamic_cast<ast::BinaryExpr*>(base_expr.get())));
		}
		else
		{
			module_id = mangler::add_module(-1, dynamic_cast<ast::VariableReferenceExpr*>(base_expr.get())->name_id);
		}

		using_modules.insert(module_id);

		return true;
	}

	int Parser::get_token_precedence()
	{
		if (std::isalpha(last_char))
		{
			return -1;
		}

		operators::BinaryOp binop = operators::is_binary_op(identifier_string);

		// Make sure it's a declared binop.
		auto f = binop_precedence.find(binop);
		if (f == binop_precedence.end())
		{
			return -1;
		}
		int token_precedence = f->second;
		if (token_precedence <= 0)
		{
			return -1;
		}

		return token_precedence;
	}

	void Parser::update_current_module()
	{
		this->finished_parsing_modules = true;

		if (current_module == -1)
		{
			// use filename as first module
			// name = file<hash of filename>
			std::string name = "file";
			std::string filename = stringManager::get_string(this->filename_id);
			name += std::to_string(std::hash<std::string>{}(filename));
			current_module = mangler::add_module(-1, stringManager::get_id(name));
		}

		moduleManager::add_module(filename_id, current_module, using_modules);
	}

	ptr_type<ast::BaseExpr> Parser::log_error(const std::string& error_message) const
	{
		log_error_empty(error_message);
		return nullptr;
	}

	ptr_type<ast::BodyExpr> Parser::log_error_body(const std::string& error_message) const
	{
		log_error_empty(error_message);
		return nullptr;
	}

	ptr_type<ast::FunctionPrototype> Parser::log_error_prototype(const std::string& error_message) const
	{
		log_error_empty(error_message);
		return nullptr;
	}

	bool Parser::log_error_bool(const std::string& error_message) const
	{
		log_error_empty(error_message);
		return false;
	}

	void Parser::log_error_empty(const std::string& error_message) const
	{
		std::cout << error_message << std::endl;
		log_line_info();
	}

	void Parser::log_line_info() const
	{
		std::cout << '\t' << "File: " << stringManager::get_string(this->filename_id) << std::endl;
		std::cout << '\t' << "Current Character: " << last_char << std::endl;
		// std::cout << '\t' << "curr token: " << (int) curr_token << ", last char: " << last_char << std::endl;
		if (identifier_string != "")
		{
			std::cout << '\t' << "Identifier String: " << identifier_string << std::endl;
		}
		else
		{
		}

		std::cout << '\t' << "At Line: " << line_info.line_count << " Position: " << line_info.line_pos << std::endl;

		std::cout << '\t' << line_info.line << std::endl;
		std::cout << '\t' << std::setfill(' ') << std::setw(line_info.line_pos_start - 1) << "";
		std::cout << std::setfill('~') << std::setw(line_info.line_pos - line_info.line_pos_start + 1);
		std::cout << '^' << std::endl;
		std::cout << std::endl;
	}

}
