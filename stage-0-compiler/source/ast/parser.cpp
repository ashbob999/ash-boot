#include <iostream>

#include "parser.h"
#include "module.h"

//#define __debug

#ifdef __debug
#define start_ std::cout << "start: " << __FUNCTION__ << std::endl;
#define end_ std::cout << "end: " << __FUNCTION__ << std::endl;
#else
#define start_
#define end_
#endif

using std::make_shared;

namespace parser
{
	// the operator precedences, higher binds tighter, same binds to the left
	const std::unordered_map<char, int> Parser::binop_precedence = {
		{'=',  2},
		{'<', 10},
		{'>', 10},
		{'+', 20},
		{'-', 20},
		{'*', 40},
		{'/', 40},
	};

	Parser::Parser(std::ifstream& input_file) : input_file(input_file)
	{}

	shared_ptr<ast::BaseExpr> Parser::parse_file()
	{
		shared_ptr<ast::BaseExpr> file_body = parse_body(true, false);

		return file_body;
	}

	shared_ptr<ast::FunctionDefinition> Parser::parse_file_as_func()
	{
		shared_ptr<ast::FunctionDefinition> func = parse_top_level();

		return func;
	}

	shared_ptr<ast::BodyExpr> Parser::parse_file_as_body()
	{
		start_;
		bodies.push_back(nullptr);
		shared_ptr<ast::BodyExpr> body = parse_body(true, false);
		end_;
		return body;
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
				else
				{
					// check if identifier is a bool
					std::pair<bool, types::Type> res = types::check_type_string(identifier_string);
					if (res.first && res.second == types::Type::Bool)
					{
						curr_type = res.second;
						curr_token = Token::LiteralValue;
						return curr_token;
					}

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
			while (std::isdigit(next_char) /*|| types::BaseType::is_sign_char(next_char)*/ || next_char == '.' || next_char == 'f')
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
		if (ast::is_binary_op(last_char) != ast::BinaryOp::None)
		{
			curr_token = Token::BinaryOperator;
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

		//std::cout << "identifier " << identifier_string << " " << last_char << std::endl;

		// anything else
		curr_token = Token::None;
		return curr_token;
	}

	/// top ::= (definition | expression)*
	shared_ptr<ast::BodyExpr> Parser::parse_body(bool is_top_level, bool has_curly_brackets)
	{
		start_;
		shared_ptr<ast::BodyExpr> body = make_shared<ast::BodyExpr>(bodies.back());

		if (has_curly_brackets)
		{
			if (curr_token != Token::BodyStart)
			{
				return log_error_body("Body must start with a '{'");
			}
		}

		bodies.push_back(body.get());

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
						return log_error_body("Body must end with a '}'");
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

					shared_ptr<ast::FunctionDefinition> fd = parse_function_definition();
					if (fd != nullptr)
					{
#ifdef __debug
						std::cout << "added function" << std::endl;
#endif
						body->add_function(fd);
					}
					else
					{
						return nullptr;
					}

					break;
				}
				case Token::ExternFunction:
				{
					ast::FunctionPrototype* proto = parse_extern();

					if (proto == nullptr)
					{
						return nullptr;
					}

					body->add_prototype(proto);
					break;
				}
				case Token::IfStatement:
				{
					shared_ptr<ast::BaseExpr> base = parse_if_else();

					if (base == nullptr)
					{
						return nullptr;
					}

					shared_ptr<ast::IfExpr> if_expr = std::dynamic_pointer_cast<ast::IfExpr>(base);
					if_expr->should_return_value = false;

					body->add_base(base);

					break;
				}
				case Token::Comment:
				{
					end_;
					parse_comment();
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
						return log_error_body("Expressions are not allowed in top level code");
					}

					shared_ptr<ast::BaseExpr> base = parse_expression(false, false);
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

					if (curr_token != Token::EndOfExpression)
					{
						return log_error_body("Expected end of expression, missing ';'");
					}

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

	shared_ptr<ast::FunctionDefinition> Parser::parse_top_level()
	{
		start_;
		bodies.push_back(nullptr);
		shared_ptr<ast::BodyExpr> expr = parse_body(true, false);
		if (expr == nullptr)
		{
			end_;
			return nullptr;
		}

		std::string name = "_";
		std::vector<types::Type> types;
		std::vector<int> arg_ids;

		ast::FunctionPrototype* proto = new ast::FunctionPrototype(name, types::Type::Void, types, arg_ids);
		end_;
		return make_shared<ast::FunctionDefinition>(proto, expr);
	}

	/// expression
	///   ::= primary binop_rhs
	shared_ptr<ast::BaseExpr> Parser::parse_expression(bool for_call, bool for_if_cond)
	{
		start_;
		shared_ptr<ast::BaseExpr> lhs = parse_primary();
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

		if (for_if_cond)
		{
			end_;
			return lhs;
		}

		end_;
		return log_error("Expected end of expression, missing ';'");
	}

	/// primary
	///   ::= variable_declaration_expr
	///   ::= variable_reference_expr
	///   ::= literal_expr
	///   ::= parenthesis_expr
	shared_ptr<ast::BaseExpr> Parser::parse_primary()
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
			case Token::IfStatement:
			{
				end_;
				return parse_if_else();
			}
			default:
			{
				end_;
				return log_error("Unknown token when expecting an expression");
			}
		}
		end_;
		return nullptr;
	}

	/// binop_rhs
	///   ::= ('+' primary)*
	shared_ptr<ast::BaseExpr> Parser::parse_binop_rhs(int expr_precedence, shared_ptr<ast::BaseExpr> lhs)
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
				return log_error("Binary operator is invalid");
			}

			get_next_token();

			shared_ptr<ast::BaseExpr> rhs = parse_primary();

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

			lhs = make_shared<ast::BinaryExpr>(bodies.back(), binop_type, lhs, rhs);
		}

		end_;
		return nullptr;
	}

	/// literal_expr ::= 'curr_type'
	shared_ptr<ast::BaseExpr> Parser::parse_literal()
	{
		start_;
		if (curr_type == types::Type::None)
		{
			end_;
			return log_error("Literal is not a valid type");
		}

		std::string literal_string = identifier_string;

		get_next_token();

		end_;
		return make_shared<ast::LiteralExpr>(bodies.back(), curr_type, literal_string);
	}

	/// variable_declaration_expr ::= 'curr_type' identifier ('=' expression)?
	shared_ptr<ast::BaseExpr> Parser::parse_variable_declaration()
	{
		start_;
		types::Type var_type = curr_type;

		get_next_token();

		if (curr_token != Token::VariableReference)
		{
			end_;
			return log_error("Expected identifier after type");
		}

		std::string name = identifier_string;
		get_next_token();

		shared_ptr<ast::BaseExpr> expr = nullptr;

		if (last_char == '=')
		{
			get_next_token();

			expr = parse_expression(false, false);
			if (expr == nullptr)
			{
				end_;
				return nullptr;
			}
		}

		end_;
		expr->get_body()->named_types[module::StringManager::get_id(name)] = var_type;
		return make_shared<ast::VariableDeclarationExpr>(bodies.back(), var_type, name, expr);
	}

	/// variable_reference_expr
	///   ::= identifier
	///   ::= identifier '(' expression* ')'
	shared_ptr<ast::BaseExpr> Parser::parse_variable_reference()
	{
		start_;
		std::string name = identifier_string;

		get_next_token();

		// simple variable reference
		if (curr_token != Token::ParenStart)
		{
			end_;
			return make_shared<ast::VariableReferenceExpr>(bodies.back(), name);
		}

		get_next_token();

		// function call
#ifdef __debug
		std::cout << "creating function call" << std::endl;
#endif
		std::vector<shared_ptr<ast::BaseExpr>> args;
		if (curr_token != Token::ParenEnd)
		{
			while (true)
			{
				shared_ptr<ast::BaseExpr> arg = parse_expression(true, false);
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
					return log_error("Expected ')' or ',' in function argument list");
				}

				get_next_token();
			}
		}

		get_next_token();

		end_;
		return make_shared<ast::CallExpr>(bodies.back(), name, args);
	}

	shared_ptr<ast::BaseExpr> Parser::parse_parenthesis()
	{
		start_;
		get_next_token();

		shared_ptr<ast::BaseExpr> expr = parse_expression(false, false);

		if (curr_token != Token::ParenEnd)
		{
			end_;
			return log_error("Expected ')' after expression");
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
			log_error_empty("Return type for function prototype is invalid");
			return nullptr;
		}

		types::Type return_type = curr_type;

		get_next_token();

		if (curr_token != Token::VariableReference)
		{
			end_;
			log_error_empty("Expected function name in function prototype");
			return nullptr;
		}

		std::string name = identifier_string;

		get_next_token();

		if (curr_token != Token::ParenStart)
		{
			end_;
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
				if (curr_token != Token::VariableDeclaration)
				{
					end_;
					log_error_empty("Expected variable declaration in function prototype");
					return nullptr;
				}

				types.push_back(curr_type);

				get_next_token();
				if (curr_token != Token::VariableReference)
				{
					end_;
					log_error_empty("Expected a variable identifier in function prototype");
					return nullptr;
				}

				args.push_back(module::StringManager::get_id(identifier_string));

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
						log_error_empty("Invalid character in function prototype");
						return nullptr;
					}
				}
			}
		}

		//get_next_token();

		end_;
		return new ast::FunctionPrototype(name, return_type, types, args);
	}

	/// external ::= 'extern' prototype
	ast::FunctionPrototype* Parser::parse_extern()
	{
		get_next_token();

		ast::FunctionPrototype* proto = parse_function_prototype();

		if (curr_token == Token::EndOfExpression)
		{
			end_;
			log_error_empty("Expected end of expression, missing ';'");
			return nullptr;
		}

		end_;
		return proto;
	}

	/// definition ::= 'function' prototype expression
	shared_ptr<ast::FunctionDefinition> Parser::parse_function_definition()
	{
		get_next_token();

		start_;
		ast::FunctionPrototype* proto = parse_function_prototype();
		if (proto == nullptr)
		{
			end_;
			return nullptr;
		}

		get_next_token();

		shared_ptr<ast::BodyExpr> body = parse_body(false, true);
		if (body == nullptr)
		{
			end_;
			delete proto;
			return nullptr;
		}

		for (int i = 0; i < proto->args.size(); i++)
		{
			body->named_types[proto->args[i]] = proto->types[i];
		}

		body->is_function_body = true;

		end_;
		return make_shared<ast::FunctionDefinition>(proto, body);
	}

	shared_ptr<ast::BaseExpr> Parser::parse_if_else()
	{
		start_;
		get_next_token();

		shared_ptr<ast::BaseExpr> cond = parse_expression(false, true);

		if (cond == nullptr)
		{
			end_;
			return log_error("Expected if condition");
		}

		shared_ptr<ast::BodyExpr> if_body = parse_body(false, true);

		if (if_body == nullptr)
		{
			end_;
			return log_error("Expected if body");
		}

		get_next_token();

		if (curr_token != Token::ElseStatement)
		{
			end_;
			return log_error("Expected else");
		}

		get_next_token();

		shared_ptr<ast::BodyExpr> else_body = parse_body(false, true);

		if (else_body == nullptr)
		{
			end_;
			return log_error("Expected else body");
		}

		get_next_token();

		end_;
		return make_shared<ast::IfExpr>(bodies.back(), cond, if_body, else_body);
	}

	shared_ptr<ast::BaseExpr> Parser::parse_comment()
	{
		// skip till end of line
		while (last_char != '\n')
		{
			last_char = get_char();
		}

		curr_token = Token::EndOfExpression;

		return make_shared<ast::CommentExpr>(bodies.back());
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

	shared_ptr<ast::BaseExpr> Parser::log_error(std::string error_message)
	{
		log_error_empty(error_message);
		return nullptr;
	}

	shared_ptr<ast::BodyExpr> Parser::log_error_body(std::string error_message)
	{
		log_error_empty(error_message);
		return nullptr;
	}

	shared_ptr<ast::FunctionPrototype> Parser::log_error_prototype(std::string error_message)
	{
		log_error_empty(error_message);
		return nullptr;
	}

	void Parser::log_error_empty(std::string error_message)
	{
		start_;
		std::cout << error_message << std::endl;
		log_line_info();
		end_;
	}

	void Parser::log_line_info()
	{
#ifdef __debug
		std::cout << '\t' << "Current Token ID: " << (int) curr_token << std::endl;
#endif
		std::cout << '\t' << "Current Character: " << last_char << std::endl;
		//std::cout << '\t' << "curr token: " << (int) curr_token << ", last char: " << last_char << std::endl;
		if (identifier_string != "")
		{
			std::cout << '\t' << "Identifier String: " << identifier_string << std::endl;
		}
		else
		{
#ifdef __debug
			std::cout << '\t' << "Identifier String: <empty>" << std::endl;
#endif
		}

		std::cout << '\t' << "At Line: " << line_info.line_count << " Position: " << line_info.line_pos << std::endl;

		std::cout << '\t' << line_info.line << std::endl;
		std::cout << '\t' << std::setfill(' ') << std::setw(line_info.line_pos_start - 1) << "";
		std::cout << std::setfill('~') << std::setw(line_info.line_pos - line_info.line_pos_start + 1);
		std::cout << '^' << std::endl;
		std::cout << std::endl;
	}

}
