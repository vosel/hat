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

LINKAGE_RESTRICTION Command::Command(std::string const & c_id, std::string const & c_desc, std::string const & c_gr, Command::HotkeysForDifferentEnvironments & hkeys) :
	commandID(c_id),
	commandNote(c_desc),
	commandGroup(c_gr),
	hotkeysForEnvironments(hkeys)
{}

LINKAGE_RESTRICTION bool Command::operator == (Command const & other) const
{
	return commandID == other.commandID;
}

LINKAGE_RESTRICTION Command Command::create(hat::core::ParsedCsvRow const & data, size_t customColumnsCount)
{
	using namespace std::string_literals;
	auto c_id = data.m_customColumns[0];
	auto c_gr = data.m_customColumns[1];
	auto c_desc = data.m_customColumns[2];
	auto target = HotkeysForDifferentEnvironments{};
	target.reserve(customColumnsCount);
	for (size_t i = 0; i < customColumnsCount; ++i) {
		if (i < data.m_customColumns.size() - 4) {
			target.push_back(std::make_shared<HotkeyCombination>(data.m_customColumns[i + 4]));
		} else {
			target.push_back(std::make_shared<HotkeyCombination>(""));
		}
	}
	return Command(c_id, c_desc, c_gr, target);
}

LINKAGE_RESTRICTION std::vector<std::string> splitTheRow(std::string const & row, char delimiter, std::function<bool(std::string const &, size_t, bool)> elementsVerificationFunctor)
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
}

LINKAGE_RESTRICTION std::vector<std::string> splitTheRow(std::string const & row, char delimiter)
{
	return splitTheRow(row, delimiter,
		[](std::string const & extractedString, size_t indexForString, bool moreDataInStream) -> bool {
		return true;
	});
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

LINKAGE_RESTRICTION auto ParsedCsvRow::parseHeaderString(std::string const & headerString)
{
	using namespace std::string_literals;
	static const auto leadingRequiredCharacters("command_id\tcommand_category\tcommand_note\tcommand_description\t"s);
	if (headerString.find(leadingRequiredCharacters) != 0) {
		static const auto errorMessage = "Error during parsing commands config. Header string should start with the specific elements: " + leadingRequiredCharacters;
		throw std::runtime_error(errorMessage);
	}
	const auto toParse(headerString.substr(leadingRequiredCharacters.size(), headerString.size()));

	return ParsedCsvRow(splitTheRow(toParse, '\t', [](std::string const & extractedString, size_t indexForString, bool moreDataInStream) -> bool {
		if ((extractedString.size() == 0) && moreDataInStream) {
			std::stringstream errorMessage;
			errorMessage << "Empty attribute in the header string at position " << indexForString << " - this is not allowed";
			throw std::runtime_error(errorMessage.str());
		}
		return true;
	}));
}

LINKAGE_RESTRICTION auto ParsedCsvRow::parseDataRowString(std::string const & rowString)
{
	auto rowElements = splitTheRow(rowString, '\t', [](std::string const & extractedString, size_t indexForString, bool moreDataInStream) -> bool {
		if (indexForString == 0) {
			if (extractedString.size() == 0) {
				std::stringstream errormessage;
				errormessage << "Each row should have a string id for reperenceing it.";
				throw std::runtime_error(errormessage.str());
			} else {
				auto idStingCheck = idStringOk(extractedString);
				if (!std::get<0>(idStingCheck)) {
					std::stringstream errormessage;
					errormessage << "Forbidden symbol in the id string value at position " << std::get<1>(idStingCheck) << ". The ID string: '" << extractedString << "'.";
					throw std::runtime_error(errormessage.str());
				}
				
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
	});
	return ParsedCsvRow(rowElements);
}

LINKAGE_RESTRICTION CommandsInfoContainer::CommandsContainer const & CommandsInfoContainer::getAllCommands() const
{
	return m_commandsList;
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

LINKAGE_RESTRICTION CommandsInfoContainer CommandsInfoContainer::parseConfigFile(std::istream & dataSource)
{
	auto tmpStr = std::string{};
	if (!getLineFromFile(dataSource, tmpStr)) {
		throw std::runtime_error("No data in the commands config stream.");
	}
	auto result = CommandsInfoContainer(ParsedCsvRow::parseHeaderString(clearUTF8_byteOrderMark(tmpStr)));
	auto commands = std::vector<ParsedCsvRow>{};
	size_t lineCount = 0;
	while (getLineFromFile(dataSource, tmpStr)) {
		++lineCount;
		try {
			if (tmpStr.size() > 0) { // Empty lines are just skipped here
				result.pushDataRow(ParsedCsvRow::parseDataRowString(tmpStr));
			}			
		} catch (std::runtime_error & e) {
			std::stringstream errorMessage;
			errorMessage << "Error parsing the command configuration file at row " << lineCount << ":\n\t";
			errorMessage << e.what();
			throw std::runtime_error(errorMessage.str());
		}
	}
	return result;
}

LINKAGE_RESTRICTION void CommandsInfoContainer::pushDataRow(hat::core::ParsedCsvRow const & data)
{
	std::stringstream errorMessage;
	using namespace std::string_literals;
	if (data.m_customColumns.size() > m_environments.size() + 4) {
		errorMessage << "Too much parameters for the row element. The max amount of parameters should be ('num of environments' + 4). (4 additional parameters are command id, group, button note and description)";
		throw std::runtime_error(errorMessage.str());
	}
	auto commandID = CommandID{ data.m_customColumns[0] };
	if (m_commandsMap.find(commandID) != m_commandsMap.end()) {
		errorMessage << "A duplicate command id found: '" << commandID.getValue() << "'. This is not allowed.";
		throw std::runtime_error(errorMessage.str());
	}
	m_commandsMap.emplace(commandID, m_commandsList.size());
	m_commandsList.push_back(Command::create(data, m_environments.size()));
}

LINKAGE_RESTRICTION std::ostream & operator << (std::ostream & target, CommandsInfoContainer const & toDump)
{
	//TODO: add proper implementation here
	for (auto const & environment : toDump.m_environments) {
		target << environment << ',';
	}
	return target;
}

LINKAGE_RESTRICTION void HotkeyCombination::execute()
{
	if (enabled) {
		std::cout << "executing command:" << m_value << "\n";
	} else {
		std::cout << "Processing command request on disabled (for this environment) element. Nothing happened.\n";
	}
}

} //namespace core
} //namespace hat

#undef LINKAGE_RESTRICTION
