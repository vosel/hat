// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#include "commands_parsing_testing_utils.hpp"
#include "../external_dependencies/Catch/single_include/catch.hpp"
#include <sstream>


namespace hat {
namespace test {

core::CommandsInfoContainer simulateParseConfigFileCall(std::string const & configContents)
{
	std::stringstream toParse(configContents);
	auto myLambda = [] (std::string const & param, core::CommandID const & commandID) {
		return std::make_shared<core::HotkeyCombination>(param);
	};

	auto simpleResult = core::CommandsInfoContainer::parseConfigFile(toParse, myLambda);
	std::string UTF8_BOM{char(0xEF), char(0xBB), char(0xBF)}; // TOOD: this constant is duplicated in several places. Maybe should put it in one place.
	std::stringstream toParse_UTF8_BOM(UTF8_BOM + configContents);
	auto utf8_result = core::CommandsInfoContainer::parseConfigFile(toParse_UTF8_BOM, myLambda);

	REQUIRE(utf8_result == simpleResult);
	return simpleResult;
}

} // namespace test
} // namespace hat