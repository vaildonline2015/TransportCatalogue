#include "json.h"
#include "json_builder.h"

#include <vector>
#include <stdexcept>
#include <string>

namespace json {

using namespace std::literals::string_literals;
using namespace json_builder;

Builder& Builder::Value(json::Node::Value value) {
	StartInsertValue();
	AddNode(json::Node{ move(value) });
	return *this;
}

ArrayBuilder& Builder::StartArray() {
	StartInsertValue();
	arrays_.emplace_back();
	collection_states_.push_back(CollectionState::Array);
	return CastToArrayBuilder(*this);
}

Builder& Builder::EndArray() {
	CheckNotReady();
	if (!IsArrayState()) {
		throw std::logic_error("Current node isn't array. Error EndArray()"s);
	}
	collection_states_.pop_back();

	json::Array array_for_add = move(arrays_.back());
	arrays_.pop_back();
	AddNode(move(array_for_add));

	return *this;
}

DictBuilder& Builder::StartDict() {
	StartInsertValue();
	dictionaries_.emplace_back();
	collection_states_.push_back(CollectionState::Dict);
	return CastToDictBuilder(*this);
}

DictValBuilder& Builder::Key(std::string key) {
	CheckNotReady();
	CheckNeedDictionaryKey();

	dictionaries_.back().current_key = key;
	dictionaries_.back().dictionary[move(key)];
	key_was_called_ = true;

	return CastToDictValBuilder(*this);
}

Builder& Builder::EndDict() {
	CheckNotReady();
	CheckNeedDictionaryKey();

	collection_states_.pop_back();

	json::Dict dict_for_add = move(dictionaries_.back().dictionary);
	dictionaries_.pop_back();
	AddNode(move(dict_for_add));
	return *this;
}

json::Node Builder::Build() {
	if (!ready_state_) {
		throw std::logic_error("Node not ready for build"s);
	}
	return node_;
}

void Builder::CheckNotReady() {
	if (ready_state_) {
		throw std::logic_error("Attempt to change ready object"s);
	}
}

bool Builder::NeedDictionaryKey() {
	if (!collection_states_.empty()) {
		if (collection_states_.back() == CollectionState::Dict) {
			if (!key_was_called_) {
				return true;
			}
		}
	}
	return false;
}

void Builder::CheckNeedDictionaryKey() {
	if (!NeedDictionaryKey()) {
		throw std::logic_error("Object not expect dictionary Key() or EndDict()"s);
	}
}
void Builder::CheckNotNeedDictionaryKey() {
	if (NeedDictionaryKey()) {
		throw std::logic_error("Object expect dictionary key"s);
	}
}

bool Builder::IsArrayState() {
	if (!collection_states_.empty()) {
		if (collection_states_.back() == CollectionState::Array) {
			return true;
		}
	}
	return false;
}

void Builder::StartInsertValue() {
	CheckNotReady();
	CheckNotNeedDictionaryKey();
	if (key_was_called_) {
		key_was_called_ = false;
	}
}

void Builder::AddNode(json::Node&& node) {

	if (collection_states_.empty()) {
		ready_state_ = true;
		node_ = move(node);
	}
	else if (collection_states_.back() == CollectionState::Array) {
		arrays_.back().push_back(move(node));
	}
	else if (collection_states_.back() == CollectionState::Dict) {
		dictionaries_.back().dictionary[dictionaries_.back().current_key] = move(node);
	}
}

DictBuilder& DictValBuilder::Value(json::Node::Value value) {
	return CastToDictBuilder(Builder::Value(value));
}

ArrayBuilder& ArrayBuilder::Value(json::Node::Value value) {
	return CastToArrayBuilder(Builder::Value(value));
}

DictValBuilder& CastToDictValBuilder(Builder& b) {
	return static_cast<DictValBuilder&>(b);
}

DictBuilder& CastToDictBuilder(Builder& b) {
	return static_cast<DictBuilder&>(b);
}

ArrayBuilder& CastToArrayBuilder(Builder& b) {
	return static_cast<ArrayBuilder&>(b);
}

} //namespace json