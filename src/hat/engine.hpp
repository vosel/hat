// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef HAT_ENGINE_HPP
#define HAT_ENGINE_HPP

#include "../hat-core/abstract_engine.hpp"
#include "../hat-core/commands_data_extraction.hpp"
#include "../hat-core/user_defined_layout.hpp"
#include "../hat-core/configs_abstraction_layer.hpp"
#include "../external_dependencies/robot/Source/Window.h"
#include <tau/layout_generation/layout_info.h>

#include <functional>
namespace hat {
namespace tool {
//TODO: refactor this class implementation

struct LoadingLayoutDataContainer {
	tau::common::ElementID generalLoadingStepsLogLabel;
	tau::common::ElementID particularFilesLogLabel;
	std::string layoutJson;
	LoadingLayoutDataContainer(tau::common::ElementID const & generalStepsLabel, tau::common::ElementID const & particularFilesLabel, std::string const & layoutJson)
		: generalLoadingStepsLogLabel(generalStepsLabel), particularFilesLogLabel(particularFilesLabel), layoutJson(layoutJson)
	{}
};

class Engine : public hat::core::AbstractEngine
{
	enum class LayoutState
	{
		NORMAL,
		STICK_ENVIRONMENT_TO_WND,
		WAIT_FOR_EXPECTED_WND_AT_FRONT
	};
	LayoutState m_currentState{ LayoutState::NORMAL };
	size_t m_selectedEnvironment;
	bool isEnv_selected;
	bool m_stickEnvToWindow;
	std::vector<ROBOT_NS::uintptr> m_stickInfo;
	unsigned int m_keystrokes_delay;
	
	// This is a simple callback function, which allows us to request UI updates on the client device (updates of the elements notes are done through this callback)
	// We have to register this updater in separate step during engine initialisation step. This could be avoided if the change the hat::core::AbstractEngine into a template. This way we will be able to add 'tau' library's 'ElementID' type to the interface methods 'AbstractEngine', without adding to the hat::core project a dependency on 'tau'.
	// TODO: try to rework the AbstractEngine into a template, which will make the engine code more streamlined and type-safe.
	std::function<void (tau::common::ElementID const &, std::string)> m_uiNotesUpdater;

	mutable bool m_shouldRebuildNormalLayout{ true };
	mutable std::vector<tau::common::LayoutPageID> TOP_PAGES_IDS;
	mutable tau::layout_generation::LayoutInfo m_currentNormalLayout;
	mutable std::vector<tau::common::LayoutPageID>::const_iterator m_lastTopPageSelected;
	
	// A mapping of the variables to the list of layout element IDs, which display that variables.
	// this map is auto-refreshed each time the normal layout is re-generated.
	mutable std::map<hat::core::VariableID, std::vector<tau::common::ElementID>> m_currentlyDisplayedVariables;
	
	hat::core::LayoutUserInformation m_layoutInfo;
	hat::core::CommandsInfoContainer m_commandsConfig;
	hat::core::ImageResourcesInfosContainer m_imagesConfig;

	Engine(hat::core::LayoutUserInformation const & layoutInfo,
		hat::core::CommandsInfoContainer const & commandsConfig,
		hat::core::ImageResourcesInfosContainer const & imagesConfig,
		bool stickEnvToWindow, unsigned int keystrokes_delay);

	bool shouldShowEnvironmentSelectionPage() const;
	std::string getCurrentLayoutJson_normal() const;
	std::string getCurrentLayoutJson_wrongTopWindowMessage() const;
	std::string getCurrentLayoutJson_waitForTopWindowInfo() const;

protected:
	bool setNewEnvironment(size_t envIndex) override;
	virtual bool canSendTheCommmandForEnvironment() const override;
	virtual void executeCommandForCurrentlySelectedEnvironment(size_t commandIndex) override;
	virtual void stickCurrentTopWindowToSelectedEnvironment() override;
	virtual void switchLayout_wrongTopmostWindow() override;
	virtual void switchLayout_restoreToNormalLayout() override;
public:
	std::string getCurrentLayoutJson() const;
	void addNoteUpdatingFeedbackCallback(std::function<void (tau::common::ElementID const &, std::string)> callback);
	void layoutPageSwitched(tau::common::LayoutPageID const & pageID);
	
	hat::core::ImageResourcesInfosContainer::ImagesInfoList getImagesPhysicalInfos() const;
	
	static bool canStickToWindows();
	static Engine create(std::string const & commandsCSV, std::vector<std::string> const & inputSequencesConfigs, std::vector<std::string> const & variablesManagersSetupConfigs, std::string const & imageResourcesConfig, std::string const & imageId2CommandIdConfig, std::string const & layoutConfig, bool stickEnvToWindow, unsigned int keyboard_intervals, std::function<void(std::string const &, std::string const &)> loggingCallback);
	
	static LoadingLayoutDataContainer const & getLayoutJson_loadingConfigsSplashscreen();
	//Platform-independent sleep operation
	static void sleep(unsigned int millisec);
};

} //namespace tool 
} //namespace hat 

#endif //HAT_ENGINE_HPP
