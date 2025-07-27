#pragma once

#include <string>
#include <vector>

namespace cliParser
{
	struct cli_parsed_data
	{
		std::string executable{};
		std::vector<std::string> flag_options{};
		std::vector<std::pair<std::string, std::string>> value_options{};
		std::vector<std::string> values{};

		bool valid{false};
		std::string parse_error{};

		bool hasOptionFlag(const std::string& option) const;
		bool hasOptionValue(const std::string& option)const;
		const std::string& getOptionValue(const std::string& option) const;
	};

	cli_parsed_data parse_arguemnts(const std::vector<std::string>& arguments);
	cli_parsed_data parse_arguemnts(int argc, char** argv);
}
