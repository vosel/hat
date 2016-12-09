// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef HAT_CORE_HEADERONLY_MODE
#include "user_defined_layout.hpp"
#endif

#include "commands_data_extraction.hpp"
#include "utils.hpp"
#include <algorithm>
#include <sstream>

#ifndef HAT_CORE_HEADERONLY_MODE
#define LINKAGE_RESTRICTION 
#else
#define LINKAGE_RESTRICTION inline
#endif

namespace hat {
namespace core {

LINKAGE_RESTRICTION auto LayoutElementTemplate::create(std::string const & configurationString)
{
	auto result = splitTheRow(configurationString, ',');
	auto commandIDs = std::vector<CommandID>{};
	std::for_each(begin(result), end(result), [&commandIDs](std::string & data) {
		// trim leading and trailing isspace symbols in data
		auto begin = data.find_first_not_of(" \t");
		auto end = data.find_last_not_of(" \t");
		if ((begin != std::string::npos) && (end != std::string::npos)) {
			data = data.substr(begin, end - begin + 1); //don't really need to modify the string in-place (TODO: maybe will remove it to make the code less complicated)
		}
		commandIDs.push_back(CommandID(data));
	});
	return LayoutElementTemplate(commandIDs);
}

LINKAGE_RESTRICTION LayoutElementTemplate::OptionsContainer const & LayoutElementTemplate::getOptions() const
{
	return m_optionsForElements;
}

LINKAGE_RESTRICTION bool LayoutElementTemplate::operator == (LayoutElementTemplate const & other) const {
	return m_optionsForElements == other.m_optionsForElements;
}

LINKAGE_RESTRICTION void LayoutPageTemplate::decode_and_store_row_description_string(std::string const & row)
{
	if (row.size() == 0) {
		throw std::runtime_error("The data row should contain at least one element id. This row contains none.");
	}
	auto result = splitTheRow(row, ';');
	m_rows.push_back(std::vector<LayoutElementTemplate>());

	auto & target = m_rows.back();
	target.reserve(result.size());
	for (auto const & elementTemplateString : result) {
		target.push_back(LayoutElementTemplate::create(elementTemplateString));
	}
}

LINKAGE_RESTRICTION std::string LayoutPageTemplate::get_note() const
{
	return m_name;
}

LINKAGE_RESTRICTION void LayoutPageTemplate::push_row(std::vector<hat::core::LayoutElementTemplate> const & rows)
{
	m_rows.push_back(rows);
}

LINKAGE_RESTRICTION void LayoutPageTemplate::push_elem_into_last_row(LayoutElementTemplate const & toAdd)
{
	m_rows.back().push_back(toAdd);
}

LINKAGE_RESTRICTION LayoutPageTemplate::Rows const & LayoutPageTemplate::get_rows() const
{
	return m_rows;
}

LINKAGE_RESTRICTION bool LayoutPageTemplate::operator == (LayoutPageTemplate const & other) const {
	return ((m_name == other.m_name) && (m_rows == other.m_rows));
}

LINKAGE_RESTRICTION auto LayoutPageTemplate::create(std::string const & page_header_caption)
{
	return LayoutPageTemplate(page_header_caption);
}

LINKAGE_RESTRICTION bool LayoutPageTemplate::isStartOfNewPage(std::string const & line)
{
	return (line.find(REQUIRED_PREFIX_()) == 0);
}

LINKAGE_RESTRICTION std::string LayoutPageTemplate::getNormalPageCaptionFromHeader(std::string const & line)
{
	if (!isStartOfNewPage(line)) {
		throw std::runtime_error("Trying to process the line as a layout page declaration. It is ill-formed. It should start with 'page:<name>' line.");
	}
	return line.substr(REQUIRED_PREFIX_().size(), line.size());
}

LINKAGE_RESTRICTION LayoutPageTemplate::SelectorPageOptions LayoutPageTemplate::getSelectorPageCaptionAndID(std::string const & line)
{
	if (!isStartOfSelectorPage(line)) {
		throw std::runtime_error("Trying to process the line as the start of the selector page, which is not correctly formed.");
	}

	auto dataToParse = line.substr(REQUIRED_OPTIONS_SELECTOR_PREFIX_().size(), line.size());
	auto result = splitTheRow(dataToParse, ';');
	if (result.size() != 2) {
		throw std::runtime_error("Selector page start line is ill-formed. It should be ID and caption split by ';' sign."); // TODO: rework this - make sure that the captions could have ';' in them.
	}

	return SelectorPageOptions{ result[1], CommandID{result[0]} };
}

LINKAGE_RESTRICTION bool LayoutPageTemplate::isStartOfSelectorPage(std::string const & line)
{
	return (line.find(REQUIRED_OPTIONS_SELECTOR_PREFIX_()) == 0);
}

LINKAGE_RESTRICTION	std::ostream & operator << (std::ostream & target, LayoutElementTemplate const & toDump)
{
	for (auto const & option : toDump.getOptions()) {
		target << option.getValue() << '|';
	}
	return target;
}

LINKAGE_RESTRICTION	std::ostream & operator << (std::ostream & target, LayoutPageTemplate const & toDump)
{
	target << "hat::core::LayoutPageTemplate : pageName=" << toDump.get_note();
	for (auto const & page_row_array : toDump.get_rows()) {
		target << "\n\t";
		for (auto const & layoutElementEntry : page_row_array) {
			target << layoutElementEntry << " ;\t";
		}
	}
	return target;
}

LINKAGE_RESTRICTION std::vector<LayoutPageTemplate> const & LayoutUserInformation::getLayoutPages() const
{
	return m_layoutPages;
}

LINKAGE_RESTRICTION void LayoutUserInformation::push(LayoutPageTemplate const & toPush)
{
	m_layoutPages.push_back(toPush);
}

LINKAGE_RESTRICTION LayoutPageTemplate & LayoutUserInformation::getCurrentlyLastLayoutPage()
{
	return m_layoutPages.back();
}

LINKAGE_RESTRICTION LayoutPageTemplate & LayoutUserInformation::getLayoutPageForOptionSelector(CommandID const & commandID)
{
	return m_optionsSelectionPages.at(commandID);
}

LINKAGE_RESTRICTION bool LayoutUserInformation::contains_selector(CommandID const & commandID) const
{
	return m_optionsSelectionPages.find(commandID) != m_optionsSelectionPages.end();
}

LINKAGE_RESTRICTION LayoutUserInformation::OptionsSelctorsContainer::const_iterator LayoutUserInformation::find_selector(CommandID const & commandID) const
{
	return m_optionsSelectionPages.find(commandID);
}

LINKAGE_RESTRICTION LayoutUserInformation::OptionsSelctorsContainer::const_iterator LayoutUserInformation::non_existent_selector() const
{
	return m_optionsSelectionPages.end();
}

LINKAGE_RESTRICTION void LayoutUserInformation::insert_selector_page(CommandID const & id, LayoutPageTemplate const & toInsert)
{
	m_optionsSelectionPages[id] = toInsert;
}

LINKAGE_RESTRICTION auto LayoutUserInformation::parseConfigFile(std::istream & dataToParse)
{
	LayoutUserInformation result;
	std::string tmpString;
	LayoutPageTemplate * currentlyConstructedPage = nullptr;
	while (getLineFromFile(dataToParse, tmpString)) {
		if (LayoutPageTemplate::isStartOfNewPage(tmpString)) {
			result.m_layoutPages.push_back(LayoutPageTemplate::create(LayoutPageTemplate::getNormalPageCaptionFromHeader(tmpString)));
			currentlyConstructedPage = &(result.m_layoutPages.back());
		} else if (LayoutPageTemplate::isStartOfSelectorPage(tmpString)) {
			auto selectorPageOptions = LayoutPageTemplate::getSelectorPageCaptionAndID(tmpString);
			if (result.contains_selector(selectorPageOptions.m_id)) {
				std::stringstream errorMessage;
				errorMessage << "Duplicate id for the selector page: " << selectorPageOptions.m_id.getValue(); //TOOD: add test for this error
				throw std::runtime_error(errorMessage.str().c_str());
			}
			result.m_optionsSelectionPages[selectorPageOptions.m_id] = LayoutPageTemplate::create(selectorPageOptions.m_caption);
			currentlyConstructedPage = &(result.m_optionsSelectionPages.at(selectorPageOptions.m_id));
		} else {
			if (nullptr == currentlyConstructedPage) {
				throw std::runtime_error("A layout configuration file should start from the proper page prefix."); //TOOD: add the unit test reference for this error (it is currently tested), make the testing verification better.
			}
			currentlyConstructedPage->decode_and_store_row_description_string(tmpString);
		}
	}
	return result;
}

LINKAGE_RESTRICTION bool LayoutUserInformation::operator == (LayoutUserInformation const & other) const
{
	return ((other.m_layoutPages == m_layoutPages) && (m_optionsSelectionPages == other.m_optionsSelectionPages));
}
} //namespace core
} //namespace hat

#undef LINKAGE_RESTRICTION