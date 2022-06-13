#include <iostream>

#include "parser.h"

//#define __debug

#ifdef __debug
#define start_ std::cout << "start: " << __FUNCTION__ << std::endl;
#define end_ std::cout << "end: " << __FUNCTION__ << std::endl;
#else
#define start_
#define end_
#endif

namespace parser
{
	// the operator precedences, higher binds tighter, same binds to the left
	const std::unordered_map<char, int> Parser::binop_precedence = {
		{'=', 2},
		{'+', 20},
		{'-', 20},
		{'*', 40},
	};

	Parser::Parser(std::ifstream& input_file) : input_file(input_file)
	{}

	ast::BaseExpr* Parser::parse_file()
	{
		ast::BaseExpr* file_body = parse_body(true, false);

		return file_body;
	}

	ast::FunctionDefinition* Parser::parse_file_as_func()
	{
		ast::FunctionDefinition* func = parse_top_level();

		return func;
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

		// identifier: [a-zA-Z][a-zA-Z0-9]*
		if (std::isalpha(last_char))
		{
			identifier_string = last_char;
			while (std::isalnum(peek_char()))
			{
				last_char = get_char();
				identifier_string += last_char;
			}

			types::Type type = types::is_valid_type(identifier_string);
			if (type != types::Type::None)
			{
				curr_type = type;
				curr_token = Token::VariableDeclaration;
				return curr_token;
			}
			else
			{
				if (identifier_string == "function")
				{
					curr_token = Token::FunctionDefinition;
					return Token::FunctionDefinition;
				}
				else
				{
#ifdef __debug
					std::cout << "identifier " << identifier_string << std::endl;
#endif
					curr_token = Token::VariableReference;
					return Token::VariableReference;
				}
			}
		}

		// literal:
		//		int: [+-]?[0-9][0-9]*
		//		float: [+-]?([0-9][0-9]*)[.]([0-9][0-9]*)[f]?
		if (std::isdigit(last_char) /* || types::BaseType::is_sign_char(last_char)*/)
		{
			identifier_string = last_char;
			char next_char = peek_char();
			while (std::isdigit(next_char) || types::BaseType::is_sign_char(next_char) || next_char == '.' || next_char == 'f')
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

		if (last_char == '(')
		{
			curr_token = Token::ParenStart;
			return curr_token;
		}

		if (last_char == ')')
		{
			curr_token = Token::ParenEnd;
			return curr_token;
		}

		if (ast::is_binary_op(last_char) != ast::BinaryOp::None)
		{
			curr_token = Token::BinaryOperator;
			return curr_token;
		}

		if (last_char == ',')
		{
			curr_token = Token::Comma;
			return curr_token;
		}

		//std::cout << "identifier " << identifier_string << " " << last_char << std::endl;

		// anything else
		curr_token = Token::None;
		return curr_token;
	}

	/// top ::= (definition | expression)*
	ast::BodyExpr* Parser::parse_body(bool is_top_level, bool has_curly_brackets)
	{
		start_;
		ast::BodyExpr* body = new ast::BodyExpr(bodies.back());

		if (has_curly_brackets)
		{
			get_next_token();

			if (curr_token != Token::BodyStart)
			{
				return log_error_body("body must start with a '{'");
			}
		}

		bodies.push_back(body);

		curr_token = get_next_token();

		while (true)
		{
			/*curr_token = get_next_token();*/
#ifdef __debug
			std::cout << "curr_token = " << (int) curr_token << std::endl;
#endif
			switch (curr_token)
			{
				case Token::EndOfFile:
				{
					end_;
					if (has_curly_brackets)
					{
						bodies.pop_back();
						return log_error_body("body must end with a '}'");
					}
					return body;
				}
				case Token::EndOfExpression:
				{
					// skip
#ifdef __debug
					std::cout << "end of expression" << std::endl;
#endif
					break;
				}
				case Token::FunctionDefinition:
				{
					ast::FunctionDefinition* fd = parse_function_definition();
					if (fd != nullptr)
					{
#ifdef __debug
						std::cout << "added function" << std::endl;
#endif
						body->add_function(fd);
					}
					else
					{
						//return body;
						return nullptr;
					}
					break;
				}
				case Token::BodyEnd:
				{
					end_;
					bodies.pop_back();
					return body;
					break;
				}
				default:
				{
					if (is_top_level)
					{
						end_;
						return log_error_body("expression not allowed in top level code");
					}

					ast::BaseExpr* base = parse_expression(false);
					if (base == nullptr)
					{
#ifdef __debug
						std::cout << "base was null";
#endif
						end_;
						return nullptr;
					}

#ifdef __debug
					std::cout << "added base" << std::endl;
#endif
					body->add_base(base);
					break;
				}
			}

			if (curr_token == Token::EndOfExpression)
			{
#ifdef __debug
				std::cout << "end of expression" << std::endl;
#endif
			}

			curr_token = get_next_token();
		}

		end_;
		return body;
	}

	ast::FunctionDefinition* Parser::parse_top_level()
	{
		start_;
		bodies.push_back(nullptr);
		ast::BodyExpr* expr = parse_body(true, false);
		if (expr == nullptr)
		{
			end_;
			return nullptr;
		}

		ast::FunctionPrototype* proto = new ast::FunctionPrototype("", types::Type::Void, {}, {});
		end_;
		return new ast::FunctionDefinition(proto, expr);
	}

	/// expression
	///   ::= primary binop_rhs
	ast::BaseExpr* Parser::parse_expression(bool for_call)
	{
		start_;
		ast::BaseExpr* lhs = parse_primary();
		if (lhs == nullptr)
		{
			end_;
			return nullptr;
		}

		if (curr_token == Token::EndOfExpression)
		{
			end_;
			return lhs;
		}

		if (curr_token == Token::BinaryOperator)
		{
			end_;
			return parse_binop_rhs(0, lhs);
		}

		if (for_call)
		{
			if (curr_token == Token::Comma)
			{
				end_;
				return lhs;
			}

			if (curr_token == Token::ParenEnd)
			{
				end_;
				return lhs;
			}
		}

		end_;
		return log_error("expected end of line ';'");
	}

	/// primary
	///   ::= variable_declaration_expr
	///   ::= variable_reference_expr
	///   ::= literal_expr
	///   ::= parenthesis_expr
	ast::BaseExpr* Parser::parse_primary()
	{
		start_;
		switch (curr_token)
		{
			case Token::VariableDeclaration:
			{
				end_;
				return parse_variable_declaration();
			}
			case Token::VariableReference:
			{
				end_;
				return parse_variable_reference();
			}
			case Token::LiteralValue:
			{
				end_;
				return parse_literal();
			}
			case Token::ParenStart:
			{
				end_;
				return parse_parenthesis();
			}
			default:
			{
				end_;
				return log_error("unknown token when expecting an expression");
			}
		}
		end_;
		return nullptr;
	}

	/// binop_rhs
	///   ::= ('+' primary)*
	ast::BaseExpr* Parser::parse_binop_rhs(int expr_precedence, ast::BaseExpr* lhs)
	{
		start_;

		while (true)
		{
			int token_precedence = get_token_precedence();

			//std::cout << "token p " << token_precedence << " exrp p " << expr_precedence << std::endl;

			// If this is a binop that binds at least as tightly as the current binop,
			// consume it, otherwise we are done.
			if (token_precedence < expr_precedence)
			{
				//std::cout << "ealry end" << std::endl;
				end_;
				return lhs;
			}

			char binop = last_char;
			//std::cout << "binop: " << binop << std::endl;

			ast::BinaryOp binop_type = ast::is_binary_op(binop);
			if (binop_type == ast::BinaryOp::None)
			{
				end_;
				return log_error("binary op is invalid");
			}

			get_next_token();

			ast::BaseExpr* rhs = parse_primary();

			if (rhs == nullptr)
			{
				end_;
				return nullptr;
			}

			//std::cout << "rhs type " << (int) rhs->get_type() << std::endl;

			//get_next_token();

			// If BinOp binds less tightly with RHS than the operator after RHS, let
			// the pending operator take RHS as its LHS.
			int next_precedence = get_token_precedence();

			//std::cout << "next token " << last_char << " next p " << next_precedence << std::endl;

			if (token_precedence < next_precedence)
			{
				//std::cout << "parse right first" << std::endl;
				rhs = parse_binop_rhs(token_precedence + 1, rhs);
				if (rhs == nullptr)
				{
					end_;
					return nullptr;
				}
			}

			lhs = new ast::BinaryExpr(bodies.back(), binop_type, lhs, rhs);
		}

		end_;
		return nullptr;
	}

	/// literal_expr ::= 'curr_type'
	ast::BaseExpr* Parser::parse_literal()
	{
		start_;
		if (curr_type == types::Type::None)
		{
			end_;
			return log_error("literal is of invalid type");
		}

		std::string literal_string = identifier_string;

		int a = 5;

		get_next_token();

		end_;
		return new ast::LiteralExpr(bodies.back(), curr_type, literal_string);
	}

	/// variable_declaration_expr ::= 'curr_type' identifier ('=' expression)?
	ast::BaseExpr* Parser::parse_variable_declaration()
	{
		start_;
		types::Type var_type = curr_type;

		get_next_token();

		if (curr_token != Token::VariableReference)
		{
			end_;
			return log_error("expected identifier after type");
		}

		std::string name = identifier_string;
		get_next_token();

		ast::BaseExpr* expr = nullptr;

		if (last_char == '=')
		{
			get_next_token();

			expr = parse_expression(false);
			if (expr == nullptr)
			{
				end_;
				return log_error("variable expression was incorrect");
			}
		}

		end_;
		expr->get_body()->named_types[name] = var_type;
		return new ast::VariableDeclarationExpr(bodies.back(), var_type, name, expr);
	}

	/// variable_reference_expr
	///   ::= identifier
	///   ::= identifier '(' expression* ')'
	ast::BaseExpr* Parser::parse_variable_reference()
	{
		start_;
		std::string name = identifier_string;

		get_next_token();

		// simple variable reference
		if (curr_token != Token::ParenStart)
		{
			end_;
			return new ast::VariableReferenceExpr(bodies.back(), name);
		}

		get_next_token();

		// function call
#ifdef __debug
		std::cout << "creating function call" << std::endl;
#endif
		std::vector<ast::BaseExpr*> args;
		if (curr_token != Token::ParenEnd)
		{
			while (true)
			{
				ast::BaseExpr* arg = parse_expression(true);
				if (arg != nullptr)
				{
					args.push_back(arg);
				}
				else
				{
					end_;
					return nullptr;
				}

				if (curr_token == Token::ParenEnd)
				{
					break;
				}

				if (curr_token != Token::Comma)
				{
					end_;
					return log_error("expected ')' or ',' in argument list");
				}

				get_next_token();
			}
		}

		get_next_token();

		end_;
		return new ast::CallExpr(bodies.back(), name, args);
	}

	ast::BaseExpr* Parser::parse_parenthesis()
	{
		start_;
		get_next_token();

		ast::BaseExpr* expr = parse_expression(false);

		if (curr_token != Token::ParenEnd)
		{
			end_;
			return log_error("expected ')'");
		}

		if (expr == nullptr)
		{
			end_;
			return nullptr;
		}

		get_next_token();

		return expr;
	}

	/// prototype
	///   ::= type id '(' [type id,]* ')'
	ast::FunctionPrototype* Parser::parse_function_prototype()
	{
		start_;
		if (curr_token != Token::VariableDeclaration)
		{
			end_;
			return log_error_prototype("expected function return type");
		}

		types::Type return_type = curr_type;

		get_next_token();

		if (curr_token != Token::VariableReference)
		{
			end_;
			return log_error_prototype("expected function name in prototype");
		}

		std::string name = identifier_string;

		get_next_token();

		if (curr_token != Token::ParenStart)
		{
			end_;
			return log_error_prototype("expected '(' in prototype");
		}

		std::vector<types::Type> types;
		std::vector<std::string> args;

		get_next_token();

		if (curr_token != Token::ParenEnd)
		{
			while (true)
			{
				if (curr_token != Token::VariableDeclaration)
				{
					end_;
					// TODO: make clearer whether it is variable or ')'
					return log_error_prototype("expected a variable type");
				}

				types.push_back(curr_type);

				get_next_token();
				if (curr_token != Token::VariableReference)
				{
					end_;
					return log_error_prototype("expected a variable identifier");
				}

				args.push_back(identifier_string);

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
						end_;
						return log_error_prototype("invalid char in prototype");
					}
				}

				/*	if (curr_token == Token::None)
					{
						if (last_char == ',')
						{
							get_next_token();
							continue;
						}
						else if (last_char == ')')
						{
							break;
						}
						else
						{
							end_;
							return log_error_prototype("invalid char in prototype");
						}
					}*/
			}
		}

		//get_next_token();

		end_;
		return new ast::FunctionPrototype(name, return_type, types, args);
	}

	/// definition ::= 'function' prototype expression
	ast::FunctionDefinition* Parser::parse_function_definition()
	{
		get_next_token();

		start_;
		ast::FunctionPrototype* proto = parse_function_prototype();
		if (proto == nullptr)
		{
			end_;
			return nullptr;
		}

		//ast::BaseExpr* expr = parse_expression();
		ast::BodyExpr* body = parse_body(false, true);
		if (body == nullptr)
		{
			end_;
			return nullptr;
		}

		for (int i = 0; i < proto->args.size(); i++)
		{
			body->named_types[proto->args[i]] = proto->types[i];
		}

		end_;
		return new ast::FunctionDefinition(proto, body);
	}

	int Parser::get_token_precedence()
	{
		if (std::isalpha(last_char))
		{
			return -1;
		}

		// Make sure it's a declared binop.
		auto f = binop_precedence.find(last_char);
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

	ast::BaseExpr* Parser::log_error(std::string error_message)
	{
		start_;
		std::cout << error_message << std::endl;
		log_line_info();
		end_;
		return nullptr;
	}

	ast::BodyExpr* Parser::log_error_body(std::string error_message)
	{
		start_;
		std::cout << error_message << std::endl;
		log_line_info();
		end_;
		return nullptr;
	}

	ast::FunctionPrototype* Parser::log_error_prototype(std::string error_message)
	{
		start_;
		std::cout << error_message << std::endl;
		log_line_info();
		end_;
		return nullptr;
	}

	void Parser::log_line_info()
	{
		std::cout << '\t' << "curr token: " << (int) curr_token << ", last char: " << last_char << std::endl;
		std::cout << '\t' << "identifier string: " << identifier_string << std::endl;
		std::cout << '\t' << "at line: " << line_info.line_count << " at pos: " << line_info.line_pos << std::endl;
		std::cout << '\t' << line_info.line << std::endl;
		std::cout << '\t' << std::setfill(' ') << std::setw(line_info.line_pos) << '^' << std::endl;
		std::cout << std::endl;
	}

}
