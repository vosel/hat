// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef HAT_CORE_HEADERONLY_MODE
#include "configs_abstraction_layer.hpp"
#endif

#include <set>
#include <sstream>

#ifndef HAT_CORE_HEADERONLY_MODE
#define LINKAGE_RESTRICTION 
#else
#define LINKAGE_RESTRICTION inline
#endif

namespace hat {
namespace core {

LINKAGE_RESTRICTION ConfigsAbstractionLayer::ConfigsAbstractionLayer(LayoutUserInformation const & layoutInfo, CommandsInfoContainer const & commandsConfig)
	: m_layoutInfo(layoutInfo), m_commandsConfig(commandsConfig)
{
	for (auto const & command : m_commandsConfig.m_commandsList) {
		if (m_layoutInfo.contains_selector(command.commandID)) {
			std::stringstream error;
			error << "A conflict of IDs: the same string is used as an ID for a command and a options picker page. This is not allowed. The problem ID='" << command.commandID.getValue() << '\'';
			throw std::runtime_error(error.str()); //TODO: test this
		}
	}
	//TODO: add the verification code here (verify that there are no duplicates in IDs)
}

LINKAGE_RESTRICTION InternalLayoutRepresentation ConfigsAbstractionLayer::generateLayoutPresentation(size_t selectedEnv, bool isEnv_selected)
{
	std::set<CommandID> activeIDs;
	for (auto const & command : m_commandsConfig.m_commandsList) {
		if (command.hotkeysForEnvironments[selectedEnv]->enabled) {
			activeIDs.insert(command.commandID);
		}
	}

	//cannot leave the type of this lambda as 'auto' because of it's recursive nature. TODO: maybe will extract this code outside, so it is easier to maintain and reason about it
	std::function<std::shared_ptr<InternalLayoutPageRepresentation>(LayoutPageTemplate const &)> createLayoutPage
		= [&](LayoutPageTemplate const & templateToUse) -> std::shared_ptr<InternalLayoutPageRepresentation>
	{
		//TODO!!! add protection from the cyclical dependencies for the options pages selectors!!!!
		std::shared_ptr<InternalLayoutPageRepresentation> result = std::make_shared<InternalLayoutPageRepresentation>(templateToUse.get_note());
		for (auto const & row : templateToUse.get_rows()) {
			std::vector<InternalLayoutElementRepresentation> currentRow;
			for (auto const & elem : row) {
				bool foundActiveElementForThisPosition = false;
				std::string firstNonEmptyNote("");
				for (auto const & currentOption : elem.getOptions()) {

					//TODO: refactor and clean up the logic here (after adding the unit tests for it).
					InternalLayoutElementRepresentation testElement;

					if (firstNonEmptyNote.size() == 0) {
						if (!currentOption.isVariableLabel()) { //NOTE: here we don't need to check variables (because they are always active), so, if we find a variable, we will not need the fallback string for inactive button.
							auto const & commandID = currentOption.getComandID();
							if (m_commandsConfig.hasCommandID(commandID)) {
								firstNonEmptyNote = m_commandsConfig.getCommandPrefs(commandID).commandNote;
							} else {
								auto foundIter = m_layoutInfo.find_selector(commandID);
								if (foundIter != m_layoutInfo.non_existent_selector()) {
									//TODO: get rid of this copy-paste (see code below)
									firstNonEmptyNote = foundIter->second.get_note();
								}
							}
						}
					}
					if (currentOption.isVariableLabel()) {
						auto & variablesManager = m_commandsConfig.getVariablesManagers_c().getManagerForEnv_c(selectedEnv);
						auto variableID = currentOption.getVariableID();
						testElement.setButtonFlag(false);
						if (variablesManager.variableExists(variableID)) {
							testElement.setNote(variablesManager.getValue(variableID));
							testElement.setReferencingVariableID(variableID);
						} else {
							testElement.setNote("<UNKNOWN_VARIABLE>"); // TODO: add warnings during configs parsing that the variable is undefined
						}
						currentRow.push_back(testElement);
						foundActiveElementForThisPosition = true;
						break;
					} else {
						auto const & commandID = currentOption.getComandID();
						if (activeIDs.find(commandID) != activeIDs.end()) { // found the element, for which the button should be created
							testElement.setCommandButtonAttrs(m_commandsConfig.getCommandPrefs(commandID).commandNote, commandID);
							currentRow.push_back(testElement);
							foundActiveElementForThisPosition = true;
							break;
						} else { //try to find the options selection page:
							auto foundIter = m_layoutInfo.find_selector(commandID);
							if (foundIter != m_layoutInfo.non_existent_selector()) {
								testElement.setSelectorButtonAttrs(foundIter->second.get_note(), createLayoutPage(foundIter->second));
								if (testElement.isActive()) {
									currentRow.push_back(testElement);
									foundActiveElementForThisPosition = true;
									break;
								}
							}
						}
					}
				}
				if (!foundActiveElementForThisPosition) {
					InternalLayoutElementRepresentation testElement;
					if (elem.getOptions().size() > 0) {
						if (firstNonEmptyNote.size() == 0) {
							firstNonEmptyNote = "UNKNOWN ID"; // we should not get here (an exception should be thrown before that - during the configuration files processing)
						}
						testElement.setButtonFlag(true).setNote(firstNonEmptyNote);
					} else {
						testElement.setButtonFlag(false);
					}
					currentRow.push_back(testElement);
				}
			}
			if (currentRow.size() > 0) {
				result->pushRow(currentRow);
			}
		}
		return result;
	};

	size_t env_count = m_commandsConfig.getEnvironments().size();
	size_t ITEMS_PER_ROW = 2;
	InternalLayoutRepresentation result;

	if ((env_count > 1) || (!isEnv_selected)) { // show environment selection page only if there are serveral environments to select from
		auto  page = InternalLayoutPageRepresentation{ "Environment selection" };

		size_t i = 0;
		while (i < env_count) {
			page.pushRow(std::vector<InternalLayoutElementRepresentation>());
			for (size_t j = 0; j < ITEMS_PER_ROW; ++j) {
				size_t currentEnvIndex = i + j;
				InternalLayoutElementRepresentation toPush;

				if (currentEnvIndex < env_count) {
					toPush.setButtonFlag(true).setNote(m_commandsConfig.getEnvironments()[currentEnvIndex]);
					if ((currentEnvIndex != selectedEnv) || (!isEnv_selected)) {
						toPush.setSwitchToAnotherEnv(currentEnvIndex);
					}
				} else {
					toPush.setButtonFlag(false);
				}
				page.pushElement(toPush);
			}
			i += ITEMS_PER_ROW;
		}
		result.push_page(page);
	}

	if (isEnv_selected) {
		for (auto const & layoutPage : m_layoutInfo.getLayoutPages()) {
			std::shared_ptr<InternalLayoutPageRepresentation> page = createLayoutPage(layoutPage);
			if (page->hasActiveUserDefinedButtons()) {
				result.push_page(*page);
			}
		}
	}
	//Create an environment selection page (if there are more then 1 environment). If there is a selected environment, then the button for it should be inactive.

	return result;
}
} //namespace core
} //namespace hat

#undef LINKAGE_RESTRICTION