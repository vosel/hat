// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

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

	mutable bool m_shouldRebuildNormalLayout{ true };
	mutable std::vector<tau::common::LayoutPageID> TOP_PAGES_IDS;
	mutable tau::layout_generation::LayoutInfo m_currentNormalLayout;
	mutable std::vector<tau::common::LayoutPageID>::const_iterator m_lastTopPageSelected;

	hat::core::LayoutUserInformation m_layoutInfo;
	hat::core::CommandsInfoContainer m_commandsConfig;

	Engine(hat::core::LayoutUserInformation const & layoutInfo,
		hat::core::CommandsInfoContainer const & commandsConfig, bool stickEnvToWindow, unsigned int keystrokes_delay);

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
	void layoutPageSwitched(tau::common::LayoutPageID const & pageID);

	static bool canStickToWindows();
	static Engine create(std::string const & commandsCSV, std::vector<std::string> const & typingSequencesConfigs, std::string const & layoutConfig, bool stickEnvToWindow, unsigned int keyboard_intervals);
};

} //namespace tool 
} //namespace hat 
