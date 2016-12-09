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

LINKAGE_RESTRICTION bool idStringOk(std::string const & toTest) {
	auto index = toTest.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_1234567890");
	if (index != std::string::npos) {
		std::cerr << "Error: forbidden symbol found in id string : '" << toTest << "' at position " << index << " (symbol: '" << toTest[index] << "').\n";
		return false;
	}
	return true;
}

LINKAGE_RESTRICTION auto ParsedCsvRow::parseHeaderString(std::string const & headerString)
{
	using namespace std::string_literals;
	static const auto leadingRequiredCharacters("hotkey_id\thotkey_category\thotkey_note\thotkey_description\t"s);
	if (headerString.find(leadingRequiredCharacters) != 0) {
		throw std::runtime_error("header string should start with the specific elements.");
	}
	const auto toParse(headerString.substr(leadingRequiredCharacters.size(), headerString.size()));

	return ParsedCsvRow(splitTheRow(toParse, '\t', [](std::string const & extractedString, size_t indexForString, bool moreDataInStream) -> bool {
		if ((extractedString.size() == 0) && moreDataInStream) {
			throw std::runtime_error("empty attribute in the header string - this is not allowed");
		}
		return true;
	}));
}

LINKAGE_RESTRICTION auto ParsedCsvRow::parseDataRowString(std::string const & rowString)
{
	auto rowElements = splitTheRow(rowString, '\t', [](std::string const & extractedString, size_t indexForString, bool moreDataInStream) -> bool {
		if (indexForString == 0) {
			if (extractedString.size() == 0) {
				throw std::runtime_error("Each row should have a string id for reperenceing it.");
			} else if (!idStringOk(extractedString)) {
				throw std::runtime_error("Forbidden symbol in the id string value.");
			}
		} else if (indexForString == 1) {
			if (!idStringOk(extractedString)) {
				throw std::runtime_error("Forbidden symbol in the category string value.");
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
	auto result = CommandsInfoContainer(ParsedCsvRow::parseHeaderString(tmpStr));
	auto commands = std::vector<ParsedCsvRow>{};
	while (getLineFromFile(dataSource, tmpStr)) {
		result.pushDataRow(ParsedCsvRow::parseDataRowString(tmpStr));
	}
	return result;
}

LINKAGE_RESTRICTION void CommandsInfoContainer::pushDataRow(hat::core::ParsedCsvRow const & data)
{
	using namespace std::string_literals;
	if (data.m_customColumns.size() > m_environments.size() + 4) {
		throw std::runtime_error("Too much parameters for the row element. The max amount of parameters should be ('num of environments' + 4). (4 additional parameters are command id, group, button note and description)");
	}
	auto commandID = CommandID{ data.m_customColumns[0] };
	if (m_commandsMap.find(commandID) != m_commandsMap.end()) {
		throw std::runtime_error("A duplicate command id found:"s + commandID.getValue());
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
