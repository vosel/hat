// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef _COMMANDS_DATA_EXTRACTION_HPP
#define _COMMANDS_DATA_EXTRACTION_HPP

#include "command_id.hpp"
#include "variables_manager.hpp"
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
	static std::string const & simpleMouseInputCommand()    { static std::string const result{ "mouseInput" }; return result; };
	static std::string const & aggregatedSetOfCommands() { static std::string const result{ "commandSequence" }; return result; };
	static std::string const & sleepOperationCommand()   { static std::string const result{ "sleepForTimeout" }; return result; };
	static std::string const & systemCallCommand()       { static std::string const result{ "systemCall" }; return result; };
	struct MouseButtonTypes {
		static std::string const & LeftButton() { static std::string const result{ "L" }; return result; };
		static std::string const & RightButton() { static std::string const result{ "R" }; return result; };
		static std::string const & MiddleButton() { static std::string const result{ "M" }; return result; };
		static std::string const & X1Button() { static std::string const result{ "X1" }; return result; };
		static std::string const & X2Button() { static std::string const result{ "X2" }; return result; };
		
		static std::string const & Scroll_pref()  { static std::string const result{ "scroll" }; return result; };
		static bool isScrollingEvent(std::string const & toTest) { return toTest.find(Scroll_pref()) != std::string::npos; };
		
		//vertical scroll operations:
		static std::string const & ScrollV()      { static std::string const result{ Scroll_pref() + "_v" }; return result; };
		static std::string const & ScrollV_up()   { static std::string const result{ ScrollV() + "_up" }; return result; };
		static std::string const & ScrollV_down() { static std::string const result{ ScrollV() + "_down" }; return result; };
		
		//horizontal scroll operations:
		static std::string const & ScrollH()      { static std::string const result{ Scroll_pref() + "_h" }; return result; };
		static std::string const & ScrollH_left() { static std::string const result{ ScrollH() + "_left" }; return result; };
		static std::string const & ScrollH_right(){ static std::string const result{ ScrollH() + "_right" }; return result; };
	};

	// Keywords for specifying the text feedback behaviours
	static std::string const & defineInternalLabelVariable()                 { static std::string const result{ "defineVariable" }; return result; };
	static std::string const & textFeedbackSetInitialValue()                 { static std::string const result{ "initValueForVar" }; return result; };
	static std::string const & textFeedbackAssignValue()                     { static std::string const result{ "variableUpdate_reset" }; return result; };
	static std::string const & textFeedbackAppendValue()                     { static std::string const result{ "variableUpdate_append" }; return result; };
	static std::string const & textFeedbackClearLastCharacter()              { static std::string const result{ "variableUpdate_backspace" }; return result; };
	static std::string const & textFeedbackAppendFromAnotherVariable()       { static std::string const result{ "variableUpdate_appendValueFromAnotherLabel" }; return result; };
	static std::string const & textFeedbackAssignFromAnotherVariable()       { static std::string const result{ "variableUpdate_assignValueFromAnotherLabel" }; return result; };
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
struct SimpleMouseInput;
struct SimpleSleepOperation;
struct SystemCall;
struct InputSequencesCollection;
struct AbstractSimulatedUserInput
{
	std::string m_value; //text representation of the operation (generally, as it is written down in the config file)
	bool const enabled;
	//todo: generate this class's objects by the static calls
	AbstractSimulatedUserInput(std::string const & value) : m_value(value), enabled(value.size() > 0) {};
	AbstractSimulatedUserInput(std::string const & value, bool enable) : m_value(value), enabled(enable) {};
	virtual void execute();
	bool isEquivalentTo(AbstractSimulatedUserInput const & other) const;

	//TODO: clean up the comparison code. It should not be visible from public interface:
	virtual bool isEquivalentTo_impl(AbstractSimulatedUserInput const & other) const = 0;
	virtual bool isEquivalentTo_impl(SimpleHotkeyCombination const & other) const = 0;
	virtual bool isEquivalentTo_impl(SimpleMouseInput const & other) const  = 0;
	virtual bool isEquivalentTo_impl(SimpleSleepOperation const & other) const = 0;
	virtual bool isEquivalentTo_impl(SystemCall const & other) const = 0;
	virtual bool isEquivalentTo_impl(InputSequencesCollection const & other) const = 0;
};

struct SimpleHotkeyCombination : public AbstractSimulatedUserInput
{	
	SimpleHotkeyCombination(std::string const & value) : AbstractSimulatedUserInput(value) {};
	SimpleHotkeyCombination(std::string const & value, bool enable) : AbstractSimulatedUserInput(value, enable) {};
	void execute() override { AbstractSimulatedUserInput::execute(); };

	bool isEquivalentTo_impl(AbstractSimulatedUserInput const & other) const override { return other.isEquivalentTo_impl(*this); };
	bool isEquivalentTo_impl(SimpleHotkeyCombination const & other) const override;
	bool isEquivalentTo_impl(SimpleMouseInput const & other) const override { return false; };
	bool isEquivalentTo_impl(SimpleSleepOperation const & other) const override { return false; };
	bool isEquivalentTo_impl(SystemCall const & other) const override { return false; };
	bool isEquivalentTo_impl(InputSequencesCollection const & other) const override {return false;};
};

struct SimpleMouseInput : public AbstractSimulatedUserInput
{
	SimpleMouseInput(std::string const & value) : AbstractSimulatedUserInput(value) {};
	SimpleMouseInput(std::string const & value, bool enable) : AbstractSimulatedUserInput(value, enable) {};
	void execute() override { AbstractSimulatedUserInput::execute(); };
	 
	bool isEquivalentTo_impl(AbstractSimulatedUserInput const & other) const override { return other.isEquivalentTo_impl(*this); };
	bool isEquivalentTo_impl(SimpleHotkeyCombination const & other) const override { return false; };
	bool isEquivalentTo_impl(SimpleMouseInput const & other) const override;
	bool isEquivalentTo_impl(SimpleSleepOperation const & other) const override { return false; };
	bool isEquivalentTo_impl(SystemCall const & other) const override { return false; };
	bool isEquivalentTo_impl(InputSequencesCollection const & other) const override {return false;};
};
struct SimpleSleepOperation : public AbstractSimulatedUserInput
{
	unsigned int m_delay;
	SimpleSleepOperation (std::string const & param, unsigned int timeoutInMs, bool isEnabled)
		: AbstractSimulatedUserInput(param, isEnabled), m_delay(timeoutInMs) {}
	void execute() override { AbstractSimulatedUserInput::execute(); };
	bool isEquivalentTo_impl(AbstractSimulatedUserInput const & other) const override { return other.isEquivalentTo_impl(*this); };
	bool isEquivalentTo_impl(SimpleHotkeyCombination const & other) const override { return false; };
	bool isEquivalentTo_impl(SimpleMouseInput const & other) const override { return false; };
	bool isEquivalentTo_impl(SimpleSleepOperation const & other) const override;
	bool isEquivalentTo_impl(SystemCall const & other) const override { return false; };
	bool isEquivalentTo_impl(InputSequencesCollection const & other) const override {return false;};
};

struct SystemCall : AbstractSimulatedUserInput {
	SystemCall(std::string const & commandToExecute) : AbstractSimulatedUserInput(commandToExecute) {};
	void execute() override { system(m_value.c_str()); };
	bool isEquivalentTo_impl(AbstractSimulatedUserInput const & other) const override { return other.isEquivalentTo_impl(*this); };
	bool isEquivalentTo_impl(SimpleHotkeyCombination const & other) const override { return false; };
	bool isEquivalentTo_impl(SimpleMouseInput const & other) const override { return false; };
	bool isEquivalentTo_impl(SimpleSleepOperation const & other) const override { return false; };
	bool isEquivalentTo_impl(SystemCall const & other) const override;
	bool isEquivalentTo_impl(InputSequencesCollection const & other) const override {return false;};
};

struct InputSequencesCollection : public AbstractSimulatedUserInput
{
	typedef std::vector<AbstractSimulatedUserInput *> CommandsSequence;
private:
	CommandsSequence const m_commandsToExecute;
public:
	InputSequencesCollection(CommandsSequence const & commandsToExecute, std::string const & value) : AbstractSimulatedUserInput(value), m_commandsToExecute(commandsToExecute) {};
	InputSequencesCollection(CommandsSequence const & commandsToExecute, std::string const & value, bool enable) : AbstractSimulatedUserInput(value, enable), m_commandsToExecute(commandsToExecute) {};
	void execute() override;

	bool isEquivalentTo_impl(AbstractSimulatedUserInput const & other) const override { return other.isEquivalentTo_impl(*this); };
	bool isEquivalentTo_impl(SimpleHotkeyCombination const & other) const override { return false; };
	bool isEquivalentTo_impl(SimpleMouseInput const & other) const override { return false; };
	bool isEquivalentTo_impl(SimpleSleepOperation const & other) const override { return false; };
	bool isEquivalentTo_impl(SystemCall const & other) const override { return false; };
	bool isEquivalentTo_impl(InputSequencesCollection const & other) const override;
};

typedef std::function<std::shared_ptr<AbstractSimulatedUserInput>(std::string const &, CommandID const & , size_t currentEnvironmentIndex)> HotkeyCombinationFactoryMethod;
typedef std::function<std::shared_ptr<AbstractSimulatedUserInput>(std::string const &, CommandID const & , size_t currentEnvironmentIndex)> MouseInputsFactoryMethod;
typedef std::function<std::shared_ptr<AbstractSimulatedUserInput>(std::string const &, CommandID const & , size_t currentEnvironmentIndex)> SleepInputsFactoryMethod;


struct Command
{
	CommandID const commandID;
	std::string const commandNote;
	std::string const commandGroup;

	typedef std::vector<std::shared_ptr<AbstractSimulatedUserInput> > HotkeysForDifferentEnvironments;
	HotkeysForDifferentEnvironments hotkeysForEnvironments;

	Command(std::string const & c_id, std::string const & c_desc, std::string const & c_gr, HotkeysForDifferentEnvironments const & hkeys);
	bool operator == (Command const & other) const;
	static Command create(hat::core::ParsedCsvRow const & data, size_t customColumnsCount, HotkeyCombinationFactoryMethod hotkey_builder);

};

struct VariablesDataForEnvironments {
	typedef std::vector<VariablesManager> VariablesManagersForEnvironments;
	VariablesDataForEnvironments(size_t environments_count)
		: m_variablesForEnvironments(environments_count) {}
	void declareVariableForAllEnvironments(VariableID const & variableID)
	{
		for (auto & variablesManagerForEnv : m_variablesForEnvironments) {
			variablesManagerForEnv.declareVariable(variableID);
		}
	}
	VariablesManager const & getManagerForEnv_c(size_t environmentIndex) const
	{
		return m_variablesForEnvironments[environmentIndex];
	}

	VariablesManager & getManagerForEnv(size_t environmentIndex)
	{
		return m_variablesForEnvironments[environmentIndex];
	}
	
	bool isVariableDeclaredForAll(VariableID const & toTest) const
	{
		for (auto & env : m_variablesForEnvironments) {
			if (!env.variableExists(toTest)) {
				return false;
			}
		}
		return true;
	}
private:
	VariablesManagersForEnvironments m_variablesForEnvironments;
};

//The class, which knows everything about all the hotkey combinations for all the environments
// It also holds other information that is differentiated by environment.
// For example it holds separate 'variableManager' object for each of the environments.
struct CommandsInfoContainer
{
	typedef std::vector<std::string> EnvsContainer;
	typedef std::vector<Command> CommandsContainer;
	typedef std::map<CommandID, size_t> CommandsMap; //Probably don't need this

	EnvsContainer m_environments;
private: //TODO: make all the fields in this class private
	VariablesDataForEnvironments m_variables;
public:
	VariablesDataForEnvironments & getVariablesManagers() { return m_variables; };
	VariablesDataForEnvironments const & getVariablesManagers_c() const { return m_variables; };
	bool isVariableDeclaredInManagers(VariableID const & toTest) const { return m_variables.isVariableDeclaredForAll(toTest); };
	CommandsMap m_commandsMap; // maps string to index (not sure if it is actully needed)
	CommandsContainer m_commandsList;

	CommandsInfoContainer(hat::core::ParsedCsvRow const & parsedHeader) : m_environments(parsedHeader.m_customColumns), m_variables(parsedHeader.m_customColumns.size()) {}

	CommandID ensureMandatoryCommandAttributesAreCorrect(hat::core::ParsedCsvRow const & data) const;

	void pushDataRow(hat::core::ParsedCsvRow const & data, HotkeyCombinationFactoryMethod hotkey_builder);
	void pushDataRowForMouseInput(hat::core::ParsedCsvRow const & data, MouseInputsFactoryMethod mouse_inputs_builder);
	void pushDataRowForSleepOperation(hat::core::ParsedCsvRow const & data, SleepInputsFactoryMethod mouse_inputs_builder);
	void pushDataRowForSystemCallCommand(hat::core::ParsedCsvRow const & data);
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
	void consumeInputSequencesConfigFile(std::istream & dataSource, HotkeyCombinationFactoryMethod hotkey_builder, MouseInputsFactoryMethod mouse_inputs_builder, SleepInputsFactoryMethod sleep_objects_builder);
	void consumeVariablesManagersConfig(std::istream & dataSource);
	bool operator == (CommandsInfoContainer const  & other) const;
};

std::ostream & operator << (std::ostream & target, CommandsInfoContainer const & toDump);

} //namespace core
} //namespace hat

#ifdef HAT_CORE_HEADERONLY_MODE
#include "commands_data_extraction.cpp"
#endif //HAT_CORE_HEADERONLY_MODE

#endif //_COMMANDS_DATA_EXTRACTION_HPP
