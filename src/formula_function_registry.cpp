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
#include "formula_function_registry.hpp"

namespace {
std::map<std::string, std::map<std::string, function_creator*> >& function_creators()
{
	static std::map<std::string, std::map<std::string, function_creator*> > instance;
	return instance;
}

std::map<std::string, std::vector<std::string> >& helpstrings()
{
	static std::map<std::string, std::vector<std::string> > instance;
	return instance;
}
}

const std::map<std::string, function_creator*>& get_function_creators(const std::string& module)
{
	return function_creators()[module];
}

int register_function_creator(const std::string& module, const std::string& id, function_creator* creator)
{
	function_creators()[module][id] = creator;
	return function_creators()[module].size();
}

const std::vector<std::string>& function_helpstrings(const std::string& module)
{
	return helpstrings()[module];
}

int register_function_helpstring(const std::string& module, const std::string& str)
{
	helpstrings()[module].push_back(str);
	return helpstrings()[module].size();
}

