// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#include "engine.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include "../external_dependencies/robot/Source/Keyboard.h"
#include "../external_dependencies/robot/Source/Mouse.h"
#include "../external_dependencies/robot/Source/Timer.h"

#ifdef HAT_WINDOWS_SCANCODES_SUPPORT
#include <windows.h>
#endif
namespace hat {
namespace tool {
void Engine::sleep(unsigned int millisec)
{
	ROBOT_NS::Timer::Sleep(millisec);
}
#ifdef HAT_WINDOWS_SCANCODES_SUPPORT
extern bool SHOULD_USE_SCANCODES;
#endif
	Engine::Engine(hat::core::LayoutUserInformation const & layoutInfo,
		hat::core::CommandsInfoContainer const & commandsConfig, hat::core::ImageResourcesInfosContainer const & imagesConfig, bool stickEnvToWindow, unsigned int keystrokes_delay) :
		m_selectedEnvironment(0), isEnv_selected(false),
		m_stickEnvToWindow(stickEnvToWindow),
		m_keystrokes_delay(keystrokes_delay),
		m_layoutInfo(layoutInfo),
		m_commandsConfig(commandsConfig),
		m_imagesConfig(imagesConfig)
	{
		if (!shouldShowEnvironmentSelectionPage()) { // if there is only one environment, we don't need to select anything
			setNewEnvironment(0);
		}
		auto environmentsCount = m_commandsConfig.getEnvironments().size();
		m_stickInfo.reserve(environmentsCount);
		for (size_t i = 0; i < environmentsCount; ++i) {
			m_stickInfo.push_back(0);
		}
	}

	bool Engine::shouldShowEnvironmentSelectionPage() const
	{
		return m_commandsConfig.getEnvironments().size() > 1;
	}

	bool Engine::setNewEnvironment(size_t environmentIndex)
	{
		if ((environmentIndex != m_selectedEnvironment) || (!isEnv_selected)) {
			m_selectedEnvironment = environmentIndex;
			isEnv_selected = true;
			m_shouldRebuildNormalLayout = true;
			if (m_stickEnvToWindow) {
				m_currentState = LayoutState::STICK_ENVIRONMENT_TO_WND;
			} else {
				m_currentState = LayoutState::NORMAL;
			}
			return true;
		}
		return false;
	}

	bool Engine::canSendTheCommmandForEnvironment() const
	{
		if (m_stickEnvToWindow) {
			return ROBOT_NS::Window::GetActive().GetHandle() == m_stickInfo[m_selectedEnvironment];
		}
		return true;
	}

	void Engine::switchLayout_wrongTopmostWindow()
	{
		m_shouldRebuildNormalLayout = false;
		m_currentState = LayoutState::WAIT_FOR_EXPECTED_WND_AT_FRONT;
	}

	void Engine::switchLayout_restoreToNormalLayout()
	{
		m_currentState = LayoutState::NORMAL;
		m_currentNormalLayout.setStartLayoutPage(*m_lastTopPageSelected);
	}

	void Engine::stickCurrentTopWindowToSelectedEnvironment()
	{
		m_stickInfo[m_selectedEnvironment] = ROBOT_NS::Window::GetActive().GetHandle();
		m_currentState = LayoutState::NORMAL;
	}

	void Engine::executeCommandForCurrentlySelectedEnvironment(size_t commandIndex)
	{
		hat::core::Command commandToExecute = m_commandsConfig.m_commandsList[commandIndex];

		auto & hotkeyToExecute = commandToExecute.hotkeysForEnvironments[m_selectedEnvironment];
		
		if (hotkeyToExecute->enabled) {
			std::cout << "The command (id='" << commandToExecute.commandID.getValue()
				<< "') is ready for execution. String representation of command to execute:\n\t" << hotkeyToExecute->m_value << "\n";
			hotkeyToExecute->execute();


			// Do the variable operations and updating their values in UI.
			auto & variablesManager = m_commandsConfig.getVariablesManagers().getManagerForEnv(m_selectedEnvironment);
			auto changedVariables = variablesManager.executeCommandAndGetChangedVariablesList(commandIndex);
			for (auto & updatedVariableID : changedVariables) {
				auto & listOfElementsToRefresh = m_currentlyDisplayedVariables[updatedVariableID];
				auto newValue = variablesManager.getValue(updatedVariableID);
				for (auto & elementIDToRefresh : listOfElementsToRefresh) {
					if (m_uiNotesUpdater) {
						m_uiNotesUpdater(elementIDToRefresh, newValue);
					}
				}
			}
		}
	}

	bool Engine::canStickToWindows()
	{
		return ROBOT_NS::Window::IsAxEnabled();
	}
	namespace {
		// This method parses the string representing a mouse input.
		// The format of the string should look like this: "L@434,234" for the mouse clicks.
		// The substring before '@' represents the type of the button, numbers after it - the coordinates of the click.
		auto parseMouseInputCommandInfoString(std::string const & toProcess)
		{
			std::stringstream myStream(toProcess);
			ROBOT_NS::Button buttonType;
			{
				auto buttonTypeStr = std::string{};
				std::getline(myStream, buttonTypeStr, '@');
				if (buttonTypeStr == core::ConfigFilesKeywords::MouseEventTypes::LeftButton()) {
					buttonType = ROBOT_NS::Button::ButtonLeft;
				} else if (buttonTypeStr == core::ConfigFilesKeywords::MouseEventTypes::RightButton()) {
					buttonType = ROBOT_NS::Button::ButtonRight;
				} else if (buttonTypeStr == core::ConfigFilesKeywords::MouseEventTypes::MiddleButton()) {
					buttonType = ROBOT_NS::Button::ButtonMid;
				} else if (buttonTypeStr == core::ConfigFilesKeywords::MouseEventTypes::X1Button()) {
					buttonType = ROBOT_NS::Button::ButtonX1;
				} else if (buttonTypeStr == core::ConfigFilesKeywords::MouseEventTypes::X2Button()) {
					buttonType = ROBOT_NS::Button::ButtonX2;
				} else {
					std::stringstream error;
					error << "Unknown mouse button type specified: " << buttonTypeStr;
					throw std::runtime_error(error.str());
				}
			}
			
			ROBOT_NS::Point coord;
			{
				auto x_str = std::string{};
				auto y_str = std::string{};
				std::getline(myStream, x_str, ',');
				std::getline(myStream, y_str);
				coord.X = std::stoi(x_str);
				coord.Y = std::stoi(y_str);
			}

			return std::make_pair(buttonType, coord);
		}

		auto getScrollInfo(std::string const & toProcess) {
			std::stringstream myStream(toProcess);
			bool is_verticalScroll{ true };
			bool shouldFlipNumericValue{ false };
			{
				auto scrollTypeStr = std::string{};
				std::getline(myStream, scrollTypeStr, ' ');
				if (scrollTypeStr == core::ConfigFilesKeywords::MouseEventTypes::ScrollV()
					|| scrollTypeStr == core::ConfigFilesKeywords::MouseEventTypes::ScrollV_up())
				{
					is_verticalScroll = true;
					shouldFlipNumericValue = false;
				} else if (scrollTypeStr == core::ConfigFilesKeywords::MouseEventTypes::ScrollV_down()) {
					is_verticalScroll = true;
					shouldFlipNumericValue = true;
				} else if (scrollTypeStr == core::ConfigFilesKeywords::MouseEventTypes::ScrollH()
					|| scrollTypeStr == core::ConfigFilesKeywords::MouseEventTypes::ScrollH_right())
				{
					is_verticalScroll = false;
					shouldFlipNumericValue = false;
				} else if (scrollTypeStr == core::ConfigFilesKeywords::MouseEventTypes::ScrollH_left()) {
					is_verticalScroll = false;
					shouldFlipNumericValue = true;
				} else {
					std::stringstream error;
					error << "Unknown type of scroll specified: " << scrollTypeStr;
					throw std::runtime_error(error.str());
				}
			}

			auto param_str = std::string{};
			std::getline(myStream, param_str);
			auto scrollAmount = std::stoi(param_str); 
			return std::pair<bool, int> {is_verticalScroll, shouldFlipNumericValue ? -scrollAmount : scrollAmount};
		}
	}
	Engine Engine::create(std::string const & commandsCSV, std::vector<std::string> const & inputSequencesConfigs, std::vector<std::string> const & variablesManagersSetupConfigs, std::string const & imageResourcesConfig, std::string const & imageId2CommandIdConfig, std::string const & layoutConfig, bool stickEnvToWindow, unsigned int keyboard_intervals, std::function<void(std::string const &, std::string const &)> loggingCallback)
	{
		std::fstream csvStream(commandsCSV.c_str());
		if (!csvStream.is_open()) {
			throw std::runtime_error("Could not find or open the commands config file: " + commandsCSV);
		}

		loggingCallback("Reading main commands list", commandsCSV);
		class MyHotkeyCombination: public core::SimpleHotkeyCombination
		{
			ROBOT_NS::KeyList m_sequence;
			unsigned int m_keystrokes_delay;
			//little helper function, which abstracts away the keyboard simulation part (which can be platform-dependant)
			void simulateSingleKeyboardInputEvent(ROBOT_NS::Keyboard & keyboard, std::pair<bool, ROBOT_NS::Key> const & keyboardEvent) {
#ifdef HAT_WINDOWS_SCANCODES_SUPPORT
				//optional behaviour:
				if (SHOULD_USE_SCANCODES) {
					bool isExtendedKey =
						(keyboardEvent.second == VK_UP) ||
						(keyboardEvent.second == VK_DOWN) ||
						(keyboardEvent.second == VK_LEFT) ||
						(keyboardEvent.second == VK_RIGHT) ||
						(keyboardEvent.second == VK_HOME) ||
						(keyboardEvent.second == VK_END) ||
						(keyboardEvent.second == VK_PRIOR) ||
						(keyboardEvent.second == VK_NEXT) ||
						(keyboardEvent.second == VK_INSERT) ||
						(keyboardEvent.second == VK_DELETE);

					INPUT input = { 0 };
					input.type = INPUT_KEYBOARD;
					// Calculate scan-code from the Robot's virtual key code:
					input.ki.wScan = MapVirtualKey(keyboardEvent.second, MAPVK_VK_TO_VSC);
					input.ki.dwFlags = (keyboardEvent.first) ? 
						KEYEVENTF_SCANCODE : (KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP);
					if (isExtendedKey) {
						input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
					}
					
					SendInput (1, &input, sizeof (INPUT));
					sleep(m_keystrokes_delay);
					return;
				}
#endif
				//default behaviour:
				(keyboardEvent.first) ? keyboard.Press(keyboardEvent.second) : keyboard.Release(keyboardEvent.second);
			}
		public:
			MyHotkeyCombination(std::string const & param, bool isEnabled, ROBOT_NS::KeyList const & sequence, unsigned int keystrokes_delay): core::SimpleHotkeyCombination(param, isEnabled), m_sequence(sequence), m_keystrokes_delay(keystrokes_delay) {
			}
			void execute() override {
				if (enabled) {
					auto keyboard = ROBOT_NS::Keyboard{};
					keyboard.AutoDelay = m_keystrokes_delay;
					for (auto const & key_event : m_sequence) {
						simulateSingleKeyboardInputEvent(keyboard, key_event);
					}
				}
			}
		};

		class MyMouseInput: public core::SimpleMouseInput
		{
			ROBOT_NS::Button m_buttonToClick;
			ROBOT_NS::Point m_scr_coord;
			unsigned int m_delay;
		public:
			MyMouseInput(std::string const & param, bool isEnabled,
				ROBOT_NS::Button buttonToClick, ROBOT_NS::Point const & pos,
				unsigned int delayAfterClick) :
					core::SimpleMouseInput(param, isEnabled), m_buttonToClick(buttonToClick),
					m_scr_coord(pos), m_delay(delayAfterClick) {
			}
			void execute() override {
				if (enabled) {
					ROBOT_NS::Mouse mouse;
					mouse.SetPos(m_scr_coord);
					mouse.Click(m_buttonToClick);

					// Adding the wait operation after each click (for consistency sake):
					ROBOT_NS::Timer::Sleep (m_delay);
				}
			}
		};

		class MyMouseScroll: public core::SimpleMouseInput
		{
			bool m_isVertical;
			int m_amount;
			unsigned int m_delay;
		public:
			MyMouseScroll(std::string const & param, bool isEnabled,
				bool verticalScroll, int amount, //TODO: use type system to distinguish the boolean flags and int values (so that they are not mixed up)
				unsigned int delayAfterScroll) :
				core::SimpleMouseInput(param, isEnabled), m_isVertical(verticalScroll),
				m_amount(amount), m_delay(delayAfterScroll) {
			}
			void execute() override {
				if (enabled) {
					ROBOT_NS::Mouse mouse;

					if (m_isVertical) {
						mouse.AutoDelay = m_delay;
						mouse.ScrollV(m_amount);
					} else {
						// Note: this is a workaround for the horizontal scroll. We can't just call mouse.ScrollH(m_amount) here because of the issue in the windows implementation of horizontal scrolling.
						// It turns out, that the sendInput() function does not simulate horizontal scroll for several clicks (it assumes that it is the 'wheel tilt' operations)
						// So, the robot's ScrollH() function will act incorrectly here - it will alwyas scroll one unit, regardless to the actual integer value, which is passed to it.
						// Caution: This workaround could become very slow. For big numbers it will lock up the hat tool for quite a long time (simulation of each of the scrolling operations is quite slow).
						// TODO: remove this workaround after the scrolH() is fixed in the robot library.
						// Link for the issue on github: https://github.com/Robot/robot/issues/92
						if (m_amount != 0) {
							auto iterationsCount = (m_amount > 0) ? m_amount : -m_amount;
							auto scrollDirection = (m_amount > 0) ? 1 : -1;
							for (int i = 0; i < iterationsCount; ++i) {
								mouse.ScrollH(scrollDirection);
							}
						}
						ROBOT_NS::Timer::Sleep(m_delay);
					}
				}
			}
		};
		
		class MySleepOperation: public core::SimpleSleepOperation
		{
			
		public:
			MySleepOperation(std::string const & param, unsigned int timeoutInMs, bool isEnabled)
				: SimpleSleepOperation(param, timeoutInMs, isEnabled) {}
			void execute() override {
				if (enabled) {
					ROBOT_NS::Timer::Sleep(m_delay);
				}
			}
		};

		auto lambdaForKeyboardInputObjectsCreation = [&] (std::string const & param, core::CommandID const & commandID, size_t ) {
			ROBOT_NS::KeyList sequence;
			auto result = ROBOT_NS::Keyboard::Compile(param.c_str(), sequence);
			if (!result) {
				std::cout << "Error during decoding of the sequence for the command (id='"
					<< commandID.getValue() << "') by Robot library:\n\t" << param << "\nThe sequence will be disabled.\n";
			}
			auto shouldEnable = result && (param.size() > 0);
			return std::make_shared<MyHotkeyCombination>(param, shouldEnable, sequence, keyboard_intervals);
		};

		auto lambdaForMouseInputObjectsCreation = [&] (std::string const & param, core::CommandID const & commandID, size_t ) {
			auto result = std::shared_ptr<core::SimpleMouseInput>{};
			if (core::ConfigFilesKeywords::MouseEventTypes::isScrollingEvent(param)) {
				auto scrollInfo = getScrollInfo(param);
				result = std::make_shared<MyMouseScroll>(
					param, true, scrollInfo.first, scrollInfo.second, keyboard_intervals);
			} else {
				auto parameterizationParseResult = parseMouseInputCommandInfoString(param);
				result = std::make_shared<MyMouseInput>(param, true, parameterizationParseResult.first, parameterizationParseResult.second, keyboard_intervals);
			}
			return result;
		};

		auto commandsConfig = hat::core::CommandsInfoContainer::parseConfigFile(csvStream, lambdaForKeyboardInputObjectsCreation);

		auto lambdaForSleepObjectsCreation = [&] (std::string const & param, core::CommandID const & commandID, size_t ) {
			auto sleepTimeout = std::stoi(param);
			return std::make_shared<MySleepOperation>(param, sleepTimeout, true);
		};

		loggingCallback("Reading input sequences configs", "");
		for (auto & inputSequencesConfig: inputSequencesConfigs) {
			if (inputSequencesConfig.size() > 0) {
				std::cout << "Starting to read input sequences file '" << inputSequencesConfig << "'\n";
				std::fstream typingsSequensesConfigStream(inputSequencesConfig.c_str());
				loggingCallback("", inputSequencesConfig);
				if (typingsSequensesConfigStream.is_open()) {
					commandsConfig.consumeInputSequencesConfigFile(typingsSequensesConfigStream, lambdaForKeyboardInputObjectsCreation, lambdaForMouseInputObjectsCreation, lambdaForSleepObjectsCreation);
				} else {
					std::cout << "  ERROR: file could not be opened. Please check the path.\n";
				}
			}
		}
		loggingCallback("Reading variables managers configs", "");
		for (auto & variablesManagersConfig: variablesManagersSetupConfigs) { // TODO: [low priority] refactoring: get rid of the code duplication (see loop above)
			if (variablesManagersConfig.size() > 0) {
				std::cout << "Starting to read varaibles managers config file '" << variablesManagersConfig << "'\n";
				std::fstream filestream(variablesManagersConfig.c_str()); 
				loggingCallback("", variablesManagersConfig);
				if (filestream.is_open()) {
					commandsConfig.consumeVariablesManagersConfig(filestream);
				} else {
					std::cout << "  ERROR: could not be opened. Please check the path.\n";
				}
			}
		}

		auto imageResourcesDataAccumulator = hat::core::ImageResourcesInfosContainer{commandsConfig.getEnvironments()};
#ifdef HAT_IMAGES_SUPPORT
		if (imageResourcesConfig.size() > 0) {
			std::cout << "Starting to read the image resources config file '" << imageResourcesConfig << "'\n";
			std::cout << "  And linking them to the commands with environments according to the config file '" << imageId2CommandIdConfig << "'\n";
			std::fstream img_resources_fstream(imageResourcesConfig.c_str());
			if (img_resources_fstream.is_open()) {
				std::fstream img_2_commands_fstream(imageId2CommandIdConfig.c_str());
				if (img_2_commands_fstream.is_open()) {
					loggingCallback("Reading images info configs...", "");
					loggingCallback("", "resources info: " + imageResourcesConfig);
					loggingCallback("", "imageIds 2 commands mappings: " + imageId2CommandIdConfig);
					imageResourcesDataAccumulator.consumeImageResourcesConfig(img_resources_fstream, img_2_commands_fstream);
				} else {
					std::cout << "  ERROR: file '" << imageId2CommandIdConfig << "'could not be opened. Please check the path. Images info will not be loaded.\n";
				}
			} else {
				std::cout << "  ERROR: file '" << imageResourcesConfig<< "'could not be opened. Please check the path. Images info will not be loaded.\n";
			}
		}
#endif //HAT_IMAGES_SUPPORT

		std::fstream configStream(layoutConfig.c_str());
		if (!configStream.is_open()) {
			throw std::runtime_error("Could not find or open the layout config file: " + layoutConfig);
		}
		auto layout = hat::core::LayoutUserInformation::parseConfigFile(configStream);
		return Engine(layout, commandsConfig, imageResourcesDataAccumulator, stickEnvToWindow, keyboard_intervals);
	}

	namespace {
		struct IDsForNavigation
		{
			tau::common::LayoutPageID m_fallbackId;
			tau::common::LayoutPageID m_currentPageID;

			IDsForNavigation(tau::common::LayoutPageID const & fallbackID, tau::common::LayoutPageID const & thisPageID) :
				m_fallbackId(fallbackID), m_currentPageID(thisPageID) {}
			IDsForNavigation createNewSelector(tau::common::LayoutPageID const & newID) const
			{
				//if there is no fallback id, then we are inside generation of the top layout page, so all the selector pages, genetared from it, should get it's ID as an initial fallback.
				return hasFallbackID() ? IDsForNavigation(m_fallbackId, newID) : IDsForNavigation(m_currentPageID, newID);
			}
			bool hasFallbackID() const {
				return m_fallbackId.getValue().size() > 0;
			}
		};
	}
	std::string Engine::getCurrentLayoutJson() const {
		if (m_currentState == LayoutState::NORMAL) {
			if (m_shouldRebuildNormalLayout) {
				m_shouldRebuildNormalLayout = false;
				return getCurrentLayoutJson_normal();
			} else {
				return m_currentNormalLayout.getJson();
			}
		} else if (m_currentState == LayoutState::STICK_ENVIRONMENT_TO_WND) {
			return getCurrentLayoutJson_waitForTopWindowInfo();
		} else if (m_currentState == LayoutState::WAIT_FOR_EXPECTED_WND_AT_FRONT) {
			return getCurrentLayoutJson_wrongTopWindowMessage();
		}
		std::cerr << "Program in invalid state. We should never get here. Exiting.\n";
		abort();
	}

namespace {
	std::string generateLoadingLayoutJson(tau::common::LayoutPageID const & pageID, tau::common::ElementID const & generalLogLabel, tau::common::ElementID const & particularFilesLogLabel) {
		namespace lg = tau::layout_generation;
		auto topElem = lg::UnevenlySplitElementsPair{
			lg::LabelElement("Loading configs...\\nPlease wait."), 
			lg::UnevenlySplitElementsPair{
				lg::LabelElement("...").ID(generalLogLabel),
				lg::LabelElement("...").ID(particularFilesLogLabel),
				true, 0.4},
			true, 0.15};
		auto resultLayout = lg::LayoutInfo{};
		resultLayout.pushLayoutPage(lg::LayoutPage(pageID, topElem));
		return resultLayout.getJson();
	}
}

	LoadingLayoutDataContainer const & Engine::getLayoutJson_loadingConfigsSplashscreen()
	{
		static auto GENERAL_LOG_ID = tau::common::ElementID{generateTrowawayTauIdentifier()};
		static auto PARTICULAR_FILES_LOG_ID = tau::common::ElementID{generateTrowawayTauIdentifier()};
		static LoadingLayoutDataContainer result = LoadingLayoutDataContainer(
			GENERAL_LOG_ID, PARTICULAR_FILES_LOG_ID, generateLoadingLayoutJson(tau::common::LayoutPageID(generateTrowawayTauIdentifier()), GENERAL_LOG_ID, PARTICULAR_FILES_LOG_ID));
		return result;
	}
	
	std::string Engine::getCurrentLayoutJson_wrongTopWindowMessage() const
	{
		namespace lg = tau::layout_generation;
		auto topElem = lg::EvenlySplitLayoutElementsContainer{ true };
		std::stringstream headerMessage;
		headerMessage << "Wrong topmost window for current environment: please bring the window for " << m_commandsConfig.getEnvironments()[m_selectedEnvironment];
		topElem.push(lg::LabelElement(headerMessage.str()));
		topElem.push(lg::ButtonLayoutElement().note("Cancel the current pending command").ID(tau::common::ElementID(generateClearPendingCommandButtonID())));
		topElem.push(lg::ButtonLayoutElement().note("Expected window now on top. Retry the command").ID(tau::common::ElementID(generateResendPendingCommandButtonID())));
		auto resultLayout = lg::LayoutInfo{};
		resultLayout.pushLayoutPage(lg::LayoutPage(tau::common::LayoutPageID(generateTrowawayTauIdentifier()), topElem));
		return resultLayout.getJson();
	}

	std::string Engine::getCurrentLayoutJson_waitForTopWindowInfo() const
	{
		//TODO: get rid of some of the code duplication with the previous method.
		namespace lg = tau::layout_generation;
		auto topElem = lg::EvenlySplitLayoutElementsContainer{ true };
		std::stringstream headerMessage;
		headerMessage << "Please move the target window for " << m_commandsConfig.getEnvironments()[m_selectedEnvironment] << " to the top.";
		topElem.push(lg::LabelElement(headerMessage.str()));
		topElem.push(lg::ButtonLayoutElement().note("Expected window now on top. Proceed").ID(tau::common::ElementID(generateStickEnvironmentToWindowCommand())));
		auto resultLayout = lg::LayoutInfo{};
		resultLayout.pushLayoutPage(lg::LayoutPage(tau::common::LayoutPageID(generateTrowawayTauIdentifier()), topElem));
		return resultLayout.getJson();
	}

	std::string Engine::getCurrentLayoutJson_normal() const
	{
		using namespace std::string_literals;
		
		m_currentlyDisplayedVariables.clear(); // Each time we generate the new normal layout, we have to refresh this mapping.

		hat::core::ConfigsAbstractionLayer layer(m_layoutInfo, m_commandsConfig, m_imagesConfig);
		auto currentLayoutState = layer.generateLayoutPresentation(m_selectedEnvironment, isEnv_selected);

		size_t const TOP_PAGES_COUNT = currentLayoutState.getPages().size();
		TOP_PAGES_IDS.clear(); // these are the pages, which are valid for restoring state to (other layout pages like the options selectors should not be restored to)
		TOP_PAGES_IDS.reserve(TOP_PAGES_COUNT);
		for (size_t i = 0; i < TOP_PAGES_COUNT; ++i) {
			TOP_PAGES_IDS.push_back(tau::common::LayoutPageID(generateTrowawayTauIdentifier()));
		}

		// These 2 variabels are used for the quick jump page - the page, from which the user can jump to any of the pages defined for the given environment.
		// For each top page, we have a button in this quick jump page, from which the given top page can be reached.
		auto pagesQuickJumpPageID = tau::common::LayoutPageID{generateTrowawayTauIdentifier()};
		auto quickJumpPageButtonsContainer = tau::layout_generation::EvenlySplitLayoutElementsContainer(true);

		m_currentNormalLayout = tau::layout_generation::LayoutInfo{};

		//Can't use 'auto' for the lambda type, because this lambda is called recursively, so it's type can't be deduced
		typedef std::function<tau::layout_generation::EvenlySplitLayoutElementsContainer(hat::core::InternalLayoutPageRepresentation const &, IDsForNavigation const & navigationIDs)> MyLambdaType;
		MyLambdaType createLayoutPage =
			[&](hat::core::InternalLayoutPageRepresentation const & userData, IDsForNavigation const & navigationIDs) -> tau::layout_generation::EvenlySplitLayoutElementsContainer {
			auto result = tau::layout_generation::EvenlySplitLayoutElementsContainer{ true };
			for (auto const & row : userData.getLayout()) {
				auto newElementsRow = tau::layout_generation::EvenlySplitLayoutElementsContainer{ false };
				for (auto const & elem : row) {
					
					// Simple lambda, which records the label id with the variable id, which this label represents
					auto linkIdWithTextVariableIfNeeded = [&](tau::common::ElementID const & elementID) {
						if (elem.referencesVariable()) {
							auto variableID = elem.getReferencedVariable();
							m_currentlyDisplayedVariables[variableID].push_back(elementID);
						}
					};

					if (elem.is_button()) {
						auto toPush = tau::layout_generation::ButtonLayoutElement();
						toPush.note(hat::core::escapeRawUTF8_forJson(elem.getNote()));
						{
							auto imageID_string = elem.getImageID().getValue();
							if (imageID_string.size() > 0) {
								toPush.imageID(tau::common::ImageID{imageID_string});
							}
						}
						if (elem.isActive()) {
							if (elem.getReferencedCommand().nonEmpty()) {
								size_t commandIndex = m_commandsConfig.getCommandIndex(elem.getReferencedCommand());
								auto idToUse = tau::common::ElementID{generateTauIdentifierForCommand(commandIndex)};
								linkIdWithTextVariableIfNeeded(idToUse);
								toPush.ID(idToUse);
								if (navigationIDs.hasFallbackID()) {
									//Since this button does not swtich to any page explicitly, we add an automatic fallback switch here
									toPush.switchToAnotherLayoutPageOnClick(navigationIDs.m_fallbackId);
								}
							}
							auto optionsSelectorForElem = elem.getOptionsPagePtr();
							if (optionsSelectorForElem != nullptr) {
								auto newPageID = tau::common::LayoutPageID{ generateTrowawayTauIdentifier() };
								auto newNav = navigationIDs.createNewSelector(newPageID);
								auto newLayoutPage = createLayoutPage(*optionsSelectorForElem, newNav);

								//decorate the page here:
								auto layoutDecorations = tau::layout_generation::EvenlySplitLayoutElementsContainer(true);
								layoutDecorations.push(tau::layout_generation::LabelElement(hat::core::escapeRawUTF8_forJson(elem.getNote())));
								layoutDecorations.push(tau::layout_generation::ButtonLayoutElement()
									.note("back").switchToAnotherLayoutPageOnClick(navigationIDs.m_currentPageID));

								m_currentNormalLayout.pushLayoutPage(tau::layout_generation::LayoutPage(newNav.m_currentPageID,
									tau::layout_generation::UnevenlySplitElementsPair(newLayoutPage, layoutDecorations, true, 0.75)
								));
								toPush.switchToAnotherLayoutPageOnClick(newNav.m_currentPageID);
							}

							auto anotherEnvironmentSwitchingInfo = elem.switchingToAnotherEnvironment_info();
							if (anotherEnvironmentSwitchingInfo.first) {
								auto idToUse = tau::common::ElementID(generateTauIdentifierForEnvSwitching(anotherEnvironmentSwitchingInfo.second));
								linkIdWithTextVariableIfNeeded(idToUse);
								toPush.ID(idToUse);
							}
						} else {
							toPush.setEnabled(false);
						}
						newElementsRow.push(toPush);
					} else {
						//TODO: clean up the code for registering the IDs. Currently we have to call manually linkIdWithTextVariableIfNeeded() function. It's logic should be applied automatically when we generate the ElementID here (and in the code above).
						auto idToUse = tau::common::ElementID(generateTrowawayTauIdentifier());
						linkIdWithTextVariableIfNeeded(idToUse);
						auto elementToAdd = tau::layout_generation::LabelElement(hat::core::escapeRawUTF8_forJson(elem.getNote()));
						elementToAdd.ID(idToUse); // don't forget to assign the ID to the label
						newElementsRow.push(elementToAdd);
					}
				}
				result.push(newElementsRow);
			}
			return result;
		}; // end of lambda that holds the logic for generating the user-defined contents for the layout page (without navigation buttons and auto-generated info label)

		auto selectedEnvCaptionPrefix = isEnv_selected ? ("["s + m_commandsConfig.getEnvironments()[m_selectedEnvironment] + "] "s) : ""s;
		
		auto emptyFallbackID = tau::common::LayoutPageID{ "" };
		for (size_t i = 0; i < TOP_PAGES_COUNT; ++i) {
			auto currentPageID = TOP_PAGES_IDS[i];
			auto & currentPreprocessedPagePresentation = currentLayoutState.getPages()[i];

			auto navigationInfo = IDsForNavigation{ emptyFallbackID, currentPageID };
			auto contents = createLayoutPage(currentPreprocessedPagePresentation, navigationInfo); ///TODO: add m_selectedEnvironment use here

			// NOTE: This lambda has such an ugly name because it is a hack. because of a limitation to the UnevenlySplitElementsPair object we have to do the following:
			// Currently we have to provide both of the child elements for it at the moment of construction (contrary to the EvenlySplitLayoutElementsContainer, which allows push() method)
			// So, we are wrapping the elements of the pair inside the one-element EvenlySplitLayoutElementsContainer objects, from which the result construct is built.
			// This makes the code a lot more readable and allows for creation of the layout structures, which are impossible to create in other ways right now (c++ language will not allow it)
			// TODO: remove the excessive EvenlySplitLayoutElementsContainer objects after the TAU library is fixed (the UnevenlySplitElementsPair should get pushRightOrBottom() and pushLeftOrTop() methods, which will replace the current elements stored in it)
			auto createNavigationButtonWrappedInEvenlySplitLayoutElementsContainer = [&](size_t destIndex) -> tau::layout_generation::EvenlySplitLayoutElementsContainer
			{
				if (destIndex < TOP_PAGES_COUNT) {
					return tau::layout_generation::EvenlySplitLayoutElementsContainer(false)
						.push(tau::layout_generation::ButtonLayoutElement().note(
							hat::core::escapeRawUTF8_forJson(currentLayoutState.getPages()[destIndex].getNote()))
						.switchToAnotherLayoutPageOnClick(tau::common::LayoutPageID(TOP_PAGES_IDS[destIndex])));
				} else {
					return tau::layout_generation::EvenlySplitLayoutElementsContainer(false)
						.push(tau::layout_generation::EmptySpace());
				}
			};

			auto createReloadButtonSegmentWrappedInEvenlySplitLayoutElementsContainer = [&]() -> tau::layout_generation::EvenlySplitLayoutElementsContainer
			{
				return tau::layout_generation::EvenlySplitLayoutElementsContainer(false)
					.push(tau::layout_generation::UnevenlySplitElementsPair(
						tau::layout_generation::ButtonLayoutElement().note("reload").ID(tau::common::ElementID(generateReloadButtonID())),
						tau::layout_generation::EmptySpace(), false, 0.75));
			};

			auto createMiddleButtonSegmentWrappedInEvenlySplitLayoutElementsContainer = [&]() -> tau::layout_generation::EvenlySplitLayoutElementsContainer
			{
				if (TOP_PAGES_COUNT > 1) {
					return tau::layout_generation::EvenlySplitLayoutElementsContainer(false)
						.push(tau::layout_generation::ButtonLayoutElement().note("*").switchToAnotherLayoutPageOnClick(pagesQuickJumpPageID)).ID(tau::common::ElementID(generateTrowawayTauIdentifier()));
				}
				return tau::layout_generation::EvenlySplitLayoutElementsContainer(false)
					.push(tau::layout_generation::EmptySpace());
			};

			auto leftAndMiddlePartOfNavigationButtons = tau::layout_generation::UnevenlySplitElementsPair(
				  (i == 0) ? createReloadButtonSegmentWrappedInEvenlySplitLayoutElementsContainer()
					: createNavigationButtonWrappedInEvenlySplitLayoutElementsContainer(i - 1) // this overflows at zero position, but since the unsigned int overflow is well-defined, we don't have a problem here
				, createMiddleButtonSegmentWrappedInEvenlySplitLayoutElementsContainer()
				, false
				, 0.66);

			auto navigationButtons = tau::layout_generation::UnevenlySplitElementsPair(
				leftAndMiddlePartOfNavigationButtons
				, createNavigationButtonWrappedInEvenlySplitLayoutElementsContainer(i + 1) // Note: this may overflow in theory, but it should go past UINT_MAX for that, which is not a realistic issue
				, false
				, 0.6);

			auto layoutDecorations = tau::layout_generation::UnevenlySplitElementsPair(
				tau::layout_generation::LabelElement(
					hat::core::escapeRawUTF8_forJson(selectedEnvCaptionPrefix + currentPreprocessedPagePresentation.getNote())),
				navigationButtons,
				true, 0.4);

			quickJumpPageButtonsContainer.push(
				tau::layout_generation::ButtonLayoutElement().note(currentPreprocessedPagePresentation.getNote())
				.switchToAnotherLayoutPageOnClick(TOP_PAGES_IDS[i]).ID(tau::common::ElementID{generateTrowawayTauIdentifier()})
			);
			

			m_currentNormalLayout.pushLayoutPage(tau::layout_generation::LayoutPage(currentPageID,
				tau::layout_generation::UnevenlySplitElementsPair(contents, layoutDecorations, true, 0.85)
			));
		}

		if (TOP_PAGES_COUNT > 1) {
			auto const PIXELS_PER_BUTTON{ 75 }; // TODO: 14.08.2018 - make this value configurable through command line
			auto quickJumpPage = tau::layout_generation::LayoutPage(pagesQuickJumpPageID, quickJumpPageButtonsContainer);
			quickJumpPage.height((int)TOP_PAGES_COUNT * PIXELS_PER_BUTTON);
			m_currentNormalLayout.pushLayoutPage(quickJumpPage);
		}

		if ((TOP_PAGES_COUNT > 1) && (m_commandsConfig.getEnvironments().size() > 1)) {
			m_lastTopPageSelected = TOP_PAGES_IDS.begin() + 1;
		} else {
			m_lastTopPageSelected = TOP_PAGES_IDS.begin();
		}
		m_currentNormalLayout.setStartLayoutPage(*m_lastTopPageSelected);

		// We always add all the images as the layout-level references.
		// This way we ensure that the images, which were passed to the client
		// at the very beginning of the session are not evicted from the client's memory
		// during the switching of environments.
		auto allImageIds = m_imagesConfig.getAllRegisteredImageIDs();
		for (auto & imageID : allImageIds) {
			m_currentNormalLayout.addImageReference(tau::common::ImageID(imageID.getValue()));
		}
		auto resultString = m_currentNormalLayout.getJson();
		return resultString;
	}

	void Engine::layoutPageSwitched(tau::common::LayoutPageID const & pageID)
	{
		auto findResult = std::find(TOP_PAGES_IDS.begin(), TOP_PAGES_IDS.end(), pageID);
		if (findResult != TOP_PAGES_IDS.end()) {
			m_lastTopPageSelected = findResult;
		}
	}
	
	hat::core::ImageResourcesInfosContainer::ImagesInfoList Engine::getImagesPhysicalInfos() const
	{
		return m_imagesConfig.getAllRegisteredImages();
	}

	void Engine::addNoteUpdatingFeedbackCallback(std::function<void (tau::common::ElementID const &, std::string)> callback)
	{
		if (m_uiNotesUpdater) {
			throw std::runtime_error("Trying to re-assign the ui notes updater callback. This is not allowed and should never happen.");
		}
		m_uiNotesUpdater = callback;
	}
} //namespace tool
} //namespace hat