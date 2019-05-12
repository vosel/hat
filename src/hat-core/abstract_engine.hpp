// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef ABSTRACT_ENGINE_HPP
#define ABSTRACT_ENGINE_HPP

#include <string>

//The code here is responsible for extracting the information from the commands configuration file.

namespace hat {
namespace core {
enum class FeedbackFromButtonClick
{
	NONE,
	UPDATE_LAYOUT,
	RESTORE_NORMAL_LAYOUT, //requests to restore the state of original layout
	RELOAD_CONFIGS,
	SHOW_STICK_ENV_TO_WIN_PAGE,
	SHOW_TARGET_WINDOW_NOT_ACTIVE_PAGE,	 // this is returned when the environment is stuck to the given window, and it is not in focus, so the command can't be executed
};
//These are helper methods. Ideally, they should not be exposed to outside code, but this way it is easier to test them;
static std::string encodeNumberInTauIdentifier(char prefix, size_t numberToEncode);
static size_t getEncodedNumberFromTauIdentifier(std::string const & id);

class AbstractEngine
{
	static char const TAU_PREFIX_COMMAND{ 'c' };
	static char const TAU_PREFIX_THROWAWAY{ 't' };
	static char const TAU_PREFIX_ENV_SWITCH{ 'i' };
	static char const TAU_PREFIX_UTIL{ 'u' };

	size_t m_currentlyPendingCommand{ 0 };
	bool m_hasCurrentlyPendingCommand{ false };
protected:
	virtual bool setNewEnvironment(size_t envIndex) = 0; //this usually triggers the layout refresh
	virtual void stickCurrentTopWindowToSelectedEnvironment() = 0;
	// If this method returns false, the pending command index is stored in this object.
	// After that, the user's code should either retry executing the command later, or cancel the pending command.
	virtual bool canSendTheCommmandForEnvironment() const = 0;
	virtual void executeCommandForCurrentlySelectedEnvironment(size_t commandIndex) = 0;

	virtual void switchLayout_wrongTopmostWindow() = 0;
	virtual void switchLayout_restoreToNormalLayout() = 0;
public:
	bool hasPendingCommand() const { return m_hasCurrentlyPendingCommand; };
	// TODO: this method is used only for testing (maybe will rework the testing architecture, so we don't need to dig into the impelemntation details of this class)
	// We could add throwing of exceptions in cases of the pending commands misuse (when a new command is requested, when there still is a pending one, we should throw exception)

//Returns an element ID, which is unique for the layout, but also is easily convertible to the command ID
	static std::string generateTauIdentifierForCommand(size_t commandIndex);
	//The values generated from this call are for the elements, which are not interesting for the user (navigation buttons, labels, etc). The only requirement for them is not to conflict with other IDs
	static std::string generateTrowawayTauIdentifier();
	static std::string generateTauIdentifierForEnvSwitching(size_t indexForEnv);
	static std::string generateReloadButtonID(); // generates a button ID, which should trigger re-read of the configs and reload of the UI on client.
	static std::string generateResendPendingCommandButtonID(); // generates a button ID, which should trigger retry of sending the pending command (this happens when the current topmost window was not equal to the expected)
	static std::string generateClearPendingCommandButtonID(); // generates a button ID, which should trigger clearing of the pending command
	static std::string generateStickEnvironmentToWindowCommand(); // generates a button ID, which should trigger clearing of the pending command
	FeedbackFromButtonClick buttonOnLayoutClicked(std::string const & buttonID);
};
} //namespace core
} //namespace hat

#ifdef HAT_CORE_HEADERONLY_MODE
#include "abstract_engine.cpp"
#endif //HAT_CORE_HEADERONLY_MODE

#endif //ABSTRACT_ENGINE_HPP