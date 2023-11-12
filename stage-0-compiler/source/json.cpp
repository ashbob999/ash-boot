#include "json.h"
#include "./utils.h"

#include <cassert>

namespace json
{
	std::string indent_to_string(size_t indent)
	{
		return std::string(indent, '\t');
	}

	JsonString::JsonString(const std::string& string) : data{string} {}

	std::string JsonString::to_string(size_t indent, bool minified) const
	{
		std::string string;

		string += '"';

		// TODO: escape chars
		string += this->data;

		string += '"';

		return string;
	}

	JsonNumber::JsonNumber(uint64_t number) : data{number} {}

	JsonNumber::JsonNumber(int64_t number) : data{number} {}

	JsonNumber::JsonNumber(double number) : data{number} {}

	std::string JsonNumber::to_string(size_t indent, bool minified) const
	{
		if (std::holds_alternative<uint64_t>(this->data))
		{
			return std::to_string(std::get<uint64_t>(this->data));
		}
		else if (std::holds_alternative<int64_t>(this->data))
		{
			return std::to_string(std::get<int64_t>(this->data));
		}
		else if (std::holds_alternative<double>(this->data))
		{
			return std::to_string(std::get<double>(this->data));
		}
	}

	static constexpr const char* TrueLiteral = "\"true\"";

	std::string JsonTrue::to_string(size_t indent, bool minified) const
	{
		return std::string{TrueLiteral};
	}

	static constexpr const char* FalseLiteral = "\"false\"";

	std::string JsonFalse::to_string(size_t indent, bool minified) const
	{
		return std::string{FalseLiteral};
	}

	static constexpr const char* NullLiteral = "\"null\"";

	std::string JsonNull::to_string(size_t indent, bool minified) const
	{
		return std::string{NullLiteral};
	}

	void JsonObject::addData(const std::string& key, const JsonValue& value)
	{
		this->data.emplace_back(key, value);
	}

	std::string JsonObject::to_string(size_t indent, bool minified) const
	{
		std::string string;

		std::string spacing;
		if (!minified)
		{
			spacing = indent_to_string(indent);
		}

		string += '{';

		string += '\n';

		for (size_t i = 0; i < this->data.size(); i++)
		{
			const auto& keyValue = this->data[i];

			if (!minified)
			{
				string += indent_to_string(indent + 1);
			}
			string += JsonString(keyValue.first).to_string(indent + 1, minified);
			string += ':';

			if (!minified)
			{
				string += ' ';
			}

			string += keyValue.second.to_string(indent + 1, minified);

			if (i < this->data.size() - 1)
			{
				string += ',';
			}

			if (!minified)
			{
				string += '\n';
			}
		}

		string += spacing;
		string += '}';

		return string;
	}

	void JsonArray::addValue(const JsonValue& value)
	{
		this->data.push_back(value);
	}

	std::string JsonArray::to_string(size_t indent, bool minified) const
	{
		std::string string;

		std::string spacing;
		if (!minified)
		{
			spacing = indent_to_string(indent);
		}

		string += '[';

		string += '\n';

		for (size_t i = 0; i < this->data.size(); i++)
		{
			if (!minified)
			{
				string += indent_to_string(indent + 1);
			}
			string += this->data[i].to_string(indent + 1, minified);

			if (i < this->data.size() - 1)
			{
				string += ',';
			}

			if (!minified)
			{
				string += '\n';
			}
		}

		string += spacing;
		string += ']';

		return string;
	}

	std::string JsonValue::to_string(bool minified) const
	{
		return this->to_string(0, minified);
	}

	std::string JsonValue::to_string(size_t indent, bool minified) const
	{
		if (std::holds_alternative<copyable_ptr<JsonObject>>(this->data))
		{
			return std::get<copyable_ptr<JsonObject>>(this->data)->to_string(indent, minified);
		}
		else if (std::holds_alternative<copyable_ptr<JsonArray>>(this->data))
		{
			return std::get<copyable_ptr<JsonArray>>(this->data)->to_string(indent, minified);
		}
		else if (std::holds_alternative<JsonString>(this->data))
		{
			return std::get<JsonString>(this->data).to_string(indent, minified);
		}
		else if (std::holds_alternative<JsonNumber>(this->data))
		{
			return std::get<JsonNumber>(this->data).to_string(indent, minified);
		}
		else if (std::holds_alternative<JsonTrue>(this->data))
		{
			return std::get<JsonTrue>(this->data).to_string(indent, minified);
		}
		else if (std::holds_alternative<JsonFalse>(this->data))
		{
			return std::get<JsonFalse>(this->data).to_string(indent, minified);
		}
		else if (std::holds_alternative<JsonNull>(this->data))
		{
			return std::get<JsonNull>(this->data).to_string(indent, minified);
		}
		assert(false && "Unknown std::variant value");
	}
}
