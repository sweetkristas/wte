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
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>

#include "foreach.hpp"
#include "formula.hpp"
#include "preprocessor.hpp"
#include "filesystem.hpp"
#include "json_parser.hpp"
#include "module.hpp"
#include "variant_callable.hpp"

std::string preprocess(const std::string& input){
	std::string output_string;
	bool in_comment = false;
	
	std::string::const_iterator i = input.begin();
	
	while(i != input.end()){
		if(*i == '#'){//enter a single-line comment
			in_comment = true;}
		if(*i == '\n' && in_comment){
			in_comment = false;}
		if(*i == '@' && !in_comment){
			// process pre-processing directive here. See what comes after the '@' and do something appropriate
			static const std::string IncludeString = "@include";
			if(size_t(input.end() - i) > IncludeString.size() && std::equal(IncludeString.begin(), IncludeString.end(), i)) {
					std::string filename_string;

					i += IncludeString.size(); //skip past the directive - we've tested that it exists
					
					//test for an argument to @include - e.g. "filename.cfg".  First the open quote:
					std::string::const_iterator quote = std::find(i, input.end(), '"');
					if(quote == input.end()) {
						std::cerr << "We didn't find a opening quote. Syntax error." << std::endl;
					}
					if(std::count_if(i, quote, isspace) != quote - i) {
					// # of whitespaces != number of intervening chars => something else was present.  Syntax Error. 
						std::cerr << "# of whitespaces != number of intervening chars." << std::endl;
					}
					i = quote + 1; //we've found a quote, advance past it
					//now the closing quote, and use it to find what's inbetween:
					std::string::const_iterator endQuote = std::find(i, input.end(), '"');
					if(endQuote == input.end()) {
						std::cerr << "We didn't find a closing quote. Syntax error." << std::endl;
					}
					
					filename_string = std::string(i, endQuote);
					
					i = endQuote + 1;
					
															
					output_string += preprocess(sys::read_file(module::map_file(filename_string)));
			}
		} else {
			//nothing special to process, just copy the chars across
			output_string.push_back(*i);
		}
		++i;
	}

	return output_string;
}

variant preprocess_string_value(const std::string& input, const game_logic::formula_callable* callable)
{
	if(input.empty() || input[0] != '@') {
		return variant(input);
	}

	if(input.size() > 1 && input[1] == '@') {
		//two @ at start of input just means a literal '@'.
		return variant(std::string(input.begin()+1, input.end()));
	}

	if(input == "@base" || input == "@derive" || input == "@call" || input == "@flatten") {
		return variant(input);
	}

	std::string::const_iterator i = std::find(input.begin(), input.end(), ' ');
	const std::string directive(input.begin(), i);
	if(directive == "@include") {
		while(i != input.end() && *i == ' ') {
			++i;
		}

		const std::string fname(i, input.end());

		//Search for a colon, which indicates an expression after the filename.
		std::string::const_iterator colon = std::find(fname.begin(), fname.end(), ':');
		if(colon != fname.end() && std::count(fname.begin(), colon, ' ') == 0) {
			const std::string file(fname.begin(), colon);
			variant doc = json::parse_from_file(module::map_file(file));
			const std::string expr(colon+1, fname.end());
			const variant expr_variant(expr);
			const game_logic::formula f(expr_variant);
			variant callable = variant_callable::create(&doc);
			const game_logic::formula_callable* vars = callable.as_callable();
			if(vars) {
				return f.execute(*vars);
			}
		}

		std::vector<std::string> includes;
		boost::split(includes, fname, std::bind2nd(std::equal_to<char>(), ' '));
		if(includes.size() == 1) {
			return json::parse_from_file(module::map_file(fname));
		} else {
			//treatment of a list of @includes is to make non-list items
			//into singleton lists, and then concatenate all lists together.
			std::vector<variant> res;
			foreach(const std::string& inc, includes) {
				variant v = json::parse_from_file(module::map_file(inc));
				if(v.is_list()) {
					foreach(const variant& item, v.as_list()) {
						res.push_back(item);
					}
				} else {
					res.push_back(v);
				}
			}

			return variant(&res);
		}
	} else if(directive == "@eval") {
		game_logic::formula f(variant(std::string(i, input.end())));
		if(callable) {
			return f.execute(*callable);
		} else {
			return f.execute();
		}
	} else {
		throw preprocessor_error();
	}
}
