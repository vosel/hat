// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#include "layout_parsing_verificator.hpp"
#include "../hat-core/configs_abstraction_layer.hpp"
#include <sstream>
#include <vector>
#include "../external_dependencies/Catch/single_include/catch.hpp"

using namespace std::string_literals;

namespace {
//this is a config file for commands. It contains all the possible combinations of commands definitions for 4 ENVs
const auto commandsConfiguration = std::string{
	"command_id\tcommand_category\tcommand_note\tcommand_description\tENV3\tENV2\tENV1\tENV0\n"   //command defined for:
	"hk0\tcategory\thk0_note\thk0_desc\t\t\n"                                                      //none of the ENVs
	"hk1\tcategory\thk1_note\thk1_desc\t\t\t\thk1_keys_0\n"                                        //ENV0
	"hk2\tcategory\thk2_note\thk2_desc\t\t\thk2_keys_1\t\n"                                        //ENV1
	"hk3\tcategory\thk3_note\thk3_desc\t\t\thk3_keys_1\thk3_keys_0\n"                              //ENV1, ENV0
	"hk4\tcategory\thk4_note\thk4_desc\t\thk4_keys_2\t\t\n"                                        //ENV2
	"hk5\tcategory\thk5_note\thk5_desc\t\thk5_keys_2\t\thk5_keys_0\n"                              //ENV2, ENV0
	"hk6\tcategory\thk6_note\thk6_desc\t\thk6_keys_2\thk6_keys_1\t\n"                              //ENV2, ENV1
	"hk7\tcategory\thk7_note\thk7_desc\t\thk7_keys_2\thk7_keys_1\thk7_keys_0\n"                    //ENV2, ENV1, ENV0
	"hk8\tcategory\thk8_note\thk8_desc\thk8_keys_3\t\n"                                            //ENV4
	"hk9\tcategory\thk9_note\thk9_desc\thk9_keys_3\t\t\thk9_keys_0\n"                              //ENV4, ENV0
	"hkA\tcategory\thkA_note\thkA_desc\thkA_keys_3\t\thkA_keys_1\t\n"                              //ENV4, ENV1
	"hkB\tcategory\thkB_note\thkB_desc\thkB_keys_3\t\thkB_keys_1\thkB_keys_0\n"                    //ENV4, ENV1, ENV0
	"hkC\tcategory\thkC_note\thkC_desc\thkC_keys_3\thkC_keys_2\t\t\n"                              //ENV4, ENV2
	"hkD\tcategory\thkD_note\thkD_desc\thkD_keys_3\thkD_keys_2\t\thkD_keys_0\n"                    //ENV4, ENV2, ENV0
	"hkE\tcategory\thkE_note\thkE_desc\thkE_keys_3\thkE_keys_2\thkE_keys_1\t\n"                    //ENV4, ENV2, ENV1
	"hkF\tcategory\thkF_note\thkF_desc\thkF_keys_3\thkF_keys_2\thkF_keys_1\thkF_keys_0\n"          //ENV4, ENV2, ENV1, ENV0
};
std::stringstream commandsStream{ commandsConfiguration };
const auto COMMANDS_INFO_CONTAINER = hat::core::CommandsInfoContainer::parseConfigFile(commandsStream);

const auto ENV0 = size_t{ 3 };
const auto ENV1 = size_t{ 2 };
const auto ENV2 = size_t{ 1 };
const auto ENV3 = size_t{ 0 };
const auto ENVs = std::vector<std::string>{ "ENV3"s, "ENV2"s, "ENV1"s, "ENV0"s };

hat::core::InternalLayoutElementRepresentation getSimpleEnvSelectorButton(size_t ENV_Index, bool referencesENV = true)
{
	auto result = hat::core::InternalLayoutElementRepresentation{ ENVs[ENV_Index] };
	if (referencesENV) {
		result.setSwitchToAnotherEnv(ENV_Index);
	}
	result.setButtonFlag(true); //in any case (either it is enabled or not), it should look like a button
	return result;
}
hat::core::InternalLayoutPageRepresentation getEnvSelectionPage(bool is_env_selected, size_t selectedEnv)
{
	auto getObject = [is_env_selected, selectedEnv](size_t envIndex) -> auto {
		bool isThisEnvSelected = ((is_env_selected) && (selectedEnv == envIndex));
		return getSimpleEnvSelectorButton(envIndex, !isThisEnvSelected);
	};
	auto result = hat::core::InternalLayoutPageRepresentation{ "Environment selection" };
	result.pushRow({ getObject(ENV3), getObject(ENV2) });
	result.pushRow({ getObject(ENV1), getObject(ENV0) });
	return result;
}
hat::core::InternalLayoutPageRepresentation oneRowPage(std::string const & caption, std::vector<hat::core::InternalLayoutElementRepresentation> const & rowData)
{
	auto result = hat::core::InternalLayoutPageRepresentation{ caption };
	result.pushRow(rowData);
	return result;
}
hat::core::InternalLayoutElementRepresentation inactiveButton(std::string const & caption) {
	return hat::core::InternalLayoutElementRepresentation{ caption }.setButtonFlag(true);
}
hat::core::InternalLayoutElementRepresentation activeButton(std::string const & caption, hat::core::CommandID const & referencedCommand) {
	return hat::core::InternalLayoutElementRepresentation{ caption }.referencedCommand(referencedCommand);
}
}

namespace Catch {

//TODO: remove these methods after the '<<' operators are defined. (also should make sure, that they picked up correctly in the unit tests)
template<> std::string toString(hat::core::InternalLayoutElementRepresentation const & toDump) {
	std::stringstream result;
	result << toDump;
	return result.str();
}
template<> std::string toString(hat::core::InternalLayoutPageRepresentation const & toDump) {
	std::stringstream result;
	result << "preprocessed layout page -- caption:" << toDump.getNote() << "\n";
	for (auto const & layoutElementsRow : toDump.getLayout()) {
		for (auto const & element : layoutElementsRow) {
			result << toString(element) << "\t|";
		}
		result << "\n";
	}
	return result.str();
}
}

TEST_CASE("test appropriate command buttons disabled", "[configs_abstraction]")
{
	using hat::core::InternalLayoutElementRepresentation;
	auto const PAGE_NAME = std::string{ "This test page" };
	auto const COMMAND_ENABLED_ONLY_IN_ENV0_ID = "hk1"s;
	auto const COMMAND_ENABLED_ONLY_IN_ENV0_NOTE = "hk1_note"s;
	auto const COMMAND_ENABLED_ONLY_IN_ENV1_ID = "hk2"s;
	auto const COMMAND_ENABLED_ONLY_IN_ENV1_NOTE = "hk2_note"s;
	auto const COMMAND_ENABLED_ONLY_IN_ENV2_ID = "hk4"s;
	auto const COMMAND_ENABLED_ONLY_IN_ENV2_NOTE = "hk4_note"s;

	// Build the layout info
	hat::test::LayoutConfigParsingVerificator verificator;
	verificator.startNewPage(PAGE_NAME);

	// TODO: add the ENV3 here
	verificator.addRow({ COMMAND_ENABLED_ONLY_IN_ENV0_ID, COMMAND_ENABLED_ONLY_IN_ENV1_ID, COMMAND_ENABLED_ONLY_IN_ENV2_ID });
	verificator.verifyConfigIsOK();

	//verify the generated preprocessed layouts for situations when no ENV is selected, or ENV0, ENV1 or ENV2 is selected:
	auto layoutsGenerator = hat::core::ConfigsAbstractionLayer{ verificator.getAccumulatedConfig(), COMMANDS_INFO_CONTAINER };
	auto layoutWhenENV0isSelected = layoutsGenerator.generateLayoutPresentation(ENV0, true);
	auto layoutWhenENV1isSelected = layoutsGenerator.generateLayoutPresentation(ENV1, true);
	auto layoutWhenENV2isSelected = layoutsGenerator.generateLayoutPresentation(ENV2, true);

	auto layoutForUnselectedEnv = layoutsGenerator.generateLayoutPresentation(ENV0, false);
	//make sure, that when the environment is not selected, the ENV parameter value does not matter:
	REQUIRE(layoutForUnselectedEnv == layoutsGenerator.generateLayoutPresentation(ENV1, false));
	REQUIRE(layoutForUnselectedEnv == layoutsGenerator.generateLayoutPresentation(ENV2, false));


	//generation of the reference layouts (TODO: maybe will move the duplicated code into lambda or external function):
	auto reference_layoutWhenENV0isSelected = hat::core::InternalLayoutRepresentation({
		getEnvSelectionPage(true, ENV0),
		oneRowPage(PAGE_NAME, {
			activeButton(COMMAND_ENABLED_ONLY_IN_ENV0_NOTE, hat::core::CommandID{COMMAND_ENABLED_ONLY_IN_ENV0_ID}),
			inactiveButton(COMMAND_ENABLED_ONLY_IN_ENV1_NOTE),
			inactiveButton(COMMAND_ENABLED_ONLY_IN_ENV2_NOTE) })
	});

	auto reference_layoutWhenENV1isSelected = hat::core::InternalLayoutRepresentation({
		getEnvSelectionPage(true, ENV1),
		oneRowPage(PAGE_NAME, {
			inactiveButton(COMMAND_ENABLED_ONLY_IN_ENV0_NOTE),
			activeButton(COMMAND_ENABLED_ONLY_IN_ENV1_NOTE, hat::core::CommandID{COMMAND_ENABLED_ONLY_IN_ENV1_ID}),
			inactiveButton(COMMAND_ENABLED_ONLY_IN_ENV2_NOTE) })
	});

	auto reference_layoutWhenENV2isSelected = hat::core::InternalLayoutRepresentation({
		getEnvSelectionPage(true, ENV2),
		oneRowPage(PAGE_NAME, {
			inactiveButton(COMMAND_ENABLED_ONLY_IN_ENV0_NOTE),
			inactiveButton(COMMAND_ENABLED_ONLY_IN_ENV1_NOTE),
			activeButton(COMMAND_ENABLED_ONLY_IN_ENV2_NOTE, hat::core::CommandID{COMMAND_ENABLED_ONLY_IN_ENV2_ID}) })
	});
	auto reference_layoutForUnselectedEnv = hat::core::InternalLayoutRepresentation({
		getEnvSelectionPage(false, ENV2)
	});

	REQUIRE(layoutWhenENV0isSelected == reference_layoutWhenENV0isSelected);
	REQUIRE(layoutWhenENV1isSelected == reference_layoutWhenENV1isSelected);
	REQUIRE(layoutWhenENV2isSelected == reference_layoutWhenENV2isSelected);
	REQUIRE(layoutForUnselectedEnv == reference_layoutForUnselectedEnv);
}

TEST_CASE("test appropriate selectors disabled (if no enabled buttons inside)", "[configs_abstraction]")
{
	auto const MAIN_PAGE_CAPTION = "test page caption"s;
	auto const SELECTOR_PAGE_CAPTION = "selector page caption"s;
	auto const SUBSELECTOR_PAGE_CAPTION = "subselector page caption"s;
	auto const SELECTOR_ID = "selector"s;
	auto const SUBSELECTOR_ID = "subselector"s;

	auto const COMMAND_BUTTON_ON_MAIN_PAGE_ID = "hkF"s;        // hkF ensures that there is at least one element on the main page, which is active (otherwise, this page will be thrown out of the set).
	auto const COMMAND_BUTTON_ON_MAIN_PAGE_NOTE = "hkF_note"s;
	auto const COMMAND_BUTTON_ON_SELECTOR_PAGE_ID = "hk3"s;    // the hk3 is enabled for ENV0 and ENV1, but disabled for ENV2 and ENV3
	auto const COMMAND_BUTTON_ON_SELECTOR_PAGE_NOTE = "hk3_note"s;
	auto const COMMAND_BUTTON_ON_SUBSELECTOR_PAGE_ID = "hk5"s; // the hk5 is enabled for ENV0 and ENV2, but disabled for ENV1 and ENV3
	auto const COMMAND_BUTTON_ON_SUBSELECTOR_PAGE_NOTE = "hk5_note"s;

	hat::test::LayoutConfigParsingVerificator verificator;
	verificator.startNewPage(MAIN_PAGE_CAPTION);
	verificator.addRow({ COMMAND_BUTTON_ON_MAIN_PAGE_ID, SELECTOR_ID });
	verificator.startNewOptionsSelectorPage(SELECTOR_PAGE_CAPTION, SELECTOR_ID);
	verificator.addRow({ SUBSELECTOR_ID, COMMAND_BUTTON_ON_SELECTOR_PAGE_ID });
	verificator.startNewOptionsSelectorPage(SUBSELECTOR_PAGE_CAPTION, SUBSELECTOR_ID);
	verificator.addRow({ COMMAND_BUTTON_ON_SUBSELECTOR_PAGE_ID });
	verificator.verifyConfigIsOK();

	auto layoutsGenerator = hat::core::ConfigsAbstractionLayer{ verificator.getAccumulatedConfig(), COMMANDS_INFO_CONTAINER };
	auto layoutWhenENV0isSelected = layoutsGenerator.generateLayoutPresentation(ENV0, true);
	auto layoutWhenENV1isSelected = layoutsGenerator.generateLayoutPresentation(ENV1, true);
	auto layoutWhenENV2isSelected = layoutsGenerator.generateLayoutPresentation(ENV2, true);
	auto layoutWhenENV3isSelected = layoutsGenerator.generateLayoutPresentation(ENV3, true);

	auto createMainLayoutPage = [&](bool selectorOptionEnabled, bool subselectorOptionEnabled) -> hat::core::InternalLayoutPageRepresentation {

		auto mainPage = oneRowPage(MAIN_PAGE_CAPTION, { activeButton(COMMAND_BUTTON_ON_MAIN_PAGE_NOTE, hat::core::CommandID{COMMAND_BUTTON_ON_MAIN_PAGE_ID}), hat::core::InternalLayoutElementRepresentation{ SELECTOR_PAGE_CAPTION }.setButtonFlag(true) });
		if (selectorOptionEnabled || subselectorOptionEnabled) {
			auto & buttonForSwitchingToSelector = mainPage.getCurrentlyLastElement();
			auto optionsSelectorPage = std::make_shared<hat::core::InternalLayoutPageRepresentation>(SELECTOR_PAGE_CAPTION);
			auto selectorPageElements = std::vector<hat::core::InternalLayoutElementRepresentation>{};
			if (subselectorOptionEnabled) {
				selectorPageElements.push_back(hat::core::InternalLayoutElementRepresentation{ SUBSELECTOR_PAGE_CAPTION }.setButtonFlag(true));
				selectorPageElements.back().resetOptionsPage(
					std::make_shared<hat::core::InternalLayoutPageRepresentation>(
						oneRowPage(SUBSELECTOR_PAGE_CAPTION, { activeButton(COMMAND_BUTTON_ON_SUBSELECTOR_PAGE_NOTE, hat::core::CommandID{COMMAND_BUTTON_ON_SUBSELECTOR_PAGE_ID}) })
						));
			} else {
				selectorPageElements.push_back(inactiveButton(SUBSELECTOR_PAGE_CAPTION));
			}
			if (selectorOptionEnabled) {
				selectorPageElements.push_back(activeButton(COMMAND_BUTTON_ON_SELECTOR_PAGE_NOTE, hat::core::CommandID{ COMMAND_BUTTON_ON_SELECTOR_PAGE_ID }));
			} else {
				selectorPageElements.push_back(inactiveButton(COMMAND_BUTTON_ON_SELECTOR_PAGE_NOTE));
			}
			optionsSelectorPage->pushRow(selectorPageElements);
			buttonForSwitchingToSelector.resetOptionsPage(optionsSelectorPage);
		}
		return mainPage;
	};
	auto const reference_layoutWhenENV0isSelected = hat::core::InternalLayoutRepresentation({ getEnvSelectionPage(true, ENV0), createMainLayoutPage(true, true) });
	auto const reference_layoutWhenENV1isSelected = hat::core::InternalLayoutRepresentation({ getEnvSelectionPage(true, ENV1), createMainLayoutPage(true, false) });
	auto const reference_layoutWhenENV2isSelected = hat::core::InternalLayoutRepresentation({ getEnvSelectionPage(true, ENV2), createMainLayoutPage(false, true) });
	auto const reference_layoutWhenENV3isSelected = hat::core::InternalLayoutRepresentation({ getEnvSelectionPage(true, ENV3), createMainLayoutPage(false, false) });

	REQUIRE(layoutWhenENV0isSelected == reference_layoutWhenENV0isSelected);
	REQUIRE(layoutWhenENV1isSelected == reference_layoutWhenENV1isSelected);
	REQUIRE(layoutWhenENV2isSelected == reference_layoutWhenENV2isSelected);
	REQUIRE(layoutWhenENV3isSelected == reference_layoutWhenENV3isSelected);
}

TEST_CASE("test correct options picked(buttons and selectors)", "[configs_abstraction]")
{
	auto const MAIN_PAGE_CAPTION = "main page"s;
	auto const OPTION_0_COMMAND_ID = "hk1"s;  // enabled for ENV0 only
	auto const OPTION_0_COMMAND_NOTE = "hk1_note"s;
	auto const OPTION_1_SELECTOR_ID = "selector"s;
	auto const OPTION_1_SELECTOR_CAPTION = "selector page"s;
	auto const OPTION_1_COMMAND_ID = "hk3"s; // Button inside the selector, enabled for ENV0, ENV1
	auto const OPTION_1_COMMAND_NOTE = "hk3_note"s;
	auto const OPTION_2_COMMAND_ID = "hk7"s; // enabled for ENV0, ENV1, ENV2
	auto const OPTION_2_COMMAND_NOTE = "hk7_note"s;
	//none of above options are enabled for ENV3, it should display an inactive button with note for the first option

	auto const OPTION_ENABLED_FOR_ALL_ENVS_ID = "hkF"s; // enabled for ENV0, ENV1, ENV2
	auto const OPTION_ENABLED_FOR_ALL_ENVS_NOTE = "hkF_note"s;

	auto const COMPLEX_COMBINATION_OF_IDs = OPTION_0_COMMAND_ID + "," + OPTION_1_SELECTOR_ID + "," + OPTION_2_COMMAND_ID;

	hat::test::LayoutConfigParsingVerificator verificator;
	verificator.startNewPage(MAIN_PAGE_CAPTION);
	verificator.addRow({ COMPLEX_COMBINATION_OF_IDs, OPTION_ENABLED_FOR_ALL_ENVS_ID });
	verificator.startNewOptionsSelectorPage(OPTION_1_SELECTOR_CAPTION, OPTION_1_SELECTOR_ID);
	verificator.addRow({ OPTION_1_COMMAND_ID });
	verificator.verifyConfigIsOK();

	//std::cout << verificator.getAccumulatedConfigText(); //DEBUG
	auto layoutsGenerator = hat::core::ConfigsAbstractionLayer{ verificator.getAccumulatedConfig(), COMMANDS_INFO_CONTAINER };
	auto layoutWhenENV0isSelected = layoutsGenerator.generateLayoutPresentation(ENV0, true);
	auto layoutWhenENV1isSelected = layoutsGenerator.generateLayoutPresentation(ENV1, true);
	auto layoutWhenENV2isSelected = layoutsGenerator.generateLayoutPresentation(ENV2, true);
	auto layoutWhenENV3isSelected = layoutsGenerator.generateLayoutPresentation(ENV3, true);

	auto reference_layoutWhenENV0isSelected = hat::core::InternalLayoutRepresentation({
		getEnvSelectionPage(true, ENV0), oneRowPage(MAIN_PAGE_CAPTION, {
			activeButton(OPTION_0_COMMAND_NOTE, hat::core::CommandID{OPTION_0_COMMAND_ID}), activeButton(OPTION_ENABLED_FOR_ALL_ENVS_NOTE, hat::core::CommandID{OPTION_ENABLED_FOR_ALL_ENVS_ID})})
	});

	auto reference_layoutWhenENV1isSelected = hat::core::InternalLayoutRepresentation({
		getEnvSelectionPage(true, ENV1), oneRowPage(MAIN_PAGE_CAPTION,{
			hat::core::InternalLayoutElementRepresentation(OPTION_1_SELECTOR_CAPTION).setButtonFlag(true).resetOptionsPage(
				std::make_shared<hat::core::InternalLayoutPageRepresentation>(//manually set the selector page reference inside the first element of the second page in the layout:
					oneRowPage(OPTION_1_SELECTOR_CAPTION,{ activeButton(OPTION_1_COMMAND_NOTE, hat::core::CommandID{ OPTION_1_COMMAND_ID }) }))),
			activeButton(OPTION_ENABLED_FOR_ALL_ENVS_NOTE, hat::core::CommandID{OPTION_ENABLED_FOR_ALL_ENVS_ID}) })
	});

	auto reference_layoutWhenENV2isSelected = hat::core::InternalLayoutRepresentation({
		getEnvSelectionPage(true, ENV2), oneRowPage(MAIN_PAGE_CAPTION,{
			activeButton(OPTION_2_COMMAND_NOTE, hat::core::CommandID{OPTION_2_COMMAND_ID}), activeButton(OPTION_ENABLED_FOR_ALL_ENVS_NOTE, hat::core::CommandID{OPTION_ENABLED_FOR_ALL_ENVS_ID}) })
	});

	auto reference_layoutWhenENV3isSelected = hat::core::InternalLayoutRepresentation({
		getEnvSelectionPage(true, ENV3), oneRowPage(MAIN_PAGE_CAPTION,{ inactiveButton(OPTION_0_COMMAND_NOTE), activeButton(OPTION_ENABLED_FOR_ALL_ENVS_NOTE, hat::core::CommandID{OPTION_ENABLED_FOR_ALL_ENVS_ID}) })
	});
	REQUIRE(layoutWhenENV0isSelected == reference_layoutWhenENV0isSelected);
	REQUIRE(layoutWhenENV1isSelected == reference_layoutWhenENV1isSelected);
	REQUIRE(layoutWhenENV2isSelected == reference_layoutWhenENV2isSelected);
	REQUIRE(layoutWhenENV3isSelected == reference_layoutWhenENV3isSelected);
}

TEST_CASE("test that pages with no active buttons are not shown", "[configs_abstraction]")
{
	auto const COMMAND_ENABLED_FOR_ENV0_1_ID = "hk3"s;
	auto const COMMAND_ENABLED_FOR_ENV0_1_NOTE = "hk3_note"s;
	auto const COMMAND_ENABLED_FOR_ENV0_2_ID = "hk5"s;
	auto const COMMAND_ENABLED_FOR_ENV0_2_NOTE = "hk5_note"s;
	auto const ENV0_1_PAGE_CAPTION = "page for ENV0, ENV1"s;
	auto const ENV0_2_PAGE_CAPTION = "page for ENV0, ENV2"s;

	hat::test::LayoutConfigParsingVerificator verificator;
	verificator.startNewPage(ENV0_1_PAGE_CAPTION);
	verificator.addRow({ COMMAND_ENABLED_FOR_ENV0_1_ID });
	verificator.startNewPage(ENV0_2_PAGE_CAPTION);
	verificator.addRow({ COMMAND_ENABLED_FOR_ENV0_2_ID });
	verificator.verifyConfigIsOK();
	auto layoutsGenerator = hat::core::ConfigsAbstractionLayer{ verificator.getAccumulatedConfig(), COMMANDS_INFO_CONTAINER };
	auto layoutWhenENV0isSelected = layoutsGenerator.generateLayoutPresentation(ENV0, true);
	auto layoutWhenENV1isSelected = layoutsGenerator.generateLayoutPresentation(ENV1, true);
	auto layoutWhenENV2isSelected = layoutsGenerator.generateLayoutPresentation(ENV2, true);

	auto reference_layoutWhenENV0isSelected = hat::core::InternalLayoutRepresentation({
		getEnvSelectionPage(true, ENV0),
		oneRowPage(ENV0_1_PAGE_CAPTION,{ activeButton(COMMAND_ENABLED_FOR_ENV0_1_NOTE, hat::core::CommandID { COMMAND_ENABLED_FOR_ENV0_1_ID }) }),
		oneRowPage(ENV0_2_PAGE_CAPTION,{ activeButton(COMMAND_ENABLED_FOR_ENV0_2_NOTE, hat::core::CommandID { COMMAND_ENABLED_FOR_ENV0_2_ID }) })
	});

	auto reference_layoutWhenENV1isSelected = hat::core::InternalLayoutRepresentation({
		getEnvSelectionPage(true, ENV1),
		oneRowPage(ENV0_1_PAGE_CAPTION,{ activeButton(COMMAND_ENABLED_FOR_ENV0_1_NOTE, hat::core::CommandID{ COMMAND_ENABLED_FOR_ENV0_1_ID }) })
	});

	auto reference_layoutWhenENV2isSelected = hat::core::InternalLayoutRepresentation({
		getEnvSelectionPage(true, ENV2),
		oneRowPage(ENV0_2_PAGE_CAPTION,{ activeButton(COMMAND_ENABLED_FOR_ENV0_2_NOTE, hat::core::CommandID{ COMMAND_ENABLED_FOR_ENV0_2_ID }) })
	});

	REQUIRE(layoutWhenENV0isSelected == reference_layoutWhenENV0isSelected);
	REQUIRE(layoutWhenENV1isSelected == reference_layoutWhenENV1isSelected);
	REQUIRE(layoutWhenENV2isSelected == reference_layoutWhenENV2isSelected);
}
TEST_CASE("test that when there is one ENV, we don't show ENV selection page", "[configs_abstraction]")
{
	auto const singleENVCommandsConfiguration = std::string{
		"command_id\tcommand_category\tcommand_note\tcommand_description\tSINGLE_ENV\n"
		"hk0\tcategory\thk0_note\thk0_desc\thk0_keys\n" };

	auto const PAGE_CAPTION = "test page"s;
	auto const COMMAND_ID = "hk0"s;
	auto const COMMAND_NOTE = "hk0_note"s;

	hat::test::LayoutConfigParsingVerificator verificator;
	verificator.startNewPage(PAGE_CAPTION);
	verificator.addRow({ COMMAND_ID });
	verificator.verifyConfigIsOK();

	std::stringstream commandsStream{ singleENVCommandsConfiguration };
	const auto COMMANDS_INFO_CONTAINER = hat::core::CommandsInfoContainer::parseConfigFile(commandsStream);
	auto layoutsGenerator = hat::core::ConfigsAbstractionLayer{ verificator.getAccumulatedConfig(), COMMANDS_INFO_CONTAINER };

	auto tested_layout = layoutsGenerator.generateLayoutPresentation(0, true);

	auto reference_layout = hat::core::InternalLayoutRepresentation({
		//note the absence of the ENV selection page here (in other tests it is created by a 'getEnvSelectionPage' call)
		oneRowPage(PAGE_CAPTION,{ activeButton(COMMAND_NOTE, hat::core::CommandID{ COMMAND_ID }) })
	});
	REQUIRE(tested_layout == reference_layout);
}

TEST_CASE("test that for emply layout information we show a placeholder page with the information", "[configs_abstraction]")
{
	//TODO: add the test case, when there is only one ENV, for which there is nothing to display (no active buttons on the page)
}

