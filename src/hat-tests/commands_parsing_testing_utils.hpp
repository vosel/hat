// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef	COMMANDS_PARSING_TESTING_UTILS
#define COMMANDS_PARSING_TESTING_UTILS

#include "../hat-core/commands_data_extraction.hpp"

namespace hat {
namespace test {
	//This function creates a commands config from a given string. It also ensures that the config parsing still works fine, if there is a UTF8 BOM sequence in the beginning of the string (this sequence could be found in some of the UTF0 encoded config files).
	core::CommandsInfoContainer simulateParseConfigFileCall(std::string const & configContents);
} // namespace test
} // namespace hat
#endif	COMMANDS_PARSING_TESTING_UTILS