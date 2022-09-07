#pragma once

#include "json.h"

#include <vector>
#include <stdexcept>
#include <string>

namespace json {

namespace json_builder {

struct Dictionary {
	json::Dict dictionary;
	std::string current_key;
};

enum class CollectionState
{
	Array,
	Dict
};

} // namespace json_builder

class Builder;
class DictValBuilder;
class DictBuilder;
class ArrayBuilder;

DictValBuilder& CastToDictValBuilder(Builder& b);
DictBuilder& CastToDictBuilder(Builder& b);
ArrayBuilder& CastToArrayBuilder(Builder& b);

class Builder {

public:

	Builder& Value(json::Node::Value value);

	ArrayBuilder& StartArray();
	Builder& EndArray();

	DictBuilder& StartDict();
	DictValBuilder& Key(std::string key);
	Builder& EndDict();

	json::Node Build();

private:
	bool NeedDictionaryKey();
	bool IsArrayState();

	void CheckNotReady();
	void CheckNeedDictionaryKey();
	void CheckNotNeedDictionaryKey();

	void StartInsertValue();

	void AddNode(json::Node&& node);

	json::Node node_;

	std::vector<json::Array> arrays_;
	std::vector<json_builder::Dictionary> dictionaries_;
	std::vector<json_builder::CollectionState> collection_states_;

	bool ready_state_ = false;
	bool key_was_called_ = false;
};

class DictValBuilder : public Builder {
public:
	DictBuilder& Value(json::Node::Value value);
private:
	using Builder::Build;
	using Builder::EndArray;
	using Builder::EndDict;
	using Builder::Key;
};

class DictBuilder : public Builder {
private:
	using Builder::Build;
	using Builder::Value;
	using Builder::StartArray;
	using Builder::EndArray;
	using Builder::StartDict;
};

class ArrayBuilder : public Builder {
public:
	ArrayBuilder& Value(json::Node::Value value);
private:
	using Builder::Build;
	using Builder::EndDict;
	using Builder::Key;
};

} //namespace json