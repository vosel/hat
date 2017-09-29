// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#include "../hat-core/commands_data_extraction.hpp"
#include "commands_parsing_testing_utils.hpp"
#include <sstream>
#include <string>
#include <vector>
#include <map>

#include "../external_dependencies/Catch/single_include/catch.hpp"

using namespace std::string_literals;
namespace Catch {
template<> std::string toString(hat::core::ParsedCsvRow const & toDump)
{
	std::stringstream result;
	result << '[' << toDump.m_customColumns.size() << "]:{";
	for (auto const & customColumnName : toDump.m_customColumns) {
		result << customColumnName << ", ";
	}
	result << '}';
	return result.str();
}
}

//Simple test case, which ensures that header strings are parsed correctly
TEST_CASE("csv header parser testing", "[csv]")
{
	using namespace std::string_literals;
	const auto & correctRequiredHeaderEntries = hat::core::ConfigFilesKeywords::mandatoryCellsNamesInCommandsCSV();
	const auto brokenRequredHeaderEntries("command_id\tcommand_category\tcommand_notE\tcommand_description\t"s);; //the required entries should be an exact text match with the expectation
	const auto correctToolsEntries0("Visual Studio\tEclipse\tAndroid Studio"s);
	const auto correctToolsEntries1("Visual Studio\tEclipse\tAndroid Studio\t"s);
	const auto correctToolsEntries2("Visual Studio\tEclipse\t"s);
	const auto incorrectToolsEntries("Visual Studio\tEclipse\t\t"s);// (2 tabs in the end - empty tool name is not allowed)

	const auto headerDescriptorRef0(hat::core::ParsedCsvRow({ "Visual Studio", "Eclipse", "Android Studio" }));
	const auto headerDescriptorRef1(headerDescriptorRef0);
	const auto headerDescriptorRef2(hat::core::ParsedCsvRow({ "Visual Studio", "Eclipse" }));
	REQUIRE(headerDescriptorRef1 != headerDescriptorRef2);
	const auto correctHeaderString0(correctRequiredHeaderEntries + correctToolsEntries0);
	const auto correctHeaderString1(correctRequiredHeaderEntries + correctToolsEntries1);
	const auto correctHeaderString2(correctRequiredHeaderEntries + correctToolsEntries2);

	const auto headerDescriptor0 = hat::core::ParsedCsvRow::parseHeaderString(correctHeaderString0);
	const auto headerDescriptor1 = hat::core::ParsedCsvRow::parseHeaderString(correctHeaderString1);
	const auto headerDescriptor2 = hat::core::ParsedCsvRow::parseHeaderString(correctHeaderString2);

	REQUIRE(headerDescriptorRef0 == headerDescriptor0);
	REQUIRE(headerDescriptorRef1 == headerDescriptor1);
	REQUIRE(headerDescriptorRef2 == headerDescriptor2);

	const auto incorrectHeaderString1(correctRequiredHeaderEntries + incorrectToolsEntries);
	const auto incorrectHeaderString2(brokenRequredHeaderEntries + correctToolsEntries0);
	REQUIRE_THROWS(hat::core::ParsedCsvRow::parseHeaderString(incorrectHeaderString1)); //todo: add exception checker here (to make sure that the correct exception is thrown)
	//TODO USE REQUIRE_THROWS_AS() here
	REQUIRE_THROWS(hat::core::ParsedCsvRow::parseHeaderString(incorrectHeaderString2)); //todo: add exception checker here (to make sure that the correct exception is thrown)
}

TEST_CASE("csv row parser tests", "[csv]")
{
	const auto correctRowString0("run\texecution\trun\tcomment for the entry 1\t^{F5}\t^{F11}\t+{F10}\t"); // all data present
	const auto rowDescriptorRef0(hat::core::ParsedCsvRow({ "run", "execution", "run", "comment for the entry 1", "^{F5}", "^{F11}", "+{F10}" }));
	auto rowDescriptor0 = hat::core::ParsedCsvRow::parseDataRowString(correctRowString0);
	REQUIRE(rowDescriptor0 == rowDescriptorRef0);

	const auto correctRowString1("run\t\trun\tcomment for the entry 2\t^{F5}\t\t+{F10}\t"); // some data missing, still correct
	const auto rowDescriptorRef1(hat::core::ParsedCsvRow({ "run", "", "run", "comment for the entry 2", "^{F5}", "", "+{F10}" }));
	auto rowDescriptor1 = hat::core::ParsedCsvRow::parseDataRowString(correctRowString1);
	REQUIRE(rowDescriptor1 == rowDescriptorRef1);

	const auto incorrectRowString0("ru n\texecution\trun\tcomment1\t^{F5}\t^{F11}\t+{F10}\t"); // problem with the required parameter: forbidden symbol in 'ID' cell
	const auto incorrectRowString1("run\texecut ion\trun\tcomment1\t^{F5}\t^{F11}\t+{F10}\t"); // problem with the required parameter: forbidden symbol in 'category' cell
	const auto incorrectRowString2("\texecution\trun\tcomment1\t^{F5}\t^{F11}\t+{F10}\t"); // problem with the required parameter: empty 'ID' cell (ID should be a unique non-empty string)
	const auto incorrectRowString3("run\texecution\t\tcomment1\t^{F5}\t^{F11}\t+{F10}\t"); // problem with the required parameter: empty 'text' cell

	REQUIRE_THROWS(hat::core::ParsedCsvRow::parseDataRowString(incorrectRowString0)); //todo: add exception checker here (to make sure that the correct exception is thrown)
	REQUIRE_THROWS(hat::core::ParsedCsvRow::parseDataRowString(incorrectRowString1)); //todo: add exception checker here (to make sure that the correct exception is thrown)
	REQUIRE_THROWS(hat::core::ParsedCsvRow::parseDataRowString(incorrectRowString2)); //todo: add exception checker here (to make sure that the correct exception is thrown)
	REQUIRE_THROWS(hat::core::ParsedCsvRow::parseDataRowString(incorrectRowString3)); //todo: add exception checker here (to make sure that the correct exception is thrown)
}

SCENARIO("csv data presentation tests", "[csv]")
{
	using hat::core::CommandID;
	GIVEN("A CommandsInfoContainer with 3 environments") {
		std::vector<std::string> ENVs{ "Visual Studio"s, "Eclipse"s, "Android Studio"s };
		hat::core::CommandsInfoContainer testContainer{ hat::core::ParsedCsvRow(ENVs) };
		auto supportedENVs = testContainer.getEnvironments();
		REQUIRE(supportedENVs.size() == 3);
		REQUIRE(supportedENVs[0] == ENVs[0]);
		REQUIRE(supportedENVs[1] == ENVs[1]);
		REQUIRE(supportedENVs[2] == ENVs[2]);
		WHEN("4 row items are added") {
			auto throwawayCommandID = CommandID{"throwaway_command_id"s}; // The command id object, which is used for testing the situations when exception is thrown. The commands with this id should not get into the tested container object.
			std::vector<CommandID> commandIDs{ CommandID{"run"s}, CommandID{ "debug"s}, CommandID{ "restart"s}, CommandID{ "terminate"s} };
			testContainer.pushDataRow(hat::core::ParsedCsvRow({ commandIDs[0].getValue(), "", "r", "", "", "" }));
			testContainer.pushDataRow(hat::core::ParsedCsvRow({ commandIDs[1].getValue(), "", "d", "", "", "" }));
			testContainer.pushDataRow(hat::core::ParsedCsvRow({ commandIDs[2].getValue(), "", "r", "", "", "" }));
			testContainer.pushDataRow(hat::core::ParsedCsvRow({ commandIDs[3].getValue(), "", "t", "", "", "" }));
			THEN("Commands container's size should be 4.") {
				REQUIRE(testContainer.getAllCommands().size() == 4);
			}
			WHEN("Row element with too much hotkey options is added") {
				THEN("An exception is thrown") {
					REQUIRE_THROWS(testContainer.pushDataRow(hat::core::ParsedCsvRow({ throwawayCommandID.getValue(), "", "r", "", "", "", "", "" })););
				}
			}
			WHEN("Row element with duplicate command ID is added") {
				THEN("An exception is thrown") {
					REQUIRE_THROWS(testContainer.pushDataRow(hat::core::ParsedCsvRow({ commandIDs[0].getValue(), "","r" })));
				}
			}
			WHEN("Element with specific ID is requested") {
				for (auto const & current_commandID : commandIDs) {
					auto elem = testContainer.getCommandPrefs(current_commandID);
					THEN("Appropriate element is returned") {
						elem.commandID == current_commandID;
					}
				}
			}

			WHEN("4 simple typing sequences objects added") {
				std::string const commonMandatoryDataForCommandRows {
					"\tcommand_test_category\tcommand test note\tcommand test description\t" };
				
				std::vector<std::string> environmentsForCommands = {"*"s, ""s,
					ENVs[0] + "," + ENVs[1],
					ENVs[0] + "," + ENVs[1] + "," + ENVs[2] };
				std::vector<CommandID> typingSequencesCommandIDs{ 
					CommandID{"test1"s}, CommandID{ "test2"s}, CommandID{ "test3"s}, CommandID{ "test4"s} };

				std::stringstream simpleTypingSequencesStream;
				for (size_t i = 0; i < typingSequencesCommandIDs.size(); ++i) {
					simpleTypingSequencesStream << hat::core::ConfigFilesKeywords::simpleTypingSeqCommand() << "\t"
						<< typingSequencesCommandIDs[i].getValue() << commonMandatoryDataForCommandRows
						<< environmentsForCommands[i] << "\t" << "someCommandSampleText" << "\n";
				}

				testContainer = hat::test::simulateAdditionalTypingSequencesConfigParsing(
					simpleTypingSequencesStream.str(), testContainer);

				THEN("Commands container's size should become 8.") {
					REQUIRE(testContainer.getAllCommands().size() == 8);
				}

				std::vector<CommandID> commandsAggregatorsIDs{ 
					CommandID{ "test_aggregator_1"s},
					CommandID{ "test_aggregator_2"s}
				};
				auto throwawayCommandID_forAggregatedCommand = CommandID{"throwaway_aggregator"s};
				auto & allEnvironments = environmentsForCommands[0];
				auto util_createAggregatedCommandDeclaration = [&allEnvironments, &commonMandatoryDataForCommandRows](CommandID const & idToUse,
					std::vector<CommandID> const & listOfAggregatedCommands)
				{
					std::stringstream mystream;
					mystream << hat::core::ConfigFilesKeywords::aggregatedTypingSeqCommand() << "\t"
						<< idToUse.getValue() << commonMandatoryDataForCommandRows
						<< allEnvironments << "\t";

					for (auto & command : listOfAggregatedCommands) {
						mystream << command.getValue() << ',';
					}
					return mystream.str();
				};

				WHEN("Complex command referencing known id's is added") {
					auto rowText = util_createAggregatedCommandDeclaration(
						commandsAggregatorsIDs[0], 
						{commandIDs[0], commandIDs[1], commandIDs[0], commandIDs[0]} 	//Note: we intentially use the same command id several times here. It is a situation, which is possible - we could want to repeat the same command several times.
					);
					testContainer =
						hat::test::simulateAdditionalTypingSequencesConfigParsing(rowText, testContainer);
					THEN("Commands container's size should become 9.") {
						REQUIRE(testContainer.getAllCommands().size() == 9);
					}
				
					WHEN("Complex command referencing known id's (one of which is another complex command) is added") {
						auto rowText = util_createAggregatedCommandDeclaration(
							commandsAggregatorsIDs[1], 
							{ commandIDs[0], commandsAggregatorsIDs[0]}
						);
						testContainer = hat::test::simulateAdditionalTypingSequencesConfigParsing(
							rowText, testContainer);
						THEN("Commands container's size should become 10.") {
							REQUIRE(testContainer.getAllCommands().size() == 10);
						}
					}
					WHEN("Complex command with unknown id is added") {
						auto rowText = util_createAggregatedCommandDeclaration(
							throwawayCommandID_forAggregatedCommand, 
							{commandIDs[0], throwawayCommandID}
						);
						THEN("Exception is thrown.") {
							REQUIRE_THROWS(
								hat::test::simulateAdditionalTypingSequencesConfigParsing(
									rowText, testContainer);
							);
						}
					}
				}
			}
			WHEN("Element with unknown ID is requested, an exception is thrown") {
				REQUIRE_THROWS(testContainer.getCommandPrefs(throwawayCommandID));
			}
		}
	}
}

TEST_CASE("Commands config parsing", "[csv]")
{
	std::string configData{ hat::core::ConfigFilesKeywords::mandatoryCellsNamesInCommandsCSV() + "ENV1\tENV2\tENV3"
		"\nrun\tdebugger\trun\ttest_desc\t{F5}\t{F6}\t{F7}"
		"\ndebug\tdebugger\tdebug\ttest_desc\t{F11}\t{F12}" };

	auto testObject = hat::test::simulateParseConfigFileCall(configData);

	hat::core::CommandsInfoContainer referenceObject(hat::core::ParsedCsvRow({ "ENV1"s, "ENV2"s, "ENV3"s }));
	referenceObject.pushDataRow(hat::core::ParsedCsvRow({ "run"s, "debugger"s, "run"s, "test_desc"s, "{F5}"s, "{F6}"s, "{F7}"s }));
	referenceObject.pushDataRow(hat::core::ParsedCsvRow({ "debug"s, "debugger"s, "debug"s, "test_desc"s, "{F11}"s, "{F12}"s }));
	REQUIRE(referenceObject == testObject);
}

SCENARIO("Equivalence of commands_config and typing_sequences files", "[csv_commands vs typing_sequences]")
{
	GIVEN ("A set of test configuration data") {
		std::vector<std::string> commands_ids = {
			"id1"s,
			"id2"s,
			"id3"s,
			"id4"s,
			"id5"s,
			"id6"s
		};
		std::vector<std::string> commands_categories = {
			"test_category"s, 
			"test_category"s, 
			"test_category"s, 
			"test_category"s, 
			"test_category"s, 
			"test_category"s
		};
		std::vector<std::string> commands_notes = {
			"test note"s, 
			"test note"s, 
			"test note"s, 
			"test note"s, 
			"test note"s, 
			"test note"s
		};
		std::vector<std::string> commands_descriptions = {
			"test description"s, 
			"test description"s, 
			"test description"s, 
			"test description"s, 
			"test description"s, 
			"test description"s 
		};
		std::vector<std::string> environments = {
			"ENV0",
			"ENV1",
			"ENV2"
		};
		// Note: the commands data values here should be the same for all of the environments. Since we are testing the typing sequences file reading, which provides the same value for the command for each of the environments (for which it is enabled).
		// So, here we are providing all the needed data: string represents the value of the command for given entry, and the vector of flags, which say if this value is specified to the given environment.
		std::vector<std::pair<std::string, std::vector<char>>> commands_data_for_environments = {
			{"{F1}"s, {1, 1, 1}},
			{"{F2}"s, {1, 1, 0}},
			{"{F3}"s, {1, 0, 1}},
			{"{F4}"s, {1, 0, 0}},
			{"{F5}"s, {0, 1, 1}},
			{"{F6}"s, {0, 0, 0}}
		};
		WHEN ("a normal csv_config and typing_sequences configuration files with this data are used as source for the CommandsInfoContainer objects") {
			std::stringstream commonCsvHeader_stream;
			commonCsvHeader_stream << hat::core::ConfigFilesKeywords::mandatoryCellsNamesInCommandsCSV();
			for (auto & env : environments) { commonCsvHeader_stream << env << "\t";}
			std::string const commonCsvHeader = commonCsvHeader_stream.str();

			std::stringstream csvConfigData;
			csvConfigData << commonCsvHeader;
			for (size_t i = 0; i < commands_ids.size(); ++i) {
				csvConfigData << "\n" << commands_ids[i] << "\t"
					<< commands_categories[i] << "\t"
					<< commands_notes[i] << "\t"
					<< commands_descriptions[i];
				for (auto isCommandEnabled : commands_data_for_environments[i].second) {
					csvConfigData << "\t";
					if (1 == isCommandEnabled) {
						csvConfigData << commands_data_for_environments[i].first;
					}
				}
			}

			std::stringstream typingSequencesConfigData;
			for (size_t i = 0; i < commands_ids.size(); ++i) {
				typingSequencesConfigData << hat::core::ConfigFilesKeywords::simpleTypingSeqCommand() << "\t"
					<< commands_ids[i] << "\t"
					<< commands_categories[i] << "\t"
					<< commands_notes[i] << "\t"
					<< commands_descriptions[i] << "\t";
				{
					std::stringstream environmentsStringBuilder;
					auto shouldAddComma = false;
					for (size_t environment_index = 0; environment_index < commands_data_for_environments[i].second.size(); ++environment_index) {
						if (commands_data_for_environments[i].second[environment_index] == 1) {
							if (shouldAddComma) {
								environmentsStringBuilder << ',';
							}						
							environmentsStringBuilder << environments[environment_index];
							shouldAddComma = true;
						}
					}
					typingSequencesConfigData << environmentsStringBuilder.str();
				}
				typingSequencesConfigData << "\t" << commands_data_for_environments[i].first << "\n";
			}

			THEN ("the resulting objects loaded from these configs should be equivalent") {
				auto testObject0 = hat::test::simulateParseConfigFileCall(csvConfigData.str());
				auto testObject1 = hat::test::simulateSetOfCommandConfigFiles(commonCsvHeader, { typingSequencesConfigData.str() });
				REQUIRE(testObject0 == testObject1);
			}
		}
	}
}

TEST_CASE("Order of the environmens in typing_sequences configuration file")
{
	std::string const commonHeader {hat::core::ConfigFilesKeywords::mandatoryCellsNamesInCommandsCSV() + "ENV0\tENV1\tENV2"s};
	auto runTestingOfEquivalencesAndDifferences = [&commonHeader](
		std::string const & referenceEnvironmentsSetup,
		std::vector<std::string> const & equivalentEnvironmentsSetups,
		std::vector<std::string> const & differentEnvironmentsSetups) {
		auto buildConfigFileAndConsumeIt = [&commonHeader] (std::string const & environmentString)
		{
			auto const commandID = std::string { "testCommandID"};
			auto const commandCategory = std::string { "testCommandCategory"};
			auto const commandNote = std::string { "test note"};
			auto const commandDescription = std::string { "test desc"};
			auto const commandStringPostfix = std::string {"testCommandData"};
			std::stringstream mystream;
			mystream 
				<< hat::core::ConfigFilesKeywords::simpleTypingSeqCommand() << "\t"
				<< commandID << "\t" 
				<< commandCategory << "\t" 
				<< commandNote << "\t" 
				<< commandDescription << "\t"
				<< environmentString << "\t"
				<< commandStringPostfix;
			std::string result = mystream.str();
			return std::pair<hat::core::CommandsInfoContainer, std::string>{
				hat::test::simulateSetOfCommandConfigFiles(commonHeader, { result }), result};
		};
		auto const buildResultForReferenceEnvironment = buildConfigFileAndConsumeIt(referenceEnvironmentsSetup);
		for (auto & testedEquivalentEnv : equivalentEnvironmentsSetups) {
			auto buildResult_equivalentEnvironmentSetup = buildConfigFileAndConsumeIt(testedEquivalentEnv);
			REQUIRE_FALSE(buildResultForReferenceEnvironment.second == buildResult_equivalentEnvironmentSetup.second); // Sanity check: test that the config files are actually different.
			REQUIRE(buildResultForReferenceEnvironment.first == buildResult_equivalentEnvironmentSetup.first);

		}
		for (auto & testedDifferentEnv : differentEnvironmentsSetups) {
			auto buildResult_differentEnvironmentSetup = buildConfigFileAndConsumeIt(testedDifferentEnv);
			//Note: We don't need the sanity check here (see the previous loop). Since we are testing inequality, if this test passes, the configs will be different automatically.
			REQUIRE_FALSE(buildResultForReferenceEnvironment.first == buildResult_differentEnvironmentSetup.first);
		}
	};

	runTestingOfEquivalencesAndDifferences(
		""s,
		{ }, //Note: 'no environment' configuration string can't be presented in any different way, so this parameter is empty
		{"ENV0,ENV1,ENV2"s, "*"s, "ENV1,ENV0"s, "ENV1"s, "ENV2"s});

	runTestingOfEquivalencesAndDifferences(
		"ENV0"s,
		{ }, //Note: single-environment configuration string can't be presented in any different way, so this parameter is empty
		{"ENV0,ENV1,ENV2"s, "*"s, "ENV1,ENV0"s, "ENV1"s, "ENV2"s, ""s});

	runTestingOfEquivalencesAndDifferences(
		"ENV0,ENV1"s,
		{ "ENV1,ENV0"s},
		{ "ENV1,ENV2"s, "ENV0,ENV1,ENV2"s, "*"s, "ENV0"s, "ENV1"s, "ENV2"s, ""s});

	runTestingOfEquivalencesAndDifferences("*"s,
		{"ENV0,ENV1,ENV2"s, "ENV1,ENV0,ENV2"s},
		{"ENV0,ENV1"s, "ENV0"s, "ENV1"s, "ENV2"s, ""s});
}

TEST_CASE("Misspelled environment identifier in typing_sequence configuration files")
{
	//This is a very simple test case to ensure that the situations with duplicated/misspelled environment IDs are pointed out by the system.
	std::string const csv_header {hat::core::ConfigFilesKeywords::mandatoryCellsNamesInCommandsCSV() + "ENV0\tENV1\tENV2"s};
	auto testRunner = [&csv_header](std::string const & wrongEnvironmentsString, std::string const & correctedEnvironmentsString)
	{
		std::string const typing_sequence_element_beginning {
			hat::core::ConfigFilesKeywords::simpleTypingSeqCommand() + 
				"\ttestCommandID\ttestCommandCategory\ttest note\ttest desc\t"
		};
		std::string const typing_sequence_element_ending {"\ttestData"};
		std::string wrongTypingSeqLine = 
			typing_sequence_element_beginning + wrongEnvironmentsString + typing_sequence_element_ending;
		std::string correctedTypingSeqLine = 
			typing_sequence_element_beginning + correctedEnvironmentsString + typing_sequence_element_ending;
		REQUIRE_THROWS(hat::test::simulateSetOfCommandConfigFiles(csv_header, {wrongTypingSeqLine}));
		REQUIRE_NOTHROW(hat::test::simulateSetOfCommandConfigFiles(csv_header, {correctedTypingSeqLine}));
	};

	testRunner("env0", "ENV0");
	testRunner("ENV0,env1", "ENV0,ENV1");
	testRunner("env1,ENV0", "ENV0,ENV1");
}
