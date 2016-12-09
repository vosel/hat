// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef HAT_CORE_HEADERONLY_MODE
#include "utils.hpp"
#endif

#ifndef HAT_CORE_HEADERONLY_MODE
#define LINKAGE_RESTRICTION 
#else
#define LINKAGE_RESTRICTION inline
#endif

namespace hat {
namespace core {

LINKAGE_RESTRICTION std::istream & getLineFromFile(std::istream & filestream, std::string & target)
{
	std::getline(filestream, target);
	if ((target.size() > 0) && (target.back() == '\r')) {
		target = target.substr(0, target.size() - 1);
	}
	return filestream;
}

} //namespace core
} //namespace hat

#undef LINKAGE_RESTRICTION