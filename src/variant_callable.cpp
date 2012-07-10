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
#include <vector>

#include "variant_callable.hpp"

variant variant_callable::create(variant* v)
{
	v->make_unique();
	return variant(new variant_callable(*v));
}

variant_callable::variant_callable(const variant& v) : value_(v)
{
}

variant variant_callable::get_value(const std::string& key) const
{
	if(key == "self" || key == "me") {
		return variant(this);
	}

	variant result = value_[variant(key)];
	if(result.is_list()) {
		return create_for_list(result);
	} else if(result.is_map()) {
		return variant(new variant_callable(result));
	} else {
		return result;
	}
}

variant variant_callable::create_for_list(const variant& value) const
{
	std::vector<variant> v;
	for(int n = 0; n != value.num_elements(); ++n) {
		const variant& item = value[n];
		if(item.is_list()) {
			v.push_back(create_for_list(item));
		} else if(item.is_map()) {
			v.push_back(variant(new variant_callable(item)));
		} else {
			v.push_back(item);
		}
	}

	return variant(&v);
}

void variant_callable::set_value(const std::string& key, const variant& value)
{
	value_.add_attr_mutation(variant(key), value);
}
