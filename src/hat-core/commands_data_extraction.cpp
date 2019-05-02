// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef HAT_CORE_HEADERONLY_MODE
#include "commands_data_extraction.hpp"
#endif
#include "utils.hpp"
#include <iostream>
#include <sstream>

#ifndef HAT_CORE_HEADERONLY_MODE
#define LINKAGE_RESTRICTION 
#else
#define LINKAGE_RESTRICTION inline
#endif

namespace hat {
namespace core {

LINKAGE_RESTRICTION bool AbstractSimulatedUserInput::isEquivalentTo(AbstractSimulatedUserInput const & other) const
{
	return isEquivalentTo_impl(other);
}

namespace {
	bool checkSimpleEquivalence(AbstractSimulatedUserInput const & first, AbstractSimulatedUserInput const & second)
	{
		return (first.enabled == second.enabled) && (first.m_value == second.m_value);
	}
}

LINKAGE_RESTRICTION bool SimpleHotkeyCombination::isEquivalentTo_impl(SimpleHotkeyCombination const & other) const
{
	return checkSimpleEquivalence(*this, other);
}

LINKAGE_RESTRICTION bool SimpleMouseInput::isEquivalentTo_impl(SimpleMouseInput const & other) const
{
	return checkSimpleEquivalence(*this, other);
}

LINKAGE_RESTRICTION bool SimpleSleepOperation::isEquivalentTo_impl(SimpleSleepOperation const & other) const
{
	return (enabled == other.enabled) && (m_delay == other.m_delay);
}

LINKAGE_RESTRICTION bool SystemCall::isEquivalentTo_impl(SystemCall const & other) const
{
	return (enabled == other.enabled) && (m_value == other.m_value);
}

LINKAGE_RESTRICTION bool InputSequencesCollection::isEquivalentTo_impl(InputSequencesCollection const & other) const
{
	return checkSimpleEquivalence(*this, other);
}

LINKAGE_RESTRICTION Command::Command(std::string const & c_id, std::string const & c_desc, std::string const & c_gr, HotkeysForDifferentEnvironments const & hkeys) :
	commandID(c_id),
	commandNote(c_desc),
	commandGroup(c_gr),
	hotkeysForEnvironments(hkeys)
{
}

LINKAGE_RESTRICTION bool Command::operator == (Command const & other) const
{
	return (commandID == other.commandID) &&
		(commandNote == other.commandNote) &&
		(commandGroup == other.commandGroup) && 
		(hotkeysForEnvironments.size() == other.hotkeysForEnvironments.size()) &&
		std::equal(
			std::begin(hotkeysForEnvironments), std::end(hotkeysForEnvironments), std::begin(other.hotkeysForEnvironments),
			[](HotkeysForDifferentEnvironments::value_type const & left, HotkeysForDifferentEnvironments::value_type const & right) {
				return left->isEquivalentTo(*right);
			});
}

LINKAGE_RESTRICTION Command Command::create(hat::core::ParsedCsvRow const & data, size_t customColumnsCount, HotkeyCombinationFactoryMethod hotkey_builder)
{
	auto c_id = data.m_customColumns[0];
	auto c_gr = data.m_customColumns[1];
	auto c_desc = data.m_customColumns[2];
	auto target = HotkeysForDifferentEnvironments{};
	target.reserve(customColumnsCount);
	for (size_t i = 0; i < customColumnsCount; ++i) {
		if (i < data.m_customColumns.size() - 4) {
			target.push_back(hotkey_builder(data.m_customColumns[i + 4], CommandID(c_id), i));
		} else {
			target.push_back(std::make_shared<SimpleHotkeyCombination>("", false)); // just an empty disabled combination. Maybe will also generate it through hotkey_builder though
		}
	}
	return Command(c_id, c_desc, c_gr, target);
}

LINKAGE_RESTRICTION std::vector<std::string> splitTheRow(std::string const & row, char delimiter)
{
	auto verificationLambda = [](std::string const & extractedString, size_t indexForString, bool moreDataInStream) -> bool {
		return true;
	};
	return splitTheRow(row, delimiter, verificationLambda);
}

LINKAGE_RESTRICTION std::tuple<bool, size_t> idStringOk(std::string const & toTest) {
	auto index = toTest.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_1234567890");
	if (index != std::string::npos) {
		// TODO: add a special command line flag to display this kind of stuff in the console:
		// std::cerr << "Error: forbidden symbol found in id string : '" << toTest << "' at position " << index << " (symbol: '" << toTest[index] << "').\n";
		return std::make_tuple(false, index);
	}
	return std::make_tuple(true, 0);
}
LINKAGE_RESTRICTION void ensureIdStringOk(std::string const & toTest)
{
	auto idStingCheck = idStringOk(toTest);
	if (!std::get<0>(idStingCheck)) {
		std::stringstream errormessage;
		errormessage << "Forbidden symbol in the id string value at position " << std::get<1>(idStingCheck) << ". The ID string: '" << toTest << "'.";
		throw std::runtime_error(errormessage.str());
	}
}
LINKAGE_RESTRICTION auto ParsedCsvRow::parseHeaderString(std::string const & headerString)
{
	const auto & leadingRequiredCharacters = ConfigFilesKeywords::mandatoryCellsNamesInCommandsCSV();
	if (headerString.find(leadingRequiredCharacters) != 0) {
		throw std::runtime_error("Error during parsing commands config. Header string should start with the specific elements: " + leadingRequiredCharacters);
	}
	const auto toParse(headerString.substr(leadingRequiredCharacters.size(), headerString.size()));
	auto myLambda = [](std::string const & extractedString, size_t indexForString, bool moreDataInStream) -> bool {
		if ((extractedString.size() == 0) && moreDataInStream) {
			std::stringstream errorMessage;
			errorMessage << "Empty attribute in the header string at position " << indexForString << " - this is not allowed";
			throw std::runtime_error(errorMessage.str());
		}
		return true;
	};
	return ParsedCsvRow(splitTheRow(toParse, '\t', myLambda));
}
namespace
{
	auto getCsvCommandDataRowElementsProcessor()
	{
		return [](std::string const & extractedString, size_t indexForString, bool moreDataInStream) -> bool {
			if (indexForString == 0) {
				if (extractedString.size() == 0) {
					std::stringstream errormessage;
					errormessage << "Each row should have a string id for reperenceing it.";
					throw std::runtime_error(errormessage.str());
				} else {
					ensureIdStringOk(extractedString);
				}
			} else if (indexForString == 1) {
				auto categoryStingCheck = idStringOk(extractedString);
				if (!std::get<0>(categoryStingCheck)) {
					std::stringstream errormessage;
					errormessage << "Forbidden symbol in the category string value at position " << std::get<1>(categoryStingCheck) << ". The categoryID string: '" << extractedString << "'.";
					throw std::runtime_error(errormessage.str());
				}
			} else if ((indexForString == 2) && (extractedString.size() == 0)) {
				throw std::runtime_error("Each row should have a non-empty string note to associate with it.");
			}
			return true;
		};
	}
}
LINKAGE_RESTRICTION auto ParsedCsvRow::parseDataRowString(std::string const & rowString)
{
	auto myLambda = getCsvCommandDataRowElementsProcessor();
	auto rowElements = splitTheRow(rowString, '\t', myLambda);
	return ParsedCsvRow(rowElements);
}

LINKAGE_RESTRICTION CommandsInfoContainer::CommandsContainer const & CommandsInfoContainer::getAllCommands() const
{
	return m_commandsList;
}

namespace {

std::pair<bool, size_t> util_getEnvironmentIndex(CommandsInfoContainer::EnvsContainer const & environmentsContainer, std::string const & environmentStringId) {
	auto begin = std::begin(environmentsContainer);
	auto end = std::end(environmentsContainer);

	auto foundElem = std::find(begin, end, environmentStringId);
	return {foundElem != end, std::distance(begin, foundElem)};
}

}
LINKAGE_RESTRICTION std::pair<bool, size_t> CommandsInfoContainer::getEnvironmentIndex(std::string const & environmentStringId) const
{
	return util_getEnvironmentIndex(m_environments, environmentStringId);
}

LINKAGE_RESTRICTION CommandsInfoContainer::EnvsContainer const & CommandsInfoContainer::getEnvironments() const
{
	return m_environments;
}

LINKAGE_RESTRICTION size_t CommandsInfoContainer::getCommandIndex(CommandID const & commandID) const
{
	return m_commandsMap.at(commandID);
}

LINKAGE_RESTRICTION bool CommandsInfoContainer::hasCommandID(CommandID const & commandID) const
{
	return m_commandsMap.find(commandID) != m_commandsMap.end();
}

LINKAGE_RESTRICTION Command const & CommandsInfoContainer::getCommandPrefs(CommandID const & commandID) const
{
	//todo: should return index also
	size_t commandIndex = getCommandIndex(commandID);
	return m_commandsList[commandIndex];
}

LINKAGE_RESTRICTION bool CommandsInfoContainer::operator == (CommandsInfoContainer const  & other) const
{
	return ((m_environments == other.m_environments) && (m_commandsList == other.m_commandsList)); // TODO: implement this properly
}

namespace {
template <typename LinesProcessingCallableObject>
void processFileStream(std::istream & dataSource, size_t lineCounterStart, std::string const & fileTypeDescription, LinesProcessingCallableObject & dataProcessor, bool shouldClearByteOrderMarkOnFirstLine) {
	auto tmpStr = std::string{};
	size_t lineCount = lineCounterStart;
	auto firstLine = true;
	while (getLineFromFile(dataSource, tmpStr)) {
		if (shouldClearByteOrderMarkOnFirstLine && firstLine) { // This is a special case
			tmpStr = clearUTF8_byteOrderMark(tmpStr);
			firstLine = false;
		}
		++lineCount;
		try {
			size_t firstNonSpaceCharacterInd = tmpStr.find_first_not_of(" \t");
			if ((firstNonSpaceCharacterInd != std::string::npos)
				&& (tmpStr[firstNonSpaceCharacterInd] != '#')) // Empty (without any non-space symbols) or commented out lines are just skipped here
			{
				dataProcessor(tmpStr);
			}
		} catch (std::runtime_error & e) {
			std::stringstream errorMessage;
			errorMessage << "Error parsing the " << fileTypeDescription << " configuration file at line " << lineCount << ":\n\t";
			errorMessage << e.what();
			throw std::runtime_error(errorMessage.str());
		}
	}
}
}

LINKAGE_RESTRICTION CommandsInfoContainer CommandsInfoContainer::parseConfigFile(std::istream & dataSource, HotkeyCombinationFactoryMethod hotkey_builder)
{
	auto tmpStr = std::string{};
	if (!getLineFromFile(dataSource, tmpStr)) {
		throw std::runtime_error("No data in the commands config stream.");
	}
	auto result = CommandsInfoContainer(ParsedCsvRow::parseHeaderString(clearUTF8_byteOrderMark(tmpStr)));

	auto dataLineProcessor = [&result, &hotkey_builder](std::string const & lineToProcess) {
		result.pushDataRow(ParsedCsvRow::parseDataRowString(lineToProcess), hotkey_builder);
	};
	processFileStream(dataSource, 1, "command", dataLineProcessor, false);
	return result;
}

namespace {

template <typename ValueType>
ValueType getElementOrThrow(
	std::map<std::string, ValueType> const & elementsMatchups, 
	std::string const & stringToDispatch,
	std::string const & typeOfFileForErrorMessage)
{
	if (elementsMatchups.count(stringToDispatch) > 0) {
		return elementsMatchups.at(stringToDispatch);
	} else {
		std::stringstream error;
		error << "Unknown type of the command in the " << typeOfFileForErrorMessage << " file. This is what is enterpreted as the command type string (it may be caused by use of space delimiter instead of <TAB>): \n'" << stringToDispatch << "'";
		throw std::runtime_error(error.str());
	}
}

// This is a simple function, which processes the environments info string and sets the corresponding flags for the environments, which are mentioned in this string.
// It is extracted from the MyInputSequencesDataProcessor class in order to avoid future code duplications.
// preconditions: flagsForEnabledEnvironments.size() should be equal to the amount of environments in the system.
//                environmentToIndexConverter should have the getEnvironmentIndex() function, which allows us to convert environment's string identifier into it's index
void util_parseEnvironmentsEnablingString(
	std::string const & stringToParse, 
	std::vector<char> & flagsForEnabledEnvironments,
	CommandsInfoContainer::EnvsContainer const & environmentsList,
	std::string const & configTypeForErrorMessage)
{
	if (stringToParse == "*") {
		flagsForEnabledEnvironments.assign(
			environmentsList.size(), 1); // setting the flags to 'all of the environments enabled'
	} else {
		auto elements = splitTheRow(stringToParse, ',');
		for (auto & environment : elements) {
			auto indexFindResult = util_getEnvironmentIndex(environmentsList, environment);
			if (indexFindResult.first) {
				flagsForEnabledEnvironments[indexFindResult.second] = 1; // enable the given environment
			} else {
				std::stringstream error;
				error << "Unknown environment id in the " << configTypeForErrorMessage << ": " << environment
					<< "\nWhole environments string: " << stringToParse;
				throw std::runtime_error(error.str());
			}
		}
	}
}

// Simple utility function for converting a string (which is read from config file) into a number, which it is supposed to represent
size_t getUintFromStringOrThrow(std::string const & toParse)
{
	auto pos = toParse.find_first_not_of("0123456789");
	if (pos != std::string::npos) {
		std::stringstream error;
		error << "unexpected symbol in the string representing number (position - " << pos << "): " << toParse;
		throw std::runtime_error(error.str());
	}
	size_t result {0};
	std::stringstream mystream(toParse);
	mystream >> result;
	return result;
}

// This is a simple class, which is used for processing the data lines read from the input_sequences config file by the splitTheRow() funcion.
// Currently there are 2 types of the data lines in this config files: simple and aggregative.
// Simple rows hold the command as the robot's format string, the aggregative rows hold commands,
// which are created from a set of another commands, which should be executed one after another.
// Format of the row string:    <typeOfRow>\t<idOfCommand>\t<environments,for which the command is enabled>\t<command data>
class MyInputSequencesDataProcessor {
	enum class TypeOfRow {
		SIMPLE_KEYBOARD_INPUT, SIMPLE_MOUSE_INPUT, SLEEP_OPERATION, SYSTEM_CALL, AGGREGATE, UNKNOWN
	};
	TypeOfRow m_type = TypeOfRow::UNKNOWN;
	CommandsInfoContainer const & m_targetContainerRef;
	std::vector<std::string> m_accumulatedRawDataCells;
	std::vector<char> m_shouldEnableCommandForGivenEnv;
	std::string m_commandData;

public:
	MyInputSequencesDataProcessor(CommandsInfoContainer const & targeContainerRef):m_targetContainerRef(targeContainerRef) {
		m_shouldEnableCommandForGivenEnv.assign(
			m_targetContainerRef.getEnvironments().size(), 0); // setting the flags to 'none of the environments enabled'
	}

	// This is a verification operator that gets called when an object of this class is passed to the splitTheRow() function.
	bool operator()(std::string const & extractedString, size_t indexForString, bool moreDataInStream) {
		if (indexForString == 0) {
			if (extractedString == ConfigFilesKeywords::simpleTypingSeqCommand()) {
				m_type = TypeOfRow::SIMPLE_KEYBOARD_INPUT;
			} else if (extractedString == ConfigFilesKeywords::simpleMouseInputCommand()) {
				m_type = TypeOfRow::SIMPLE_MOUSE_INPUT;
			} else if (extractedString == ConfigFilesKeywords::sleepOperationCommand()) {
				m_type = TypeOfRow::SLEEP_OPERATION;
			} else if (extractedString == ConfigFilesKeywords::systemCallCommand()) {
				m_type = TypeOfRow::SYSTEM_CALL;
			} else if (extractedString == ConfigFilesKeywords::aggregatedSetOfCommands()) {
				m_type = TypeOfRow::AGGREGATE;
			} else {
				std::stringstream error;
				error << "Unknown type of the command in the input sequences file. This is what is enterpreted as the command type string (it may be caused by use of space delimiter instead of <TAB>): \n'" << extractedString << "'";
				throw std::runtime_error(error.str());
			}
		} else if ((indexForString >= 1) && (indexForString <= 4)) {
			if ((TypeOfRow::SIMPLE_KEYBOARD_INPUT == m_type)
				|| (TypeOfRow::SIMPLE_MOUSE_INPUT == m_type)
				|| (TypeOfRow::SLEEP_OPERATION == m_type)
				|| (TypeOfRow::SYSTEM_CALL == m_type)
				|| (TypeOfRow::AGGREGATE == m_type))
			{
				m_accumulatedRawDataCells.push_back(extractedString);
				return getCsvCommandDataRowElementsProcessor()(extractedString, indexForString - 1, moreDataInStream);
			} else {
				throw std::runtime_error("Unsupported data type"); //TODO: add better error message here
			}
		} else if (indexForString == 5) { //determine the environments, for which this command is enabled from the environments string:
			util_parseEnvironmentsEnablingString(extractedString, m_shouldEnableCommandForGivenEnv, m_targetContainerRef.m_environments, "input sequences file");
		} else if (indexForString == 6) {
			m_commandData = extractedString;
		} else {
			std::stringstream error;
			error << "Wrong format for the input sequences file's row: it has too many \\t symbols. There are some excessive symbols. Please check the documentation on the format of the data.";
			throw std::runtime_error(error.str());
		}
		return true;
	}

	// This method determines the type of data, which should be pushed into the target container and passes the data to it in the needed format.
	void storeAccumulatedDataTo(CommandsInfoContainer & target, HotkeyCombinationFactoryMethod hotkey_builder, MouseInputsFactoryMethod mouse_inputs_builder, SleepInputsFactoryMethod sleep_objects_builder)
	{
		for (auto flag : m_shouldEnableCommandForGivenEnv) { // here we finish generating synthetic representation of the simple command (as if it was typed inside the csv_commands file), and then pass the data to the target container.
			m_accumulatedRawDataCells.push_back((flag == 1) ? m_commandData : "");
		}
		if (TypeOfRow::SIMPLE_KEYBOARD_INPUT == m_type) {
			target.pushDataRow(hat::core::ParsedCsvRow(m_accumulatedRawDataCells), hotkey_builder);
		} else if (TypeOfRow::SIMPLE_MOUSE_INPUT == m_type) {
			target.pushDataRowForMouseInput(
				hat::core::ParsedCsvRow(m_accumulatedRawDataCells), mouse_inputs_builder);
		} else if (TypeOfRow::SLEEP_OPERATION == m_type) {
			target.pushDataRowForSleepOperation(
				hat::core::ParsedCsvRow(m_accumulatedRawDataCells), sleep_objects_builder);
		} else if (TypeOfRow::SYSTEM_CALL == m_type) {
			target.pushDataRowForSystemCallCommand(
				hat::core::ParsedCsvRow(m_accumulatedRawDataCells));
		} else if (TypeOfRow::AGGREGATE == m_type) {
			target.pushDataRowForAggregatedCommand(
				hat::core::ParsedCsvRow(m_accumulatedRawDataCells));
		} else {
			// Unreachable code
			throw std::runtime_error("Unsupported data type. Note: this exception should never be thrown (the issue should've been detected earlier).");
		}
	}
};

//TODO: [low priority] refactor this code: this class was copy-pasted from MyInputSequencesDataProcessor class, so there is some duplication with it. Will have to clean it up.
class TextFeedbackLogicConfigurationProcessor {
	enum class TypeOfRow {
		VARIABLE_DECLARATION,
		INITIAL_VALUE_FOR_VARIABLE,
		ASSIGN_TEXT,
		APPEND_TEXT,
		CLEAR_LAST_CHARACTERS,
		ASSIGN_VALUE,
		APPEND_VALUE,
		UNKNOWN
	};
	static std::map<std::string, TypeOfRow> const & getRowTypesMap() {
		static std::map<std::string, TypeOfRow> 
			mymap{ 
				 {ConfigFilesKeywords::defineInternalLabelVariable(), TypeOfRow::VARIABLE_DECLARATION}
				,{ConfigFilesKeywords::textFeedbackSetInitialValue(), TypeOfRow::INITIAL_VALUE_FOR_VARIABLE}
				,{ConfigFilesKeywords::textFeedbackAssignValue(), TypeOfRow::ASSIGN_TEXT}
				,{ConfigFilesKeywords::textFeedbackAppendValue(), TypeOfRow::APPEND_TEXT}
				,{ConfigFilesKeywords::textFeedbackClearLastCharacter(), TypeOfRow::CLEAR_LAST_CHARACTERS}
				,{ConfigFilesKeywords::textFeedbackAppendFromAnotherVariable(), TypeOfRow::APPEND_VALUE}
				,{ConfigFilesKeywords::textFeedbackAssignFromAnotherVariable(), TypeOfRow::ASSIGN_VALUE}
			};
		return mymap;
	}
	TypeOfRow m_type = TypeOfRow::UNKNOWN;

	CommandsInfoContainer const & m_targetContainerRef;
	std::vector<std::string> m_accumulatedRawDataCells;
	std::vector<char> m_shouldEnableCommandForGivenEnv;

	VariableID m_variableID;
	CommandID m_triggerCommand;
	std::string m_stringParameterValue;
public:
	TextFeedbackLogicConfigurationProcessor(CommandsInfoContainer const & targeContainerRef)
		: m_targetContainerRef(targeContainerRef)
	{
		m_shouldEnableCommandForGivenEnv.assign(
			m_targetContainerRef.getEnvironments().size(), 0); // setting the flags to 'none of the environments enabled'
	}

	void ensureTargetVariableExistsAndStoreIt(std::string const & value) {
		m_variableID = VariableID{ value };
		if (!(m_targetContainerRef.isVariableDeclaredInManagers(m_variableID))) {
			std::stringstream error;
			error << "Unknown variable id (" << value << ") provided as a target for the operation. Please declare a variable before using it.";
			throw std::runtime_error(error.str());
		}
	}

	bool operator()(std::string const & extractedString, size_t indexForString, bool moreDataInStream)
	{
		auto throwErrorOnTooManyStringElements = []()
		{
			std::stringstream error;
			error << "Wrong format for the text feedback file's row: it has too many \\t symbols. Please check the documentation on the format of the data.";
			throw std::runtime_error(error.str());
		};
		auto throwOnNotEnoughArguments = [&]() {
			std::stringstream error;
			error << "Not enough arguments provided to the variable operation string. Please make sure that all the parameters are there.";
			throw std::runtime_error(error.str());
		};
		
		if (indexForString == 0) {
			m_type = getElementOrThrow<TypeOfRow>(getRowTypesMap(), extractedString, "text feedback");
			if (!moreDataInStream) {
				throwOnNotEnoughArguments();
			}
		} else if (indexForString == 1) { //determine the environments, for which this command is enabled from the environments string:
			if (TypeOfRow::VARIABLE_DECLARATION == m_type) {
				
				//Note: here we should not verify the existance of the variable (since we are declaring it right now). But we should check the re-definition
				m_variableID = VariableID::createFromUserString(extractedString);
				
				if (m_targetContainerRef.isVariableDeclaredInManagers(m_variableID)) {
					std::stringstream error;
					error << "Variable redefinition (variableID = " << m_variableID.getValue() << "). This is not allowed.";
					throw std::runtime_error(error.str());
				}
				if (moreDataInStream) { // last element for this type of row, so if there is more data, we should throw error
					throwErrorOnTooManyStringElements();
				}
			} else {
				if (extractedString.size() == 0) {
					std::stringstream error;
					error << "Error: no environments specified for the variable operation. It will have no effect during runtime. This is not allowed in variable manager's configuratoin. Either add at least one environment, or comment this line out.";
					throw std::runtime_error(error.str());
				}
				util_parseEnvironmentsEnablingString(extractedString, m_shouldEnableCommandForGivenEnv, m_targetContainerRef.m_environments, "input sequences file");
				if (!moreDataInStream) {
					throwOnNotEnoughArguments();
				}
			}
		} else if (indexForString == 2) {
			if (TypeOfRow::INITIAL_VALUE_FOR_VARIABLE == m_type) {
				ensureTargetVariableExistsAndStoreIt(extractedString);
				if (!moreDataInStream) {
					throwOnNotEnoughArguments();
				}
			} else {
				m_triggerCommand = CommandID{extractedString};
				if (!m_targetContainerRef.hasCommandID(m_triggerCommand)) {
					std::stringstream error;
					error << "Unknown trigger command referenced by the variable operation descriptor: " << extractedString;
					throw std::runtime_error(error.str());
				}
			}
		} else if (indexForString == 3) {
			if (TypeOfRow::INITIAL_VALUE_FOR_VARIABLE == m_type) {
				m_stringParameterValue = extractedString;
				if (moreDataInStream) { //last element for this type of row, so if there is more data, we should throw error
					throwErrorOnTooManyStringElements();
				}
			} else {
				if (!moreDataInStream) {
					throwOnNotEnoughArguments();
				}
				ensureTargetVariableExistsAndStoreIt(extractedString);
			}
		} else if (indexForString == 4) {
			m_stringParameterValue = extractedString;
			if ((TypeOfRow::ASSIGN_VALUE == m_type) || (TypeOfRow::APPEND_VALUE == m_type)) {
				if (!m_targetContainerRef.isVariableDeclaredInManagers(VariableID{m_stringParameterValue})) {
					std::stringstream error;
					error << "Trying to append/assign value from an unknown variable (variableID = " << m_stringParameterValue << ". Please declare the variable before referencing it.";
					throw std::runtime_error(error.str());
				}
			}
			if (moreDataInStream) {
				throwErrorOnTooManyStringElements();
			}
		}
		return true;
	}
	void storeAccumulatedDataTo(CommandsInfoContainer & target)
	{
		// Some sugar: 2 lambdas for making the code below less dense:
		auto forEachEnabledEnv = [&](std::function<void (VariablesManager & )> operation) {
			for (size_t i = 0; i < m_shouldEnableCommandForGivenEnv.size(); ++i) {
				if (m_shouldEnableCommandForGivenEnv[i] == 1) {
					operation(target.getVariablesManagers().getManagerForEnv(i));
				}
			}
		};

		auto addOnCommandOperation = 
			[&](VariablesManager & manager, std::shared_ptr<VariableOperation> operation)
		{
			auto commandIndex = target.getCommandIndex(m_triggerCommand);
			manager.addOperationToExecuteOnCommand(commandIndex, operation);
		};

		switch (m_type) {
		case TypeOfRow::VARIABLE_DECLARATION:
			target.getVariablesManagers().declareVariableForAllEnvironments(m_variableID);
			break;
		case TypeOfRow::INITIAL_VALUE_FOR_VARIABLE:
			forEachEnabledEnv([&](VariablesManager & manager) {
				manager.setVariableInitialValue(m_variableID, m_stringParameterValue);
			});
			break;
		case TypeOfRow::ASSIGN_TEXT:
			forEachEnabledEnv([&](VariablesManager & manager) {
				addOnCommandOperation(manager, std::make_shared<AssignText>(
					m_variableID, m_stringParameterValue));
			});
			break;
		case TypeOfRow::APPEND_TEXT:
			forEachEnabledEnv([&](VariablesManager & manager) {
				addOnCommandOperation(manager, std::make_shared<AppendText>(
					m_variableID, m_stringParameterValue));
			});
			break;
		case TypeOfRow::CLEAR_LAST_CHARACTERS:
		{
			size_t const charactersCount =
				(m_stringParameterValue.size() == 0) ? 
					0 : getUintFromStringOrThrow(m_stringParameterValue);
			if (charactersCount != 0) { // if character count is 0, then this operation will not do anything, so it is fine not to add it.
				forEachEnabledEnv([&](VariablesManager & manager) {
					addOnCommandOperation(manager, std::make_shared<ClearTailCharacters>(
						m_variableID, charactersCount));
				});
			} // else {} //TODO: report warning to the user here (the 'ClearTailCharacters' operation with '0' parameter is discarded)
			break;
		}
		case TypeOfRow::ASSIGN_VALUE:
			forEachEnabledEnv([&](VariablesManager & manager) {
				addOnCommandOperation(manager, std::make_shared<AssignValue>(
					m_variableID, VariableID{ m_stringParameterValue }));
			});
			break;
		case TypeOfRow::APPEND_VALUE:
			forEachEnabledEnv([&](VariablesManager & manager) {
				addOnCommandOperation(manager, std::make_shared<AppendValue>(
					m_variableID, VariableID{ m_stringParameterValue }));
			});
			break;
		case TypeOfRow::UNKNOWN:
			throw std::runtime_error("Unsupported text feedback row type. Note: this exception should never be thrown (the issue should've been detected earlier)."); //unreachable code
		}
	}
};

}

LINKAGE_RESTRICTION void CommandsInfoContainer::consumeInputSequencesConfigFile(std::istream & dataSource, HotkeyCombinationFactoryMethod hotkey_builder, MouseInputsFactoryMethod mouse_inputs_builder, SleepInputsFactoryMethod sleep_objects_builder)
{
	auto dataLineProcessor = [this, &hotkey_builder, &mouse_inputs_builder, &sleep_objects_builder](std::string const & lineToProcess) {
		MyInputSequencesDataProcessor rowProcessor(*this);
		auto rowElements = splitTheRow(lineToProcess, '\t', rowProcessor);
		rowProcessor.storeAccumulatedDataTo(*this, hotkey_builder, mouse_inputs_builder, sleep_objects_builder);
	};
	processFileStream(dataSource, 0, "input sequences", dataLineProcessor, true);
}

LINKAGE_RESTRICTION void CommandsInfoContainer::consumeVariablesManagersConfig(std::istream & dataSource)
{
	auto dataLineProcessor = [this](std::string const & lineToProcess) {
		TextFeedbackLogicConfigurationProcessor rowProcessor(*this);
		auto rowElements = splitTheRow(lineToProcess, '\t', rowProcessor);
		rowProcessor.storeAccumulatedDataTo(*this);
	};
	processFileStream(dataSource, 0, "text variables behaviour logic", dataLineProcessor, true);
}

LINKAGE_RESTRICTION void CommandsInfoContainer::pushDataRow(hat::core::ParsedCsvRow const & data)
{
	auto myLambda =  [] (std::string const & param, CommandID const & commandID, size_t env_index) {
		return std::make_shared<SimpleHotkeyCombination>(param);		
	};
	pushDataRow(data, myLambda);
}

LINKAGE_RESTRICTION CommandID CommandsInfoContainer::ensureMandatoryCommandAttributesAreCorrect(hat::core::ParsedCsvRow const & data) const
{
	using namespace std::string_literals;
	if (data.m_customColumns.size() > m_environments.size() + 4) { //TODO - refactoring: get rid of this magic number
		std::stringstream errorMessage;
		errorMessage << "Too much parameters for the row element. The max amount of parameters should be ('num of environments' + 4). (4 additional parameters are command id, group, button note and description)";
		throw std::runtime_error(errorMessage.str());
	}
	auto commandID = CommandID{ data.m_customColumns[0] };
	if (m_commandsMap.find(commandID) != m_commandsMap.end()) {
		std::stringstream errorMessage;
		errorMessage << "A duplicate command id found: '" << commandID.getValue() << "'. This is not allowed.";
		throw std::runtime_error(errorMessage.str());
	}
	return commandID;
}

LINKAGE_RESTRICTION void CommandsInfoContainer::pushDataRow(hat::core::ParsedCsvRow const & data, HotkeyCombinationFactoryMethod hotkey_builder)
{
	auto commandID = ensureMandatoryCommandAttributesAreCorrect(data);
	storeCommandObject(commandID, Command::create(data, m_environments.size(), hotkey_builder));
}

LINKAGE_RESTRICTION void CommandsInfoContainer::pushDataRowForMouseInput(hat::core::ParsedCsvRow const & data, MouseInputsFactoryMethod mouse_inputs_builder)
{
	auto commandID = ensureMandatoryCommandAttributesAreCorrect(data);
	storeCommandObject(commandID, Command::create(data, m_environments.size(), mouse_inputs_builder));
}

LINKAGE_RESTRICTION void CommandsInfoContainer::pushDataRowForSleepOperation(hat::core::ParsedCsvRow const & data, SleepInputsFactoryMethod sleep_inputs_builder)
{
	auto commandID = ensureMandatoryCommandAttributesAreCorrect(data);
	storeCommandObject(commandID, Command::create(data, m_environments.size(), sleep_inputs_builder));
}

LINKAGE_RESTRICTION void CommandsInfoContainer::pushDataRowForSystemCallCommand(hat::core::ParsedCsvRow const & data)
{
	auto commandID = ensureMandatoryCommandAttributesAreCorrect(data);
	storeCommandObject(commandID, Command::create(data, m_environments.size(),
		[&](std::string const & param, CommandID const & commandId, size_t currentEnvironmentIndex) {
			return std::make_shared<SystemCall>(param);
		}
	));
}

LINKAGE_RESTRICTION void CommandsInfoContainer::pushDataRowForAggregatedCommand(hat::core::ParsedCsvRow const & data)
{
	auto commandID = ensureMandatoryCommandAttributesAreCorrect(data);
	auto myLambda = [this](std::string const & commandStringRepresentation, CommandID const & commandID, size_t env_index) {
		auto commandIDs = splitTheRow(commandStringRepresentation, ',');
		auto commandsPointers = InputSequencesCollection::CommandsSequence{};
		auto should_enable = false;
		for (auto & currentCommandID : commandIDs) {
			auto key = CommandID{ currentCommandID };
			if (hasCommandID(key)) {
				auto commandPrefs = getCommandPrefs(key);
				commandsPointers.push_back(commandPrefs.hotkeysForEnvironments[env_index].get());
				should_enable = true;
			} else {
				std::stringstream errorMessage; 
				errorMessage << "Could not find the command with id '" << currentCommandID << "' during generation of the 'aggregatedCommand' object.";
				throw std::runtime_error(errorMessage.str());
			}
		}
		return std::make_shared<InputSequencesCollection>(
			commandsPointers, commandStringRepresentation, should_enable);
	};
	storeCommandObject(commandID, Command::create(data, m_environments.size(), myLambda));
}

LINKAGE_RESTRICTION void CommandsInfoContainer::storeCommandObject(CommandID const & commandID, Command const & commandToStore)
{
	m_commandsMap.emplace(commandID, m_commandsList.size());
	m_commandsList.push_back(commandToStore);
}

LINKAGE_RESTRICTION std::ostream & operator << (std::ostream & target, CommandsInfoContainer const & toDump)
{
	//TODO: add proper implementation here
	for (auto const & environment : toDump.m_environments) {
		target << environment << ',';
	}
	return target;
}

LINKAGE_RESTRICTION void AbstractSimulatedUserInput::execute()
{
	if (enabled) {
		std::cout << "executing command:" << m_value << "\n";
	} else {
		std::cout << "Processing command request on disabled (for this environment) element. Nothing happened.\n";
	}
}
LINKAGE_RESTRICTION void InputSequencesCollection::execute()
{
	if (enabled) {
		for (auto element : m_commandsToExecute) {
			element->execute();
			//TODO: there is a very subtle issue with this code.
			// We have a command-line parameter for timeout between the each keystroke, 
			// The way we do the execution of the keystrokes one after another here, this timeout will not be honored between consequent hotkey combinations.
			// This is a rare issue, but it still has to be addressed
		}
	}
}
} //namespace core
} //namespace hat

#undef LINKAGE_RESTRICTION
