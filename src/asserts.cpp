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
#include "asserts.hpp"
#include "variant.hpp"

#if defined(_WINDOWS)
#include "SDL_syswm.h"
#endif

void report_assert_msg(const std::string& m)
{
#if defined(__ANDROID__)
	__android_log_print(ANDROID_LOG_INFO, "wte", m.c_str());

#endif
	
#if defined(_WINDOWS)
	SDL_SysWMinfo SysInfo;
	SDL_VERSION(&SysInfo.version);
	if(SDL_GetWMInfo(&SysInfo) > 0) {
		::MessageBoxA(SysInfo.window, m.c_str(), "Assertion failed", MB_OK|MB_ICONSTOP);
	}
#endif
}

validation_failure_exception::validation_failure_exception(const std::string& m)
  : msg(m)
{
	std::cerr << "ASSERT FAIL: " << m << "\n";
}

namespace {
	int throw_validation_failure = 0;
}

bool throw_validation_failure_on_assert()
{
	return throw_validation_failure != 0;
}

assert_recover_scope::assert_recover_scope()
{
	throw_validation_failure++;
}

assert_recover_scope::~assert_recover_scope()
{
	throw_validation_failure--;
}

void output_backtrace()
{
	std::cerr << get_call_stack() << "\n";
}
