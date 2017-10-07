// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#include <tau/util/basic_events_dispatcher.h>
#include <tau/util/boost_asio_server.h>

#include "engine.hpp"
#include <iostream>
#include <memory>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
namespace hat {
namespace tool {
std::string LAYOUT_CONFIG_PATH;
std::string COMMANDS_CONFIG_PATH;
std::vector<std::string> TYPING_SEQUENCES_CFG_PATHS;
bool STICK_ENV_TO_WINDOW = false;
unsigned int KEYSTROKES_DELAY = 0;
#ifdef HAT_WINDOWS_SCANCODES_SUPPORT
extern bool SHOULD_USE_SCANCODES = false;
#endif



class MyEventsDispatcher : public tau::util::BasicEventsDispatcher
{
	std::unique_ptr<Engine> m_engine;
public:
	MyEventsDispatcher(
		tau::communications_handling::OutgiongPacketsGenerator & outgoingGeneratorToUse) :
		tau::util::BasicEventsDispatcher(outgoingGeneratorToUse)
	{
	};

	virtual void packetReceived_requestProcessingError(
		std::string const & layoutID, std::string const & additionalData) override
	{
		std::cout << "Error received from client:\nLayouID: "
			<< layoutID << "\nError: " << additionalData << "\n";
	}
	virtual void packetReceived_buttonClick(
		tau::common::ElementID const & buttonID) override
	{
		switch (m_engine->buttonOnLayoutClicked(buttonID.getValue())) {
		case hat::core::FeedbackFromButtonClick::RELOAD_CONFIGS:
			if (!reloadConfigs()) {
				std::cerr << "Configuration parsing failed. The layout will not be refreshed.\n";
				break;
			}
			refreshLayout();
			break;
		case hat::core::FeedbackFromButtonClick::UPDATE_LAYOUT:
			refreshLayout();
			break;
		default: break;
		}
	}
	virtual void onClientConnected(
		tau::communications_handling::ClientConnectionInfo const & connectionInfo) override
	{
		std::cout << "Client connected: remoteAddr: "
			<< connectionInfo.getRemoteAddrDump()
			<< ", localAddr : "
			<< connectionInfo.getLocalAddrDump() << "\n";
		if (!reloadConfigs()) {
			std::cerr << "Initial configuration parsing failed. Closing the connection.\n";
			closeConnection();;
		}
	}
	virtual void packetReceived_clientDeviceInfo(
		tau::communications_handling::ClientDeviceInfo const & info) override
	{
		refreshLayout();
	}
	virtual void packetReceived_layoutPageSwitched(tau::common::LayoutPageID const & pageID) override
	{
		m_engine->layoutPageSwitched(pageID);
	}
private:
	bool reloadConfigs()
	{
		try {
			m_engine = std::make_unique<Engine>(Engine::create(COMMANDS_CONFIG_PATH, TYPING_SEQUENCES_CFG_PATHS, LAYOUT_CONFIG_PATH, STICK_ENV_TO_WINDOW, KEYSTROKES_DELAY));
		} catch (std::runtime_error & e) {
			std::cerr << "\n --- Error during reading of the config files:\n" << e.what() << "\n";
			return false;
		}
		return true;
	}
	void refreshLayout()
	{
		auto currentLayout = m_engine->getCurrentLayoutJson();
		sendPacket_resetLayout(currentLayout);
	}
};

bool checkConfigsForErrors() {

	try {
		std::cout << "Checking configuration files for errors ...\n";
		Engine::create(COMMANDS_CONFIG_PATH, TYPING_SEQUENCES_CFG_PATHS, LAYOUT_CONFIG_PATH, STICK_ENV_TO_WINDOW, KEYSTROKES_DELAY);
		std::cout << "\t... done.\n";
	} catch (std::runtime_error & e) {
		std::cerr << "\n --- Error during reading of the config files at startup:\n" << e.what() << "\n";
		return false;
	}
	return true;
}

}// namespace tool
}// namespace hat 


int main(int argc, char ** argv)
{
	auto const VERSION_STR = "0.1.0";
	// --------------------------------- Command line parameters parsing --------------------------------- 
	auto const HELP = "help";
	auto const VERSION = "version";
	auto const PORT = "port";
	auto const KEYB_DELAY = "keysDelay";
	auto const COMMANDS_CFG = "commands";
	auto const TYPING_SEQUENCES_CFG = "typing_sequences";
	auto const LAYOUT_CFG = "layout";
	auto const STICK_ENV_TO_WIN = "stickEnvToWindow";

#ifdef HAT_WINDOWS_SCANCODES_SUPPORT
	auto const USE_SCAN_CODES_FOR_KEYBOARD_EMULATION = "useScanCodes";
#endif

	namespace po = boost::program_options;
	short port = 12345;
	auto desc = po::options_description{ "Allowed options" };
	desc.add_options()
		(HELP, "Output this help message and exit")
		(VERSION, "Print the tool version and exit")
		(PORT, po::value<short>(), "Set the server listen port")
		(KEYB_DELAY, po::value<unsigned int>(), "delay interval between simulated keystrokes in milliseconds (default is 0 - no delays)")
		(COMMANDS_CFG, po::value<std::string>(), "Filepath to the configuration file, holding the information about commands configurations for each of the environments")
		(TYPING_SEQUENCES_CFG, po::value<std::vector<std::string>>()->multitoken()->composing(), "Filepath(s) to the configuration file(s) for the typing sequences (may be omitted). Multiple config files of this type are allowed.")
		(LAYOUT_CFG, po::value<std::string>(), "Filepath to the configuration file, holding the layout information")
		(STICK_ENV_TO_WIN, "If set, the tool will require the user to specify a target window for each environment selected")
#ifdef HAT_WINDOWS_SCANCODES_SUPPORT
		(USE_SCAN_CODES_FOR_KEYBOARD_EMULATION, "If set, the tool will use scan-codes instead of virtual keycodes for keyboard emulation (windows only)")
#endif
		;

	po::variables_map vm;

	try {
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);
	} catch (std::exception const & e) {
		std::cerr << "Error parsing command line arguments. Please specify --help for the details on the allowed options\n" << e.what();
		return 1;
	}

	if ((vm.count(VERSION) > 0) || (vm.count(HELP) > 0)) {
		std::cout << "HAT (Hotkey Abstraction Tool) " << VERSION_STR << "\n";
		std::cout << "Copyright (C) 2016 Yuriy Vosel, https://github.com/vosel\n";
		if (vm.count(HELP)) {
			std::cout << desc;
		}
		return 2;
	}

	if (vm.count(COMMANDS_CFG)) {
		hat::tool::COMMANDS_CONFIG_PATH = vm[COMMANDS_CFG].as<std::string>();
		std::cout << "Commands config file: " << hat::tool::COMMANDS_CONFIG_PATH << "\n";
	} else {
		std::cout << "Required option '--" << COMMANDS_CFG << "' not provided. Exiting.\n";
		return 3;
	}

	if (vm.count(TYPING_SEQUENCES_CFG)) {
		hat::tool::TYPING_SEQUENCES_CFG_PATHS = vm[TYPING_SEQUENCES_CFG].as<std::vector<std::string>>();
		
		std::cout << "Typing sequences config file(s): [";
		for (auto & configPath : hat::tool::TYPING_SEQUENCES_CFG_PATHS) {
			std::cout << '\'' << configPath << "\', ";
		}
		std::cout << "]\n";
	}

	if (vm.count(LAYOUT_CFG)) {
		hat::tool::LAYOUT_CONFIG_PATH = vm[LAYOUT_CFG].as<std::string>();
		std::cout << "Layout config file: " << hat::tool::LAYOUT_CONFIG_PATH << "\n";
	} else {
		std::cout << "Required option '--" << LAYOUT_CFG << "' not provided. Exiting.\n";
		return 4;
	}
	if (vm.count(STICK_ENV_TO_WIN)) {
		if (hat::tool::Engine::canStickToWindows()) {
			hat::tool::STICK_ENV_TO_WINDOW = true;
			std::cout << "Setting 'stick to window' flag to true.\n";
		} else {
			std::cout << "Could not enable the 'stick to window' option - it is not supported by current system. Ignoring the '" << STICK_ENV_TO_WIN << "' command line argument.\n";
		}
	}
#ifdef HAT_WINDOWS_SCANCODES_SUPPORT
	if (vm.count(USE_SCAN_CODES_FOR_KEYBOARD_EMULATION)) {
		hat::tool::SHOULD_USE_SCANCODES = true;
	}
#endif
	if (vm.count(PORT)) {
		port = vm[PORT].as<short>();
	}
	std::cout << "server will listen on port " << port << " for incoming connections\n";

	if (vm.count(KEYB_DELAY) > 0) {
		std::cout << "delay for the simulated keyboard events is set to " << vm[KEYB_DELAY].as<unsigned int>() << "\n";
		hat::tool::KEYSTROKES_DELAY = vm[KEYB_DELAY].as<unsigned int>();
	}

	// --------------------------------- Command line parsing done. Starting the server. --------------------------------- 

	// Trying to read configs - make sure that everything is ok.
	if (hat::tool::checkConfigsForErrors()) {
		boost::asio::io_service io_service;
		tau::util::SimpleBoostAsioServer<hat::tool::MyEventsDispatcher>::type s(io_service, port);
		std::cout << "Starting server on port " << port << "...\n";
		s.start();
		std::cout << "Calling io_service.run()\n";
		io_service.run();
		return 0;
	}
	return 5; // error in configs at startup
}
