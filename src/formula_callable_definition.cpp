/* $Id: formula.cpp 25895 2008-04-17 18:57:13Z mordante $ */
/*
 Copyright (C) 2007 by David White <dave@whitevine.net>
 Part of the Silver Tree Project
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 2 or later.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY.
 
 See the COPYING file for more details.
 */
#include <map>
#include <vector>

#include "foreach.hpp"
#include "formula_callable_definition.hpp"

namespace game_logic
{

namespace
{

class simple_definition : public formula_callable_definition
{
public:
	simple_definition() : base_(NULL)
	{}

	size_t get_slot(const std::string& key) const {
		int index = 0;
		foreach(const entry& e, entries_) {
			if(e.id == key) {
				return base_num_slots() + index;
			}

			++index;
		}

		if(base_) {
			size_t result = base_->get_slot(key);
			if(result != -1) {
				return result;
			}
		}

		return -1;
	}

	entry* get_entry(size_t slot) {
		if(base_ && slot < base_num_slots()) {
			return NULL;
		}

		slot -= base_num_slots();

		if(slot < 0 || slot >= entries_.size()) {
			return NULL;
		}

		return &entries_[slot];
	}

	const entry* get_entry(size_t slot) const {
		if(base_ && slot < base_num_slots()) {
			return base_->get_entry(slot);
		}

		slot -= base_num_slots();

		if(slot < 0 || slot >= entries_.size()) {
			return NULL;
		}

		return &entries_[slot];
	}

	size_t num_slots() const { return base_num_slots() + entries_.size(); }

	void add(const std::string& id) {
		entries_.push_back(entry(id));
	}

	void set_base(const formula_callable_definition* base) { base_ = base; }

private:
	size_t base_num_slots() const { return base_ ? base_->num_slots() : 0; }
	const formula_callable_definition* base_;
	std::vector<entry> entries_;
};

}

formula_callable_definition_ptr create_formula_callable_definition(const std::string* i1, const std::string* i2, const formula_callable_definition* base)
{
	simple_definition* def = new simple_definition;
	def->set_base(base);
	while(i1 != i2) {
		def->add(*i1);
		++i1;
	}

	return formula_callable_definition_ptr(def);
}

namespace {
std::map<std::string, const formula_callable_definition*> registry;
int num_definitions = 0;
}

int register_formula_callable_definition(const std::string& id, const formula_callable_definition* def)
{
	registry[id] = def;
	return ++num_definitions;
}

const formula_callable_definition* get_formula_callable_definition(const std::string& id)
{
	return registry[id];
}

}
