// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef _UTILS_HPP
#define _UTILS_HPP

#include <string>
#include <istream>
namespace hat {
namespace core {
	std::istream & getLineFromFile(std::istream & filestream, std::string & target);
} //namespace core
} //namespace hat

#ifdef HAT_CORE_HEADERONLY_MODE
#include "utils.cpp"
#endif //HAT_CORE_HEADERONLY_MODE

#endif //_UTILS_HPP