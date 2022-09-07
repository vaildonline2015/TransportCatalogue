#pragma once

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace json {

class Node;
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

class ParsingError : public std::runtime_error {
public:
	using runtime_error::runtime_error;
};

class Node final
	: private std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string> {
public:
	using variant::variant;
	using Value = variant;

	Node(Value value);

	bool IsNull() const;
	bool IsBool() const;
	bool IsInt() const;
	bool IsPureDouble() const;
	bool IsDouble() const;
	bool IsString() const;
	bool IsArray() const;
	bool IsDict() const;

	bool AsBool() const;
	int AsInt() const;
	double AsDouble() const;
	const std::string& AsString() const;
	const Array& AsArray() const;
	const Dict& AsDict() const;

	std::string& AsString();
	Array& AsArray();
	Dict& AsDict();

	bool operator==(const Node& rhs) const;

	const Value& GetValue() const;
	void Print(std::ostream& os) const;
};

bool operator!=(const Node& lhs, const Node& rhs);

Node LoadNode(std::istream& input);

}  // namespace json