// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef _COMMANDS_DATA_EXTRACTION_HPP
#define _COMMANDS_DATA_EXTRACTION_HPP

#include "command_id.hpp"
#include <memory>
#include <map>
#include <string>
#include <iostream>
#include <vector>
#include <istream>
#include <stdexcept>
#include <functional>
#include <algorithm>

//The code here is responsible for extracting the infromation from the commands.csv configuration file.
namespace hat {
namespace core {

std::vector<std::string> splitTheRow(std::string const & row, char delimiter, std::function<bool(std::string const &, size_t, bool)> elementsVerificationFunctor);
std::vector<std::string> splitTheRow(std::string const & row, char delimiter);
std::tuple<bool, size_t> idStringOk(std::string const & toTest);


//This is a set of tests for the code, which handles the parsing of the data from hotkeys configuration csv.
struct ParsedCsvRow
{
	std::vector<std::string> const m_customColumns;
	ParsedCsvRow() = default;
	ParsedCsvRow(std::vector<std::string> const & customRowsNames) :m_customColumns(customRowsNames) {}
	bool operator == (ParsedCsvRow const & other) const { return m_customColumns == other.m_customColumns; };
	bool operator != (ParsedCsvRow const & other) const { return m_customColumns != other.m_customColumns; };
	static auto parseHeaderString(std::string const & headerString);
	static auto parseDataRowString(std::string const & rowString); // TODO; add a parameter of environments count (make sure that the length of m_customColumns is consistent (not bigger and not smaller then expected))
};

struct HotkeyCombination
{
	std::string m_value; //TODO: replace with the compiled version of the hotkey
	bool const enabled;
	//todo: generate this class's objects by the static calls
	HotkeyCombination(std::string const & value) : m_value(value), enabled(value.size() > 0) {};
	void execute();
};

struct Command
{
	CommandID const commandID;
	std::string const commandNote;
	std::string const commandGroup;

	typedef std::vector<std::shared_ptr<HotkeyCombination> > HotkeysForDifferentEnvironments;
	HotkeysForDifferentEnvironments hotkeysForEnvironments;

	Command(std::string const & c_id, std::string const & c_desc, std::string const & c_gr, HotkeysForDifferentEnvironments & hkeys);
	bool operator == (Command const & other) const;
	static Command create(hat::core::ParsedCsvRow const & data, size_t customColumnsCount);
};

//The class, which knows everything about all the hotkey combinations for all the environments
struct CommandsInfoContainer
{
	typedef std::vector<std::string> EnvsContainer;
	typedef std::vector<Command> CommandsContainer;
	typedef std::map<CommandID, size_t> CommandsMap; //Probably don't need this

	EnvsContainer const m_environments;
	CommandsMap m_commandsMap; // maps string to index (not sure if it is actully needed)
	CommandsContainer m_commandsList;

	CommandsInfoContainer(hat::core::ParsedCsvRow const & parsedHeader) :m_environments(parsedHeader.m_customColumns) {}
	void pushDataRow(hat::core::ParsedCsvRow const & data);

	//TODO: make this private (the user code should access commands through indices
	Command const & getCommandPrefs(CommandID const & commandID) const;

	bool hasCommandID(CommandID const & commandID) const;
	size_t getCommandIndex(CommandID const & commandID) const;
	EnvsContainer const & getEnvironments() const;

	CommandsContainer const & getAllCommands() const;
	static CommandsInfoContainer parseConfigFile(std::istream & dataSource);
	bool operator == (CommandsInfoContainer const  & other) const;
};

std::ostream & operator << (std::ostream & target, CommandsInfoContainer const & toDump);

} //namespace core
} //namespace hat

#ifdef HAT_CORE_HEADERONLY_MODE
#include "commands_data_extraction.cpp"
#endif //HAT_CORE_HEADERONLY_MODE

#endif //_COMMANDS_DATA_EXTRACTION_HPP
