// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#include <tau/util/basic_events_dispatcher.h>
#include <tau/util/boost_asio_server.h>

#include "engine.hpp"
#ifdef HAT_IMAGES_SUPPORT
#include "images_loader.hpp"
#endif // HAT_IMAGES_SUPPORT
#include <set>
#include <iostream>
#include <memory>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
namespace hat {
namespace tool {
std::string LAYOUT_CONFIG_PATH;
std::string COMMANDS_CONFIG_PATH;
std::string IMAGE_RESOURCES_CONFIG_PATH;
std::string COMMAND_ID_TO_IMAGE_ID_CONFIG_PATH;
std::vector<std::string> INPUT_SEQUENCES_CFG_PATHS;
std::vector<std::string> VARIABLE_MANAGERS_CFG_PATHS;
bool STICK_ENV_TO_WINDOW = false;
unsigned int KEYSTROKES_DELAY = 0;
#ifdef HAT_WINDOWS_SCANCODES_SUPPORT
extern bool SHOULD_USE_SCANCODES = false;
#endif

#ifdef HAT_WINDOWS_CONSOLE_HIDING_FEATURE_SUPPORTED
extern bool SHOULD_HIDE_CONSOLE_WHEN_CLIENTS_ARE_CONNECTED = false;
#endif //HAT_WINDOWS_CONSOLE_HIDING_FEATURE_SUPPORTED


class MyEventsDispatcher;
namespace {
	bool MONITOR_CONNECTIONS_WITH_HEARTBEATS = true;
	size_t const UNANSWERED_HEARTBEATS_LIMIT = 5; //after we send this amount of heartbeats without receiving a reply, we should assume that the connection is no longer active.
	void connectionEstablished(MyEventsDispatcher * dispatcherForTheConnection);
	void connectionClosed(MyEventsDispatcher * dispatcherForTheConnection);
}
class MyEventsDispatcher : public tau::util::BasicEventsDispatcher
{
	std::unique_ptr<Engine> m_engine;

	// This variable is used to establish, if the connection is still alive. So, if we receive any packet from the client, this variable is set to 0 (we don't actually need to account for all of the heartbeat packets, we just try to make sure that the client device is still active)
	size_t m_unanswered_heartbeats_counter;
	bool m_should_reupload_images {true};
	ImageBuffersList m_loadedImagesForConfig{};
public:
	MyEventsDispatcher(
		tau::communications_handling::OutgiongPacketsGenerator & outgoingGeneratorToUse) :
		tau::util::BasicEventsDispatcher(outgoingGeneratorToUse), m_unanswered_heartbeats_counter(0)
	{
	};

	virtual void packetReceived_requestProcessingError(
		std::string const & layoutID, std::string const & additionalData) override
	{
		m_unanswered_heartbeats_counter = 0;
		std::cout << "Error received from client:\nLayoutID: "
			<< layoutID << "\nError: " << additionalData << "\n";
	}
	virtual void packetReceived_buttonClick(
		tau::common::ElementID const & buttonID) override
	{
		m_unanswered_heartbeats_counter = 0;
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
		m_unanswered_heartbeats_counter = 0;
		connectionEstablished(this);
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
		m_unanswered_heartbeats_counter = 0;
		refreshLayout();
	}
	virtual void packetReceived_layoutPageSwitched(tau::common::LayoutPageID const & pageID) override
	{
		m_unanswered_heartbeats_counter = 0;
		m_engine->layoutPageSwitched(pageID);
	}
	virtual void packetReceived_heartbeatResponse() override
	{
		m_unanswered_heartbeats_counter = 0;
	}
public:
	~MyEventsDispatcher() {
		connectionClosed(this);
	}
	void timeToMonitorConnectionState()
	{
		if (m_unanswered_heartbeats_counter >= UNANSWERED_HEARTBEATS_LIMIT) {
			closeConnection();
		} else {
			++m_unanswered_heartbeats_counter;
			sendPacket_heartbeat();
		}
	}
private:
	bool reloadConfigs()
	{
		auto loadingLayoutInfo = Engine::getLayoutJson_loadingConfigsSplashscreen();
		sendPacket_resetLayout(loadingLayoutInfo.second);
		std::string loadingDynamicMessage = "load log:";
		auto refresh_loading_log = [&loadingLayoutInfo, this](std::string const & newMessage) {
			sendPacket_changeElementNote(loadingLayoutInfo.first, newMessage);
		};
		auto add_line_to_client_onscreen_log = [&loadingDynamicMessage, &refresh_loading_log](std::string const & lineToAppendToLog)
		{
			loadingDynamicMessage = loadingDynamicMessage + ("\\n" + lineToAppendToLog);
			refresh_loading_log(loadingDynamicMessage);
		};
		refresh_loading_log(loadingDynamicMessage);
		try {
			m_engine = std::make_unique<Engine>(Engine::create(COMMANDS_CONFIG_PATH, INPUT_SEQUENCES_CFG_PATHS, VARIABLE_MANAGERS_CFG_PATHS, IMAGE_RESOURCES_CONFIG_PATH, COMMAND_ID_TO_IMAGE_ID_CONFIG_PATH, LAYOUT_CONFIG_PATH, STICK_ENV_TO_WINDOW, KEYSTROKES_DELAY, add_line_to_client_onscreen_log));
			m_engine->addNoteUpdatingFeedbackCallback([this](tau::common::ElementID const & elementToUpdate, std::string const & newTextValue) {
				sendPacket_changeElementNote(elementToUpdate, newTextValue);
				
			});
		} catch (std::runtime_error & e) {
			std::cerr << "\n --- Error during reading of the config files:\n" << e.what() << "\n";
			return false;
		}
#ifdef HAT_IMAGES_SUPPORT
		// loading images:
		add_line_to_client_onscreen_log("Starting to load images...");
		try {
			m_loadedImagesForConfig.clear();
			auto imagesToLoad = m_engine->getImagesPhysicalInfos();
			m_loadedImagesForConfig = loadImages(imagesToLoad, add_line_to_client_onscreen_log);
			std::stringstream message;
			message << " ... done (" << m_loadedImagesForConfig.size() << " bitmaps extracted)";
			add_line_to_client_onscreen_log(message.str());
		} catch (std::runtime_error & e) {
			std::cerr << "\n --- Error during loading data from one of the images:\n" << e.what() << "\n";
			return false;
		}
		m_should_reupload_images = true;
#endif // HAT_IMAGES_SUPPORT
		return true;
	}
	void refreshLayout()
	{
#ifdef HAT_IMAGES_SUPPORT
		if (m_should_reupload_images) {
			// Note: we are loading the images into memory in reloadConfig(), but uploading them
			// to the client _after_ the main layout.
			// Since the main layout does not have images (it is environment selection layout),
			// the uploading of images could be done after it.
			// This will make the initial loading feel a little bit snappier.
			m_should_reupload_images = false;
			for (auto & imageInfo : m_loadedImagesForConfig) {
				sendPacket_putImage(imageInfo.first, imageInfo.second);
			}
		}
#endif // HAT_IMAGES_SUPPORT
		auto currentLayout = m_engine->getCurrentLayoutJson();
		sendPacket_resetLayout(currentLayout);
	}
};

bool checkConfigsForErrors() {

	try {
		std::cout << "Checking configuration files for errors ...\n";
		Engine::create(COMMANDS_CONFIG_PATH, INPUT_SEQUENCES_CFG_PATHS, VARIABLE_MANAGERS_CFG_PATHS, IMAGE_RESOURCES_CONFIG_PATH, COMMAND_ID_TO_IMAGE_ID_CONFIG_PATH, LAYOUT_CONFIG_PATH, STICK_ENV_TO_WINDOW, KEYSTROKES_DELAY, [](std::string const &){});
		std::cout << "\t... done.\n";
	} catch (std::runtime_error & e) {
		std::cerr << "\n --- Error during reading of the config files at startup:\n" << e.what() << "\n";
		return false;
	}
	return true;
}

namespace {
	std::set<MyEventsDispatcher *> activeConnections;
	void connectionEstablished(MyEventsDispatcher * dispatcherForTheConnection)
	{
		activeConnections.insert(dispatcherForTheConnection);
#ifdef HAT_WINDOWS_CONSOLE_HIDING_FEATURE_SUPPORTED
		if (SHOULD_HIDE_CONSOLE_WHEN_CLIENTS_ARE_CONNECTED) {
			ShowWindow(GetConsoleWindow(), SW_HIDE);
		}
#endif // HAT_WINDOWS_CONSOLE_HIDING_FEATURE_SUPPORTED
	}

	void connectionClosed(MyEventsDispatcher * dispatcherForTheConnection)
	{
		activeConnections.erase(dispatcherForTheConnection);
#ifdef HAT_WINDOWS_CONSOLE_HIDING_FEATURE_SUPPORTED
		if ((activeConnections.size() == 0) && (SHOULD_HIDE_CONSOLE_WHEN_CLIENTS_ARE_CONNECTED)) {
			ShowWindow(GetConsoleWindow(), SW_SHOW);
		}
#endif // HAT_WINDOWS_CONSOLE_HIDING_FEATURE_SUPPORTED
	}

	auto const TIMER_INTERVAL = boost::posix_time::seconds{1};
	void resetTimer(boost::asio::deadline_timer* t)
	{
		if (hat::tool::MONITOR_CONNECTIONS_WITH_HEARTBEATS) {
			for (auto dispatcher : activeConnections) {
				dispatcher->timeToMonitorConnectionState();
			}
			t->expires_from_now(TIMER_INTERVAL);
			t->async_wait(boost::bind(resetTimer, t));
		}
	}
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
	auto const INPUT_SEQUENCES_CFG = "input_sequences";
	auto const VARIABLE_MANAGERS_CFG = "variable_managers";
	auto const LAYOUT_CFG = "layout";
#ifdef HAT_IMAGES_SUPPORT
	auto const IMAGES_PHYSICAL_INFO_CFG = "image_resources_cfg";
	auto const IMAGE_ID_2_COMMAND_ID_CFG = "images_to_commands_cfg";
#endif // HAT_IMAGES_SUPPORT
	auto const STICK_ENV_TO_WIN = "stickEnvToWindow";

#ifdef HAT_WINDOWS_SCANCODES_SUPPORT
	auto const USE_SCAN_CODES_FOR_KEYBOARD_EMULATION = "useScanCodes";
#endif

#ifdef HAT_WINDOWS_CONSOLE_HIDING_FEATURE_SUPPORTED
	auto const HIDE_CONSOLE = "hideConsole";
#endif // HAT_WINDOWS_CONSOLE_HIDING_FEATURE_SUPPORTED

	namespace po = boost::program_options;
	short port = 12345;
	auto desc = po::options_description{ "Allowed options" };
	desc.add_options()
		(HELP, "Output this help message and exit")
		(VERSION, "Print the tool version and exit")
		(PORT, po::value<short>(), "Set the server listen port")
		(KEYB_DELAY, po::value<unsigned int>(), "delay interval between simulated keystrokes in milliseconds (default is 0 - no delays)")
		(COMMANDS_CFG, po::value<std::string>(), "Filepath to the configuration file, holding the information about commands configurations for each of the environments")
		(INPUT_SEQUENCES_CFG, po::value<std::vector<std::string>>()->multitoken()->composing(), "Filepath(s) to the configuration file(s) for the input sequences (may be omitted). Multiple config files of this type are allowed.")
		(VARIABLE_MANAGERS_CFG, po::value<std::vector<std::string>>()->multitoken()->composing(), "Filepath(s) to the configuration file(s) for describing the behaviour of the internal variables, which chould be displayed on the UI for the user (may be ommitted).")
#ifdef HAT_IMAGES_SUPPORT
		(IMAGES_PHYSICAL_INFO_CFG, po::value<std::string>(), "Filepath to the configuration file, holding the information about images for the buttons (only png images are supported right now)")
		(IMAGE_ID_2_COMMAND_ID_CFG, po::value<std::string>(), "Filepath to the configuration file, holding the linking information between images and the commands+environments in the system")
#endif // HAT_IMAGES_SUPPORT
		(LAYOUT_CFG, po::value<std::string>(), "Filepath to the configuration file, holding the layout information")
		(STICK_ENV_TO_WIN, "If set, the tool will require the user to specify a target window for each environment selected")
#ifdef HAT_WINDOWS_SCANCODES_SUPPORT
		(USE_SCAN_CODES_FOR_KEYBOARD_EMULATION, "If set, the tool will use scan-codes instead of virtual keycodes for keyboard emulation (windows only)")
#endif
#ifdef HAT_WINDOWS_CONSOLE_HIDING_FEATURE_SUPPORTED
		(HIDE_CONSOLE, "If set, the tool will hide the console window when at least 1 client is connected (windows only)")
#endif // HAT_WINDOWS_CONSOLE_HIDING_FEATURE_SUPPORTED
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

#ifdef HAT_WINDOWS_CONSOLE_HIDING_FEATURE_SUPPORTED
	if (vm.count(HIDE_CONSOLE)) {
		std::cout << "\t\'" << HIDE_CONSOLE << "\' option provided. Will hide the console window when at least one client is connected.\n";
		hat::tool::SHOULD_HIDE_CONSOLE_WHEN_CLIENTS_ARE_CONNECTED = true;
		hat::tool::MONITOR_CONNECTIONS_WITH_HEARTBEATS = true;
	}
#endif // HAT_WINDOWS_CONSOLE_HIDING_FEATURE_SUPPORTED

	if (vm.count(INPUT_SEQUENCES_CFG)) {
		hat::tool::INPUT_SEQUENCES_CFG_PATHS = vm[INPUT_SEQUENCES_CFG].as<std::vector<std::string>>();
		
		std::cout << "Input sequences config file(s): [";
		for (auto & configPath : hat::tool::INPUT_SEQUENCES_CFG_PATHS) {
			std::cout << '\'' << configPath << "\', ";
		}
		std::cout << "]\n";
	}

	if (vm.count(VARIABLE_MANAGERS_CFG)) { // TODO: [low priority] refactoring: get rid of code duplication (see the case above)
		hat::tool::VARIABLE_MANAGERS_CFG_PATHS = vm[VARIABLE_MANAGERS_CFG].as<std::vector<std::string>>();

		std::cout << "Variable managers config file(s): [";
		for (auto & configPath : hat::tool::VARIABLE_MANAGERS_CFG_PATHS) {
			std::cout << '\'' << configPath << "\', ";
		}
		std::cout << "]\n";
	}

#ifdef HAT_IMAGES_SUPPORT
	{
		auto physicalConfigsCount = vm.count(IMAGES_PHYSICAL_INFO_CFG);
		auto img2commandConfigsCount = vm.count(IMAGE_ID_2_COMMAND_ID_CFG);
		if (physicalConfigsCount && img2commandConfigsCount) {
			hat::tool::IMAGE_RESOURCES_CONFIG_PATH = vm[IMAGES_PHYSICAL_INFO_CFG].as<std::string>();
			hat::tool::COMMAND_ID_TO_IMAGE_ID_CONFIG_PATH = vm[IMAGE_ID_2_COMMAND_ID_CFG].as<std::string>();
			std::cout << "Image resources config file: '" << hat::tool::IMAGE_RESOURCES_CONFIG_PATH << "'\n";
			std::cout << "'ImageIDs 2 CommandIDs+EnvID' config file: '" << hat::tool::COMMAND_ID_TO_IMAGE_ID_CONFIG_PATH << "'\n";
		} else if (!(!physicalConfigsCount && !img2commandConfigsCount)) {
			std::cout << "Error: only one of the images configs is provided. Need both of them.\n"
				<< "Please provide both '--" << IMAGES_PHYSICAL_INFO_CFG << "' and '--" << IMAGE_ID_2_COMMAND_ID_CFG << "' parameters";
			return 6;
		}
	}
#endif //HAT_IMAGES_SUPPORT

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
		
		//This timer is used for the heartbeats generation. If they are not enabled, the timer will not be enabled inside the resetTimer() function:
		boost::asio::deadline_timer timer(io_service);
		hat::tool::resetTimer(&timer);
		
		std::cout << "Starting server on port " << port << "...\n";
		s.start();
		std::cout << "Calling io_service.run()\n";
		io_service.run();
		return 0;
	}
	return 5; // error in configs at startup
}
