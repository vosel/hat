// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#include "commands_parsing_testing_utils.hpp"
#include "../external_dependencies/Catch/single_include/catch.hpp"
#include <sstream>


namespace hat {
namespace test {

auto getMockHotkeyCombinationProvider() {
	return [] (std::string const & param, core::CommandID const & commandID, size_t env_index) {
		return std::make_shared<core::SimpleHotkeyCombination>(param);
	};
};

auto getMockMouseInputsProvider() {
	return [] (std::string const & param, core::CommandID const & commandID, size_t env_index) {
		return std::make_shared<core::SimpleMouseInput>(param);
	};
};

auto getMockSleepInputsProvider() {
	return [] (std::string const & param, core::CommandID const & commandID, size_t env_index) {
		auto sleepTimeout = std::stoi(param); //TODO: this should be done in one operation with the creation of the SimpleSleepOperation (so that the code could be re-used with the place, where a real instance of sleep object is created)
		return std::make_shared<core::SimpleSleepOperation>(param, sleepTimeout, true);
	};
};

core::CommandsInfoContainer simulateParseConfigFileCall(std::string const & configContents, std::function <core::CommandsInfoContainer (std::istream & srcStream)> objectProviderCallback)
{	
	std::stringstream toParse(configContents);
	auto simpleResult = objectProviderCallback(toParse);
	std::string UTF8_BOM{char(0xEF), char(0xBB), char(0xBF)}; // TODO: this constant is duplicated in several places. Maybe should put it in one place.
	std::stringstream toParse_UTF8_BOM(UTF8_BOM + configContents);
	auto utf8_result = objectProviderCallback(toParse_UTF8_BOM);
	REQUIRE(utf8_result == simpleResult);
	return simpleResult;
}

core::CommandsInfoContainer simulateParseConfigFileCall(std::string const & configContents)
{
	return simulateParseConfigFileCall(configContents, [](std::istream & dataToProcess) {
		return core::CommandsInfoContainer::parseConfigFile(dataToProcess, getMockHotkeyCombinationProvider());
	});
}

core::CommandsInfoContainer simulateUI_variablesOperationsConfigParsing(
	std::string const & configContents, core::CommandsInfoContainer const & sourceCommandsContainerObject)
{
	return simulateParseConfigFileCall(configContents, [&](std::istream & dataToProcess) {
		core::CommandsInfoContainer result = sourceCommandsContainerObject;
		result.consumeVariablesManagersConfig(dataToProcess);
		return result;
	});
}

//Note: the result returned by this function is not the same object, that was passed into it. The result object is copy-constructed inside this function.
core::CommandsInfoContainer simulateAdditionalInputSequencesConfigParsing(
		std::string const & configContents, core::CommandsInfoContainer const & sourceCommandsContainerObject)
{
	return simulateParseConfigFileCall(configContents, [&](std::istream & dataToProcess) {
		core::CommandsInfoContainer result = sourceCommandsContainerObject;
		result.consumeInputSequencesConfigFile(dataToProcess, getMockHotkeyCombinationProvider(), getMockMouseInputsProvider(), getMockSleepInputsProvider());
		return result;
	});
}

core::CommandsInfoContainer simulateSetOfCommandConfigFiles(
	std::string const & mainCommandsConfig, std::vector<std::string> const & additionalTypingSequencesConfigs)
{
	auto result = simulateParseConfigFileCall(mainCommandsConfig);

	for (auto & typingSequencesFileData : additionalTypingSequencesConfigs) {
		result = simulateAdditionalInputSequencesConfigParsing(typingSequencesFileData, result);
	}
	return result;
}


} // namespace test
} // namespace hat