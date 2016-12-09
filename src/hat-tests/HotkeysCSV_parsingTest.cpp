// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#include "../hat-core/commands_data_extraction.hpp"
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
	const auto requiredHeaderEntries("hotkey_id\thotkey_category\thotkey_note\thotkey_description\t"s);
	const auto brokenRequredHeaderEntries("hotkey_id\thotkey_category\thotkey_notE\thotkey_description\t"s);; //the required entries should be an exact text match with the expectation
	const auto correctToolsEntries0("Visual Studio\tEclipse\tAndroid Studio"s);
	const auto correctToolsEntries1("Visual Studio\tEclipse\tAndroid Studio\t"s);
	const auto correctToolsEntries2("Visual Studio\tEclipse\t"s);
	const auto incorrectToolsEntries("Visual Studio\tEclipse\t\t"s);// (2 tabs in the end - empty tool name is not allowed)

	const auto headerDescriptorRef0(hat::core::ParsedCsvRow({ "Visual Studio", "Eclipse", "Android Studio" }));
	const auto headerDescriptorRef1(headerDescriptorRef0);
	const auto headerDescriptorRef2(hat::core::ParsedCsvRow({ "Visual Studio", "Eclipse" }));
	REQUIRE(headerDescriptorRef1 != headerDescriptorRef2);
	const auto correctHeaderString0(requiredHeaderEntries + correctToolsEntries0);
	const auto correctHeaderString1(requiredHeaderEntries + correctToolsEntries1);
	const auto correctHeaderString2(requiredHeaderEntries + correctToolsEntries2);

	const auto headerDescriptor0 = hat::core::ParsedCsvRow::parseHeaderString(correctHeaderString0);
	const auto headerDescriptor1 = hat::core::ParsedCsvRow::parseHeaderString(correctHeaderString1);
	const auto headerDescriptor2 = hat::core::ParsedCsvRow::parseHeaderString(correctHeaderString2);

	REQUIRE(headerDescriptorRef0 == headerDescriptor0);
	REQUIRE(headerDescriptorRef1 == headerDescriptor1);
	REQUIRE(headerDescriptorRef2 == headerDescriptor2);

	const auto incorrectHeaderString1(requiredHeaderEntries + incorrectToolsEntries);
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
			std::vector<CommandID> commandIDs{ CommandID{"run"s}, CommandID{ "debug"s}, CommandID{ "restart"s}, CommandID{ "terminate"s} };
			testContainer.pushDataRow(hat::core::ParsedCsvRow({ commandIDs[0].getValue(),      "","r", "", "", "" }));
			testContainer.pushDataRow(hat::core::ParsedCsvRow({ commandIDs[1].getValue(),    "","d", "", "", "" }));
			testContainer.pushDataRow(hat::core::ParsedCsvRow({ commandIDs[2].getValue(),  "","t", "", "", "" }));
			testContainer.pushDataRow(hat::core::ParsedCsvRow({ commandIDs[3].getValue(),"","r", "", "", "" }));
			THEN("Commands container size should be 4.") {
				REQUIRE(testContainer.getAllCommands().size() == 4);
			}
			WHEN("Row element with too much hotkey options is added, exception is thrown.") {
				REQUIRE_THROWS(testContainer.pushDataRow(hat::core::ParsedCsvRow({ "terminate","","r", "", "", "", "" })););
			}
			WHEN("Row element with duplicate command ID is added") {
				THEN("An exception is thrown") {
					REQUIRE_THROWS(testContainer.pushDataRow(hat::core::ParsedCsvRow({ "run",		"","r" })));
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
			WHEN("Element with unknown ID is requested, an exception is thrown") {
				REQUIRE_THROWS(testContainer.getCommandPrefs(hat::core::CommandID{ "UNKNOWN_ID" }));
			}
		}
	}
}

TEST_CASE("Commands config parsing", "[csv]")
{
	std::string configData{ "hotkey_id\thotkey_category\thotkey_note\thotkey_description\tENV1\tENV2\tENV3"
					  "\nrun\tdebugger\trun\t{F5}\t{F6}\t{F7}"
					  "\ndebug\tdebugger\tdebug\t{F11}\t{F12}" };
	std::stringstream configStream(configData);

	auto testObject = hat::core::CommandsInfoContainer::parseConfigFile(configStream);

	hat::core::CommandsInfoContainer referenceObject(hat::core::ParsedCsvRow({ "ENV1"s, "ENV2"s, "ENV3"s }));
	referenceObject.pushDataRow(hat::core::ParsedCsvRow({ "run"s, "debugger"s, "run"s, "{F5}"s, "{F6}"s, "{F7}"s }));
	referenceObject.pushDataRow(hat::core::ParsedCsvRow({ "debug"s, "debugger"s, "debug"s, "{F11}"s, "{F12}"s }));
	REQUIRE(referenceObject == testObject);
}

