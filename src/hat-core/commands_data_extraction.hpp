// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef _COMMANDS_DATA_EXTRACTION_HPP
#define _COMMANDS_DATA_EXTRACTION_HPP

#include "command_id.hpp"
#include <memory>
#include <utility>
#include <map>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <istream>
#include <stdexcept>
#include <functional>
#include <algorithm>

//The code here is responsible for extracting the infromation from the commands.csv configuration file.
namespace hat {
namespace core {

// This function template is used to split a string with the given delimiter character.
// It also allows the callback mechanism (through the CellVerificationFunctor), which is used to handle the elements
// of the split string and control the process.
// Note: the CellVerificationFunctor is a callable object, that takes 3 parameters and returns a boolean flag.
// The 3 parameters are: the current substring to process, index of this substring, a boolean flag, which tells the callback code, if there is any more data to process.
// The return value is a flag, which tells the system, if it should continue the process, or terminate it before the string is processed fully.
template<typename CellVerificationFunctor>
std::vector<std::string> splitTheRow(std::string const & row, char delimiter, CellVerificationFunctor & elementsVerificationFunctor)
{
	using namespace std::string_literals;
	auto cellValue(""s);
	std::vector<std::string> elements;
	std::stringstream mystream(row);

	while (std::getline(mystream, cellValue, delimiter)) {
		if (elementsVerificationFunctor(cellValue, elements.size(), !mystream.eof())) {
			elements.push_back(cellValue);
		}
	}
	return elements;
};
std::vector<std::string> splitTheRow(std::string const & row, char delimiter);
std::tuple<bool, size_t> idStringOk(std::string const & toTest);

struct ConfigFilesKeywords
{
	static std::string const & mandatoryCellsNamesInCommandsCSV() { static std::string result{ "command_id\tcommand_category\tcommand_note\tcommand_description\t" }; return result; };
	static std::string const & simpleTypingSeqCommand()     { static std::string const result{ "simpleTypingJob" }; return result; };
	static std::string const & aggregatedTypingSeqCommand() { static std::string const result{ "commandSequence" }; return result; };
};

//This is a set of tests for the code, which handles the parsing of the data from hotkeys configuration csv.
struct ParsedCsvRow
{
	std::vector<std::string> const m_customColumns;
	ParsedCsvRow() = default;
	ParsedCsvRow(std::vector<std::string> const & customRowsNames) :m_customColumns(customRowsNames) {}
	bool operator == (ParsedCsvRow const & other) const { return m_customColumns == other.m_customColumns; };
	bool operator != (ParsedCsvRow const & other) const { return m_customColumns != other.m_customColumns; };
	static auto parseHeaderString(std::string const & headerString);
	static auto parseDataRowString(std::string const & rowString); // TODO: add environments count parameter (make sure that the length of m_customColumns is consistent (not bigger and not smaller then expected))
};

struct SimpleHotkeyCombination;
struct HotkeyCombinationCollection;
struct AbstractHotkeyCombination
{
	std::string m_value; //TODO: replace with the compiled version of the hotkey
	bool const enabled;
	//todo: generate this class's objects by the static calls
	AbstractHotkeyCombination(std::string const & value) : m_value(value), enabled(value.size() > 0) {};
	AbstractHotkeyCombination(std::string const & value, bool enable) : m_value(value), enabled(enable) {};
	virtual void execute();
	bool isEquivalentTo(AbstractHotkeyCombination const & other) const;

	//TODO: clean up the comparison code. It should not be visible from public interface:
	virtual bool isEquivalentTo_impl(AbstractHotkeyCombination const & other) const = 0;
	virtual bool isEquivalentTo_impl(SimpleHotkeyCombination const & other) const = 0;
	virtual bool isEquivalentTo_impl(HotkeyCombinationCollection const & other) const = 0;
};

struct SimpleHotkeyCombination : public AbstractHotkeyCombination
{	
	SimpleHotkeyCombination(std::string const & value) : AbstractHotkeyCombination(value) {};
	SimpleHotkeyCombination(std::string const & value, bool enable) : AbstractHotkeyCombination(value, enable) {};
	void execute() override { AbstractHotkeyCombination::execute(); };

	bool isEquivalentTo_impl(AbstractHotkeyCombination const & other) const override { return other.isEquivalentTo_impl(*this); };
	bool isEquivalentTo_impl(SimpleHotkeyCombination const & other) const override;
	bool isEquivalentTo_impl(HotkeyCombinationCollection const & other) const override {return false;};
};

struct HotkeyCombinationCollection : public AbstractHotkeyCombination
{
	typedef std::vector<AbstractHotkeyCombination *> CommandsSequence;
private:
	CommandsSequence const m_commandsToExecute;
public:
	HotkeyCombinationCollection(CommandsSequence const & commandsToExecute, std::string const & value) : AbstractHotkeyCombination(value), m_commandsToExecute(commandsToExecute) {};
	HotkeyCombinationCollection(CommandsSequence const & commandsToExecute, std::string const & value, bool enable) : AbstractHotkeyCombination(value, enable), m_commandsToExecute(commandsToExecute) {};
	void execute() override;

	bool isEquivalentTo_impl(AbstractHotkeyCombination const & other) const override { return other.isEquivalentTo_impl(*this); };
	bool isEquivalentTo_impl(SimpleHotkeyCombination const & other) const override { return false; };
	bool isEquivalentTo_impl(HotkeyCombinationCollection const & other) const override;
};

typedef std::function<std::shared_ptr<AbstractHotkeyCombination>(std::string const &, CommandID const & , size_t currentEnvironmentIndex)> HotkeyCombinationFactoryMethod;

struct Command
{
	CommandID const commandID;
	std::string const commandNote;
	std::string const commandGroup;

	typedef std::vector<std::shared_ptr<AbstractHotkeyCombination> > HotkeysForDifferentEnvironments;
	HotkeysForDifferentEnvironments hotkeysForEnvironments;

	Command(std::string const & c_id, std::string const & c_desc, std::string const & c_gr, HotkeysForDifferentEnvironments const & hkeys);
	bool operator == (Command const & other) const;
	static Command create(hat::core::ParsedCsvRow const & data, size_t customColumnsCount, HotkeyCombinationFactoryMethod hotkey_builder);

};

//The class, which knows everything about all the hotkey combinations for all the environments
struct CommandsInfoContainer
{
	typedef std::vector<std::string> EnvsContainer;
	typedef std::vector<Command> CommandsContainer;
	typedef std::map<CommandID, size_t> CommandsMap; //Probably don't need this

	EnvsContainer m_environments;
	CommandsMap m_commandsMap; // maps string to index (not sure if it is actully needed)
	CommandsContainer m_commandsList;

	CommandsInfoContainer(hat::core::ParsedCsvRow const & parsedHeader) :m_environments(parsedHeader.m_customColumns) {}

	CommandID ensureMandatoryCommandAttributesAreCorrect(hat::core::ParsedCsvRow const & data) const;

	void pushDataRow(hat::core::ParsedCsvRow const & data, HotkeyCombinationFactoryMethod hotkey_builder);
	void pushDataRow(hat::core::ParsedCsvRow const & data);
	void pushDataRowForAggregatedCommand(hat::core::ParsedCsvRow const & data);
private:
	void storeCommandObject(CommandID const & commandID, Command const & commandToStore);
public:

	//TODO: make this private (the user code should access commands through indices
	Command const & getCommandPrefs(CommandID const & commandID) const;

	bool hasCommandID(CommandID const & commandID) const;
	size_t getCommandIndex(CommandID const & commandID) const;
	EnvsContainer const & getEnvironments() const;
	std::pair<bool, size_t> getEnvironmentIndex(std::string const & environmentStringId) const;
	CommandsContainer const & getAllCommands() const;
	static CommandsInfoContainer parseConfigFile(std::istream & dataSource, HotkeyCombinationFactoryMethod hotkey_builder);
	void consumeTypingSequencesConfigFile(std::istream & dataSource, HotkeyCombinationFactoryMethod hotkey_builder);
	bool operator == (CommandsInfoContainer const  & other) const;
};

std::ostream & operator << (std::ostream & target, CommandsInfoContainer const & toDump);

} //namespace core
} //namespace hat

#ifdef HAT_CORE_HEADERONLY_MODE
#include "commands_data_extraction.cpp"
#endif //HAT_CORE_HEADERONLY_MODE

#endif //_COMMANDS_DATA_EXTRACTION_HPP
