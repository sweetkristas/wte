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
#include <ctype.h>

#include "asserts.hpp"
#include "decimal.hpp"
#include "foreach.hpp"
#include "formula_constants.hpp"
#include "i18n.hpp"
#include "variant_utils.hpp"

namespace game_logic
{

namespace {
typedef std::map<std::string, variant> constants_map;
std::vector<constants_map> constants_stack;
}

variant get_constant(const std::string& id)
{
	//if(id == "SCREEN_WIDTH") {
	//	return variant(???);
	//} else if(id == "SCREEN_HEIGHT") {
	//	return variant(???);
	//} else
	if(id == "LOCALE") {
		return variant(i18n::get_locale());
	} else if(id == "EPSILON") {
		return variant(decimal::epsilon());
	}

	if(constants_stack.empty() == false) {
		constants_map& m = constants_stack.back();
		constants_map::const_iterator itor = m.find(id);
		if(itor != m.end()) {
			return itor->second;
		}
	}

	return variant();
}

constants_loader::constants_loader(variant node) : same_as_base_(false)
{
	constants_map m;
	if(node.is_null() == false) {
		foreach(variant key, node.get_keys().as_list()) {
			const std::string& attr = key.as_string();
			if(std::find_if(attr.begin(), attr.end(), islower) != attr.end()) {
				//only all upper case are loaded as consts
				continue;
			}

			m[attr] = node[key];
		}
	}

	if(constants_stack.empty() == false && constants_stack.back() == m) {
		same_as_base_ = true;
	}

	constants_stack.push_back(m);
}

constants_loader::~constants_loader()
{
	ASSERT_EQ(constants_stack.empty(), false);
	constants_stack.pop_back();
	//std::cerr << "REMOVE CONSTANTS_STACK\n";
}

}
