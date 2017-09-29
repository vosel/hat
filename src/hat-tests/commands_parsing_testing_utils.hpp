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

	//This function takes a string representing the contents of the typing sequences configuration file and adds this data to the given CommandsInfoContainer object.
	//Note: It creates a copy of the provided object and does all the manipulations with it. The original object's state is not changed.
	core::CommandsInfoContainer simulateAdditionalTypingSequencesConfigParsing(
		std::string const & configContents, core::CommandsInfoContainer const & sourceCommandsContainerObject);

	//This function simulates the CommandsInfoContainer object's creation.
	//First, the mandatory commands_config file is used, then more information is added from a set of optional typing_sequences configuration files.
	core::CommandsInfoContainer simulateSetOfCommandConfigFiles(
		std::string const & mainCommandsConfig, std::vector<std::string> const & additionalTypingSequencesConfigs);

} // namespace test
} // namespace hat
#endif // COMMANDS_PARSING_TESTING_UTILS