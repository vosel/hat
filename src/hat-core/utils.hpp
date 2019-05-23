// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <istream>
namespace hat {
namespace core {
	std::istream & getLineFromFile(std::istream & filestream, std::string & target);
	std::string clearUTF8_byteOrderMark(std::string const & firstLineOfFile);
	std::string escapeRawUTF8_forJson(std::string const & stringToProcess);
	bool isSvgFile(std::string const & file_path);
	std::string loadSvgFromFile(std::string const & file_path);
} //namespace core
} //namespace hat

#ifdef HAT_CORE_HEADERONLY_MODE
#include "utils.cpp"
#endif //HAT_CORE_HEADERONLY_MODE

#endif //UTILS_HPP