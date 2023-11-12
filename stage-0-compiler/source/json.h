#pragma once

#include <string>
#include <variant>
#include <vector>

#include "./config.h"
#include "./utils.h"

namespace json
{
	class JsonValue;
	class JsonObject;
	class JsonArray;
	class JsonString;
	class JsonNumber;
	class JsonTrue;
	class JsonFalse;
	class JsonNull;

	class JsonString
	{
	public:
		JsonString() = default;
		JsonString(const std::string& string);

		std::string to_string(size_t indent, bool minified) const;

	private:
		std::string data;
	};

	class JsonNumber
	{
	public:
		JsonNumber() = default;
		JsonNumber(uint64_t number);
		JsonNumber(int64_t number);
		JsonNumber(double number);

		std::string to_string(size_t indent, bool minified) const;

	private:
		std::variant<uint64_t, int64_t, double> data;
	};

	class JsonTrue
	{
	public:
		std::string to_string(size_t indent, bool minified) const;
	};

	class JsonFalse
	{
	public:
		std::string to_string(size_t indent, bool minified) const;
	};

	class JsonNull
	{
	public:
		std::string to_string(size_t indent, bool minified) const;
	};

	class JsonObject
	{
	public:
		JsonObject() = default;

		void addData(const std::string& key, const JsonValue& value);

		std::string to_string(size_t indent, bool minified) const;

	private:
		// Use a vector to keep insertion order
		std::vector<std::pair<std::string, JsonValue>> data;
	};

	class JsonArray
	{
	public:
		JsonArray() = default;

		void addValue(const JsonValue& value);

		std::string to_string(size_t indent, bool minified) const;

	private:
		std::vector<JsonValue> data;
	};

	class JsonValue
	{
	public:
		using VariantType = std::variant<
			copyable_ptr<JsonObject>,
			copyable_ptr<JsonArray>,
			JsonString,
			JsonNumber,
			JsonTrue,
			JsonFalse,
			JsonNull>;

		JsonValue() : data{JsonNull{}} {}
		JsonValue(const JsonObject& data) : data{data} {}
		JsonValue(const JsonArray& data) : data{data} {}
		JsonValue(const JsonString& data) : data{data} {}
		JsonValue(const JsonNumber& data) : data{data} {}
		JsonValue(const JsonTrue& data) : data{data} {}
		JsonValue(const JsonFalse& data) : data{data} {}
		JsonValue(const JsonNull& data) : data{data} {}

		JsonValue(const std::string& str) : JsonValue{JsonString{str}} {}
		JsonValue(const char* str) : JsonValue{JsonString{str}} {}
		JsonValue(bool bool_value) : JsonValue{}
		{
			if (bool_value)
			{
				this->data = JsonTrue{};
			}
			else
			{
				this->data = JsonFalse{};
			}
		}

		template<class T>
		JsonValue(const T&) = delete;

		std::string to_string(bool minified = false) const;
		std::string to_string(size_t indent, bool minified) const;

	private:
		VariantType data;
	};

	inline constexpr JsonTrue True{};
	inline constexpr JsonFalse False{};
	inline constexpr JsonNull Null{};

	static_assert(std::is_copy_constructible_v<JsonValue>);
	static_assert(std::is_copy_constructible_v<JsonObject>);
	static_assert(std::is_copy_constructible_v<JsonArray>);
	static_assert(std::is_copy_constructible_v<JsonString>);
	static_assert(std::is_copy_constructible_v<JsonNumber>);
	static_assert(std::is_copy_constructible_v<JsonTrue>);
	static_assert(std::is_copy_constructible_v<JsonFalse>);
	static_assert(std::is_copy_constructible_v<JsonNull>);

	static_assert(std::is_copy_assignable_v<JsonValue>);
	static_assert(std::is_copy_assignable_v<JsonObject>);
	static_assert(std::is_copy_assignable_v<JsonArray>);
	static_assert(std::is_copy_assignable_v<JsonString>);
	static_assert(std::is_copy_assignable_v<JsonNumber>);
	static_assert(std::is_copy_assignable_v<JsonTrue>);
	static_assert(std::is_copy_assignable_v<JsonFalse>);
	static_assert(std::is_copy_assignable_v<JsonNull>);

	static_assert(std::is_move_constructible_v<JsonValue>);
	static_assert(std::is_move_constructible_v<JsonObject>);
	static_assert(std::is_move_constructible_v<JsonArray>);
	static_assert(std::is_move_constructible_v<JsonString>);
	static_assert(std::is_move_constructible_v<JsonNumber>);
	static_assert(std::is_move_constructible_v<JsonTrue>);
	static_assert(std::is_move_constructible_v<JsonFalse>);
	static_assert(std::is_move_constructible_v<JsonNull>);

	static_assert(std::is_move_assignable_v<JsonValue>);
	static_assert(std::is_move_assignable_v<JsonObject>);
	static_assert(std::is_move_assignable_v<JsonArray>);
	static_assert(std::is_move_assignable_v<JsonString>);
	static_assert(std::is_move_assignable_v<JsonNumber>);
	static_assert(std::is_move_assignable_v<JsonTrue>);
	static_assert(std::is_move_assignable_v<JsonFalse>);
	static_assert(std::is_move_assignable_v<JsonNull>);
}
