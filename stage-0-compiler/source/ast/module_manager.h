#pragma once

#include "../config.h"
#include "ast.h"

#include <string>
#include <unordered_set>
#include <vector>

namespace moduleManager
{
	void add_ast(int filename, ptr_type<ast::BodyExpr> ast_body);
	int get_file_as_module(const std::string& file_name);
	void add_module(int filename, int module_id, std::unordered_set<int>& usings);
	bool check_modules();
	bool is_module_available(int filename, int module_id);
	int find_function(int filename, int name_id, bool is_mangled);
	std::vector<int> get_matching_function_locations(int filename, int name_id);
	std::unordered_set<int>& get_exported_functions(int filename);
	int get_module(int filename);
	ast::BodyExpr* get_ast(int filename);
	std::vector<int> get_build_files_order();
	ast::BodyExpr* find_body(int function_id);
}
