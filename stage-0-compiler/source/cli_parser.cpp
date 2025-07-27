#include "cli_parser.h"

namespace cliParser
{
	cli_parsed_data parse_arguemnts(const std::vector<std::string>& arguments)
	{
		cli_parsed_data data{};

		if (!arguments.empty())
		{
			data.executable = arguments.front();
			data.valid = true;
		}

		for (size_t i = 1; i < arguments.size(); i++)
		{
			auto& arg = arguments.at(i);

			if (arg.find("--") == 0)
			{
				auto equalsIndex = arg.find('=');
				if (equalsIndex != std::string::npos)
				{
					auto key = arg.substr(2, equalsIndex - 2);
					auto value = arg.substr(equalsIndex + 1);
					data.value_options.push_back({key, value});
				}
				else
				{
					data.flag_options.push_back(arg.substr(2));
				}
			}
			else
			{
				data.values.push_back(arg);
			}
		}

		return data;
	}

	cli_parsed_data parse_arguemnts(int argc, char** argv)
	{
		std::vector<std::string> arguments{};

		for (int i = 0; i < argc; i++)
		{
			arguments.emplace_back(argv[i]);
		}

		return parse_arguemnts(arguments);
	}

	bool cli_parsed_data::hasOptionFlag(const std::string& option) const
	{
		return std::find(this->flag_options.begin(), this->flag_options.end(), option) != this->flag_options.end();
	}

	bool cli_parsed_data::hasOptionValue(const std::string& option) const
	{
		for (auto& [key, value] : this->value_options)
		{
			if (key == option)
			{
				return true;
			}
		}
		return false;
	}

	const std::string& cli_parsed_data::getOptionValue(const std::string& option) const
	{
		static const std::string emptyString{};

		for (auto& [key, value] : this->value_options)
		{
			if (key == option)
			{
				return value;
			}
		}
		return emptyString;
	}
}
