#include <iostream>

#include "parser.h"

using std::make_shared;

namespace parser
{
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

	Parser::Parser(std::ifstream& input_file, std::string file_name) : input_file(input_file), file_name(file_name)
	{
		filename_id = module::ModuleManager::get_file_as_module(file_name);
	}

	shared_ptr<ast::BaseExpr> Parser::parse_file()
	{
		shared_ptr<ast::BaseExpr> file_body = parse_body(ast::BodyType::Global, true, false);

		return file_body;
	}

	shared_ptr<ast::FunctionDefinition> Parser::parse_file_as_func()
	{
		shared_ptr<ast::FunctionDefinition> func = parse_top_level();

		return func;
	}

	shared_ptr<ast::BodyExpr> Parser::parse_file_as_body()
	{
		bodies.push_back(nullptr);
		shared_ptr<ast::BodyExpr> body = parse_body(ast::BodyType::Global, true, false);
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

				curr_token = Token::VariableReference;
				return Token::VariableReference;
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
	shared_ptr<ast::BodyExpr> Parser::parse_body(ast::BodyType body_type, bool is_top_level, bool has_curly_brackets)
	{
		shared_ptr<ast::BodyExpr> body = make_shared<ast::BodyExpr>(bodies.back(), body_type);

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
			switch (curr_token)
			{
				case Token::EndOfFile:
				{
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
					break;
				}
				case Token::VariableDeclaration:
				{
					if (is_top_level)
					{
						return log_error_body("Expressions are not allowed in top level code");
					}

					shared_ptr<ast::BaseExpr> base = parse_variable_declaration();

					if (base == nullptr)
					{
						return nullptr;
					}

					body->add_base(base);

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
						return log_error_body("Nested Functions are currently disabled.");
					}

					shared_ptr<ast::FunctionDefinition> fd = parse_function_definition();
					if (fd != nullptr)
					{
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
					if (this->finished_parsing_modules == false)
					{
						this->update_current_module();
					}

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
					if (is_top_level)
					{
						return log_error_body("If statements are not allowed in top level code");
					}

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
				case Token::ForStatement:
				{
					if (is_top_level)
					{
						return log_error_body("For statements are not allowed in top level code");
					}

					shared_ptr<ast::BaseExpr> base = parse_for_loop();

					if (base == nullptr)
					{
						return nullptr;
					}

					body->add_base(base);

					break;
				}
				case Token::WhileStatement:
				{
					if (is_top_level)
					{
						return log_error_body("While statements are not allowed in top level code");
					}

					shared_ptr<ast::BaseExpr> base = parse_while_loop();

					if (base == nullptr)
					{
						return nullptr;
					}

					body->add_base(base);

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
						return log_error_body("Return statements are not allowed in top level code");
					}

					shared_ptr<ast::BaseExpr> base = parse_return();

					if (base == nullptr)
					{
						return nullptr;
					}

					body->add_base(base);

					break;
				}
				case Token::ContinueStatement:
				{
					if (is_top_level)
					{
						return log_error_body("Continue statements are not allowed in top level code");
					}

					shared_ptr<ast::BaseExpr> base = parse_continue();

					if (base == nullptr)
					{
						return nullptr;
					}

					body->add_base(base);

					break;
				}
				case Token::BreakStatement:
				{
					if (is_top_level)
					{
						return log_error_body("Break statements are not allowed in top level code");
					}

					shared_ptr<ast::BaseExpr> base = parse_break();

					if (base == nullptr)
					{
						return nullptr;
					}

					body->add_base(base);

					break;
				}
				case Token::ModuleStatement:
				{
					if (!is_top_level)
					{
						return log_error_body("Module Definition can only be in top level code");
					}

					if (this->finished_parsing_modules)
					{
						return log_error_body("Module Definitions must be before any other code");
					}

					if (!parse_module())
					{
						return nullptr;
					}

					break;
				}
				case Token::UsingStatement:
				{
					if (!is_top_level)
					{
						return log_error_body("Using Statement can only be in top level code");
					}

					if (this->finished_parsing_modules)
					{
						return log_error_body("Using Statements must be before any other code");
					}

					if (!parse_using())
					{
						return nullptr;
					}

					break;
				}
				case Token::BodyEnd:
				{
					bodies.pop_back();
					return body;
					break;
				}
				default:
				{
					if (is_top_level)
					{
						return log_error_body("Expressions are not allowed in top level code");
					}

					shared_ptr<ast::BaseExpr> base = parse_expression(false, false);
					if (base == nullptr)
					{
						return nullptr;
					}

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
				curr_token = get_next_token();
			}
		}

		return body;
	}

	shared_ptr<ast::FunctionDefinition> Parser::parse_top_level()
	{
		bodies.push_back(nullptr);
		shared_ptr<ast::BodyExpr> expr = parse_body(ast::BodyType::Global, true, false);
		if (expr == nullptr)
		{
			return nullptr;
		}

		std::string name = "_";
		std::vector<types::Type> types;
		std::vector<int> arg_ids;

		ast::FunctionPrototype* proto = new ast::FunctionPrototype(name, types::Type::Void, types, arg_ids);
		return make_shared<ast::FunctionDefinition>(proto, expr);
	}

	/// expression
	///   ::= primary binop_rhs
	shared_ptr<ast::BaseExpr> Parser::parse_expression(bool for_call, bool for_if_cond)
	{
		shared_ptr<ast::BaseExpr> lhs = parse_unary();
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
			return parse_binop_rhs(0, lhs);
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

		if (for_if_cond)
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
	shared_ptr<ast::BaseExpr> Parser::parse_primary()
	{
		switch (curr_token)
		{
			case Token::VariableReference:
			{
				return parse_variable_reference();
			}
			case Token::LiteralValue:
			{
				return parse_literal();
			}
			case Token::ParenStart:
			{
				return parse_parenthesis();
			}
			case Token::IfStatement:
			{
				return parse_if_else();
			}
			default:
			{
				return log_error("Unknown token when expecting an expression");
			}
		}
	}

	/// binop_rhs
	///   ::= ('+' primary)*
	shared_ptr<ast::BaseExpr> Parser::parse_binop_rhs(int expr_precedence, shared_ptr<ast::BaseExpr> lhs)
	{
		while (true)
		{
			int token_precedence = get_token_precedence();

			//std::cout << "token p " << token_precedence << " exrp p " << expr_precedence << std::endl;

			// If this is a binop that binds at least as tightly as the current binop,
			// consume it, otherwise we are done.
			if (token_precedence < expr_precedence)
			{
				//std::cout << "ealry end" << std::endl;
				return lhs;
			}

			operators::BinaryOp binop_type = operators::is_binary_op(identifier_string);
			if (binop_type == operators::BinaryOp::None)
			{
				return log_error("Binary operator is invalid");
			}

			get_next_token();

			shared_ptr<ast::BaseExpr> rhs = parse_unary();

			if (rhs == nullptr)
			{
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
					return nullptr;
				}
			}

			lhs = make_shared<ast::BinaryExpr>(bodies.back(), binop_type, lhs, rhs);
		}

		return nullptr;
	}

	/// unary_expr
	///   ::= primary
	///   ::= unop expression
	shared_ptr<ast::BaseExpr> Parser::parse_unary()
	{
		// if current char is not a unary op, then it is a primary expression
		if (operators::is_unary_op(last_char) == operators::UnaryOp::None)
		{
			return parse_primary();
		}

		// get the unary operator
		operators::UnaryOp unop = operators::is_unary_op(last_char);

		get_next_token();

		shared_ptr<ast::BaseExpr> expr = parse_unary();
		if (!expr)
		{
			return nullptr;
		}

		return make_shared<ast::UnaryExpr>(bodies.back(), unop, expr);
	}

	/// literal_expr ::= 'curr_type'
	shared_ptr<ast::BaseExpr> Parser::parse_literal()
	{
		if (curr_type == types::Type::None)
		{
			return log_error("Literal is not a valid type");
		}

		std::string literal_string = identifier_string;

		// check if literal if whithin the correct range
		// TODO: not ignore float
		if (!types::check_range(literal_string, curr_type))
		{
			return log_error("Literal value for type: " + types::to_string(curr_type) + " is out of range");
		}

		get_next_token();

		return make_shared<ast::LiteralExpr>(bodies.back(), curr_type, literal_string);
	}

	/// variable_declaration_expr ::= var 'curr_type' identifier ('=' expression)?
	shared_ptr<ast::BaseExpr> Parser::parse_variable_declaration()
	{
		get_next_token();

		types::Type var_type = types::is_valid_type(identifier_string);

		if (var_type == types::Type::None)
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

		shared_ptr<ast::BaseExpr> expr = nullptr;

		if (last_char == '=')
		{
			get_next_token();

			expr = parse_expression(false, false);
			if (expr == nullptr)
			{
				return nullptr;
			}
		}

		int name_id = module::StringManager::get_id(name);

		expr->get_body()->named_types[name_id] = var_type;
		return make_shared<ast::VariableDeclarationExpr>(bodies.back(), var_type, name_id, expr);
	}

	/// variable_reference_expr
	///   ::= identifier
	///   ::= identifier '(' expression* ')'
	shared_ptr<ast::BaseExpr> Parser::parse_variable_reference()
	{
		int name_id = module::StringManager::get_id(identifier_string);

		get_next_token();

		// simple variable reference
		if (curr_token != Token::ParenStart)
		{
			return make_shared<ast::VariableReferenceExpr>(bodies.back(), name_id);
		}

		get_next_token();

		// function call
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

		get_next_token();

		return make_shared<ast::CallExpr>(bodies.back(), name_id, args);
	}

	shared_ptr<ast::BaseExpr> Parser::parse_parenthesis()
	{
		get_next_token();

		shared_ptr<ast::BaseExpr> expr = parse_expression(false, false);

		if (curr_token != Token::ParenEnd)
		{
			return log_error("Expected ')' after expression");
		}

		if (expr == nullptr)
		{
			return nullptr;
		}

		get_next_token();

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

		if (return_type == types::Type::None)
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
						log_error_empty("Invalid character in function prototype");
						return nullptr;
					}
				}
			}
		}

		//get_next_token();

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
	shared_ptr<ast::FunctionDefinition> Parser::parse_function_definition()
	{
		get_next_token();

		ast::FunctionPrototype* proto = parse_function_prototype();
		if (proto == nullptr)
		{
			return nullptr;
		}

		get_next_token();

		shared_ptr<ast::BodyExpr> body = parse_body(ast::BodyType::Function, false, true);
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

		return make_shared<ast::FunctionDefinition>(proto, body);
	}

	shared_ptr<ast::BaseExpr> Parser::parse_if_else()
	{
		get_next_token();

		shared_ptr<ast::BaseExpr> cond = parse_expression(false, true);

		if (cond == nullptr)
		{
			return log_error("Expected if condition");
		}

		shared_ptr<ast::BodyExpr> if_body = parse_body(ast::BodyType::Conditional, false, true);

		if (if_body == nullptr)
		{
			return log_error("Expected if body");
		}

		get_next_token();

		if (curr_token != Token::ElseStatement)
		{
			return log_error("Expected else");
		}

		get_next_token();

		shared_ptr<ast::BodyExpr> else_body = parse_body(ast::BodyType::Conditional, false, true);

		if (else_body == nullptr)
		{
			return log_error("Expected else body");
		}

		get_next_token();

		return make_shared<ast::IfExpr>(bodies.back(), cond, if_body, else_body);
	}

	/// forexpr ::= 'for' 'type' identifier '=' expr ';' expr (';' expr)? '{' expression* '}'
	shared_ptr<ast::BaseExpr> Parser::parse_for_loop()
	{
		get_next_token();

		if (curr_token != Token::VariableReference)
		{
			return log_error("Expected type after for");
		}

		types::Type var_type = types::is_valid_type(identifier_string);

		if (var_type == types::Type::None)
		{
			return log_error("Invalid type after for");
		}

		get_next_token();

		if (curr_token != Token::VariableReference)
		{
			return log_error("Expected identifier after type");
		}

		int name_id = module::StringManager::get_id(identifier_string);

		get_next_token();

		if (last_char != '=')
		{
			return log_error("Expected '=' after identifier");
		}

		get_next_token();

		shared_ptr<ast::BaseExpr> start_expr = parse_expression(false, false);

		if (start_expr == nullptr)
		{
			return nullptr;
		}

		get_next_token();

		shared_ptr<ast::BaseExpr> end_expr = parse_expression(false, false);

		if (end_expr == nullptr)
		{
			return nullptr;
		}

		get_next_token();

		shared_ptr<ast::BaseExpr> step_expr;

		// optional step expression
		if (curr_token != Token::BodyStart)
		{
			step_expr = parse_expression(false, false);

			if (step_expr == nullptr)
			{
				return nullptr;
			}
		}

		shared_ptr<ast::BodyExpr> for_body = parse_body(ast::BodyType::Loop, false, true);

		if (for_body == nullptr)
		{
			return nullptr;
		}

		get_next_token();

		// change the body of the start, end, step expressions to the for body
		start_expr->set_body(for_body.get());
		end_expr->set_body(for_body.get());
		if (step_expr != nullptr)
		{
			step_expr->set_body(for_body.get());
		}

		start_expr->get_body()->named_types[name_id] = var_type;

		return make_shared<ast::ForExpr>(bodies.back(), var_type, name_id, start_expr, end_expr, step_expr, for_body);
	}

	/// whileexpr ::= 'while' expr '{' expression* '}'
	shared_ptr<ast::BaseExpr> Parser::parse_while_loop()
	{
		get_next_token();

		shared_ptr<ast::BaseExpr> end_expr = parse_expression(false, true);

		if (end_expr == nullptr)
		{
			return nullptr;
		}

		//get_next_token();

		shared_ptr<ast::BodyExpr> while_body = parse_body(ast::BodyType::Loop, false, true);

		if (while_body == nullptr)
		{
			return nullptr;
		}

		get_next_token();

		return make_shared<ast::WhileExpr>(bodies.back(), end_expr, while_body);
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

	/// returnexpr ::= 'return' expr?
	shared_ptr<ast::BaseExpr> Parser::parse_return()
	{
		get_next_token();

		shared_ptr<ast::BaseExpr> expr;

		if (curr_token != Token::EndOfExpression)
		{
			expr = parse_expression(false, false);

			if (expr == nullptr)
			{
				return log_error("Return statement expected expression");
			}
		}

		return make_shared<ast::ReturnExpr>(bodies.back(), expr);
	}

	/// continueexpr ::= 'continue'
	shared_ptr<ast::BaseExpr> Parser::parse_continue()
	{
		get_next_token();

		return make_shared<ast::ContinueExpr>(bodies.back());
	}

	/// breakexpr ::= 'break'
	shared_ptr<ast::BaseExpr> Parser::parse_break()
	{
		get_next_token();

		return make_shared<ast::BreakExpr>(bodies.back());
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

		int module_id = module::StringManager::get_id(identifier_string);

		get_next_token();

		if (curr_token != Token::EndOfExpression)
		{
			log_error_empty("Expected end of expression, missing ';'");
			return false;
		}

		if (this->current_module == -1)
		{
			this->current_module = module_id;
		}
		else
		{
			this->current_module = module::Mangler::add_module(this->current_module, module_id, false);
		}

		return true;
	}

	bool Parser::parse_using()
	{
		get_next_token();

		shared_ptr<ast::BaseExpr> base_expr = parse_expression(false, false);

		if (curr_token != Token::EndOfExpression)
		{
			log_error_empty("Expected end of expression, missing ';'");
			return false;
		}

		if (base_expr->get_type() != ast::AstExprType::VariableReferenceExpr && base_expr->get_type() != ast::AstExprType::BinaryExpr)
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
					log_error_empty("Binary Operator: " + operators::to_string(scope_expr->binop) + ", lhs must be an identifier or another scoper operator.");
					return false;
				}
				else
				{
					// check rhs type is variable refernce, if were not at the top scope binop
					if (scope_expr != expr && scope_expr->rhs->get_type() != ast::AstExprType::VariableReferenceExpr)
					{
						log_error_empty("Binary Operator: " + operators::to_string(scope_expr->binop) + ", rhs must be an identifier.");
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
				log_error_empty("Binary Operator: " + operators::to_string(expr->binop) + ", rhs must be an identifier.");
				return false;
			}
		}

		// get module id
		int module_id = -1;
		if (base_expr->get_type() == ast::AstExprType::BinaryExpr)
		{
			module_id = module::Mangler::get_module(dynamic_cast<ast::BinaryExpr*>(base_expr.get()));
		}
		else
		{
			module_id = dynamic_cast<ast::VariableReferenceExpr*>(base_expr.get())->name_id;
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
			current_module = filename_id;
		}

		module::ModuleManager::add_module(filename_id, current_module, using_modules);
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
		std::cout << error_message << std::endl;
		log_line_info();
	}

	void Parser::log_line_info()
	{
		std::cout << '\t' << "File: " << module::StringManager::get_string(this->filename_id) << std::endl;
		std::cout << '\t' << "Current Character: " << last_char << std::endl;
		//std::cout << '\t' << "curr token: " << (int) curr_token << ", last char: " << last_char << std::endl;
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
