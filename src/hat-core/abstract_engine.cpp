// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef HAT_CORE_HEADERONLY_MODE
#include "abstract_engine.hpp"
#endif
#include <sstream>

#ifndef HAT_CORE_HEADERONLY_MODE
#define LINKAGE_RESTRICTION 
#else
#define LINKAGE_RESTRICTION inline
#endif

namespace hat {
namespace core {
LINKAGE_RESTRICTION std::string encodeNumberInTauIdentifier(char prefix, size_t numberToEncode)
{
	static size_t counter = 0;
	std::stringstream result;
	result << prefix << numberToEncode << '_' << counter;
	++counter;
	return result.str();
}

LINKAGE_RESTRICTION size_t getEncodedNumberFromTauIdentifier(std::string const & id)
{
	size_t encodedNumberEnd = id.find("_");
	auto encodedPart = id.substr(1, encodedNumberEnd - 1);
	std::stringstream str(encodedPart);
	size_t result = 0;
	str >> result;
	return result;
}

//Returns an element ID, which is unique for the layout, but also is easily convertible to the command ID
LINKAGE_RESTRICTION std::string AbstractEngine::generateTauIdentifierForCommand(size_t commandIndex)
{
	return encodeNumberInTauIdentifier(TAU_PREFIX_COMMAND, commandIndex);
}

//The values generated from this call are for the elements, which are not interesting for the user (navigation buttons, labels, etc). The only requirement for them is not to conflict with other IDs
LINKAGE_RESTRICTION std::string AbstractEngine::generateTrowawayTauIdentifier()
{
	return encodeNumberInTauIdentifier(TAU_PREFIX_THROWAWAY, 0);
}

LINKAGE_RESTRICTION std::string AbstractEngine::generateTauIdentifierForEnvSwitching(size_t indexForEnvironment)
{
	return encodeNumberInTauIdentifier(TAU_PREFIX_ENV_SWITCH, indexForEnvironment);
}

enum class SPECIAL_SERVER_COMMANDS
{
	RELOAD_CONFIGS_BUTTON,
	RETRY_CURRENT_PENDING_COMMAND,
	CANCEL_PENDING_COMMAND,
	STICK_TOPMOST_WINDOW_TO_SELECTED_ENVIRONMENT
};

LINKAGE_RESTRICTION std::string AbstractEngine::generateReloadButtonID()
{

	return encodeNumberInTauIdentifier(TAU_PREFIX_UTIL, static_cast<size_t>(SPECIAL_SERVER_COMMANDS::RELOAD_CONFIGS_BUTTON));
}

LINKAGE_RESTRICTION std::string AbstractEngine::generateResendPendingCommandButtonID()
{
	return encodeNumberInTauIdentifier(TAU_PREFIX_UTIL, static_cast<size_t>(SPECIAL_SERVER_COMMANDS::RETRY_CURRENT_PENDING_COMMAND));
}

LINKAGE_RESTRICTION std::string AbstractEngine::generateClearPendingCommandButtonID()
{
	return encodeNumberInTauIdentifier(TAU_PREFIX_UTIL, static_cast<size_t>(SPECIAL_SERVER_COMMANDS::CANCEL_PENDING_COMMAND));
}

LINKAGE_RESTRICTION std::string AbstractEngine::generateStickEnvironmentToWindowCommand()
{
	return encodeNumberInTauIdentifier(TAU_PREFIX_UTIL, static_cast<size_t>(SPECIAL_SERVER_COMMANDS::STICK_TOPMOST_WINDOW_TO_SELECTED_ENVIRONMENT));
}

//Return value tells the caller, if the layout should be refereshed.
LINKAGE_RESTRICTION FeedbackFromButtonClick AbstractEngine::buttonOnLayoutClicked(std::string const & buttonID)
{
	char indicator = buttonID[0];
	if (indicator == TAU_PREFIX_COMMAND) {
		auto commandToExecute = getEncodedNumberFromTauIdentifier(buttonID);
		if (canSendTheCommmandForEnvironment()) {
			executeCommandForCurrentlySelectedEnvironment(commandToExecute);
		} else {
			m_currentlyPendingCommand = commandToExecute;
			m_hasCurrentlyPendingCommand = true;

			switchLayout_wrongTopmostWindow();
			return FeedbackFromButtonClick::UPDATE_LAYOUT;
		}
	} else if (indicator == TAU_PREFIX_ENV_SWITCH) {
		return setNewEnvironment(getEncodedNumberFromTauIdentifier(buttonID)) ? FeedbackFromButtonClick::UPDATE_LAYOUT : FeedbackFromButtonClick::NONE;
	} else if (indicator == TAU_PREFIX_UTIL) {
		auto encodedActionIndex = static_cast<SPECIAL_SERVER_COMMANDS>(getEncodedNumberFromTauIdentifier(buttonID));
		switch (encodedActionIndex) {
		case SPECIAL_SERVER_COMMANDS::RELOAD_CONFIGS_BUTTON:
			return FeedbackFromButtonClick::RELOAD_CONFIGS;
		case SPECIAL_SERVER_COMMANDS::RETRY_CURRENT_PENDING_COMMAND:
			if (canSendTheCommmandForEnvironment()) {
				//TODO: ensure that the m_hasCurrentlyPendingCommand is true here.
				executeCommandForCurrentlySelectedEnvironment(m_currentlyPendingCommand);
				m_hasCurrentlyPendingCommand = false;
				switchLayout_restoreToNormalLayout();
				return FeedbackFromButtonClick::UPDATE_LAYOUT;
			} else {
				return FeedbackFromButtonClick::NONE; //Still wrong top window
			}
			//case SPECIAL_SERVER_COMMANDS::CANCEL_TOPMOST_WINDOW_SELECTION: //TODO: implement (switches back to unselected environment state)
		case SPECIAL_SERVER_COMMANDS::STICK_TOPMOST_WINDOW_TO_SELECTED_ENVIRONMENT:
			stickCurrentTopWindowToSelectedEnvironment();
			return FeedbackFromButtonClick::UPDATE_LAYOUT;
		case SPECIAL_SERVER_COMMANDS::CANCEL_PENDING_COMMAND:
			m_hasCurrentlyPendingCommand = false;
			switchLayout_restoreToNormalLayout();
			return FeedbackFromButtonClick::UPDATE_LAYOUT;
		default:
			break;
		}

		//TODO: throw an exception here - this should never happen
		return FeedbackFromButtonClick::RELOAD_CONFIGS;
	}
	return FeedbackFromButtonClick::NONE;
}

} //namespace core
} //namespace hat

#undef LINKAGE_RESTRICTION