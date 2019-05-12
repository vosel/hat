// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef USER_DEFINED_LAYOUT_HPP
#define USER_DEFINED_LAYOUT_HPP

#include "command_id.hpp"
#include "variables_manager.hpp"
#include <ostream>
#include <string>
#include <map>
#include <vector>

//The code here is responsible for extracting the infromation from the commands.csv configuration file.

namespace hat {
namespace core {

// This is a small struct, which could represent either a button or a label (for displaying automatically updated text variable) on the layout
// Note: since there are no future plans of adding a diverse set of different controls, the simple isVariableLabel() member function is enough to handle the different situations.
// If this chanes in the future, this class should be redesigned. 
// Contract constraint: this class assumes that if isVariableLabel() returns 'true' the user will not call getComandID(), and if it returns 'false' the user will not call getVariableID(). Otherwise the behaviour is undefined.
class LayoutElementOptionToDisplay
{
	CommandID m_commandID;
	VariableID m_variableID;
	bool m_isVariableLabel;
public:
	LayoutElementOptionToDisplay(CommandID const & commandID)
		: m_commandID(commandID), m_isVariableLabel(false) {};

	LayoutElementOptionToDisplay(VariableID const & variableID)
		: m_variableID(variableID), m_isVariableLabel(true) {};

	bool isVariableLabel() const { return m_isVariableLabel; };
	VariableID getVariableID() const { return m_variableID; };
	CommandID getComandID() const { return m_commandID; };
	std::string const & getValue() const {
		return m_isVariableLabel ? m_variableID.getValue() : m_commandID.getValue();
	}
	bool operator == (LayoutElementOptionToDisplay const & other) const;
	static LayoutElementOptionToDisplay createFromUserData(std::string const & dataString);
	
	//TODO: move to the common class for string constants
	static std::string const & VARIABLE_DEF_CONFIG_PREFIX() { static std::string result{ "text:" }; return result; };
};

//If there is no options for this element, an empty space is used as a layout element
//For the list of options, the layout builder will pick up the first one, which is enabled for this environment
struct LayoutElementTemplate
{
	typedef std::vector<LayoutElementOptionToDisplay> OptionsContainer;
private:
	OptionsContainer m_optionsForElements;
public:
	LayoutElementTemplate() = default;
	LayoutElementTemplate(std::vector<std::string> const & data)
	{
		m_optionsForElements.reserve(data.size());
		for (auto const & elem : data) {
			// Silently discard the strings, which contain no non-space symbols:
			if (elem.find_first_not_of(" \t") != std::string::npos) {
				m_optionsForElements.push_back(LayoutElementOptionToDisplay::createFromUserData( elem ));
			}
		}
	}; //TODO: remove this method

	OptionsContainer const & getOptions() const;
	LayoutElementTemplate(OptionsContainer const & data) : m_optionsForElements(data) {};
	LayoutElementTemplate(std::vector<CommandID> const & data)
	{
		m_optionsForElements.reserve(data.size());
		for (auto & commandID : data) {
			m_optionsForElements.push_back(LayoutElementOptionToDisplay{commandID});
		}
	};
	static auto create(std::string const & configurationString);
	bool operator == (LayoutElementTemplate const & other) const;
};
std::ostream & operator << (std::ostream & target, LayoutElementTemplate const & toDump);

struct LayoutPageTemplate
{
	typedef std::vector<std::vector<LayoutElementTemplate> > Rows;
private:
	std::string m_name;
	Rows m_rows;
public:

	LayoutPageTemplate(std::string const & note) :m_name(note) {};
	LayoutPageTemplate() = default;

	void decode_and_store_row_description_string(std::string const & row);
	void push_row(std::vector<LayoutElementTemplate> const & rows);
	void push_elem_into_last_row(LayoutElementTemplate const & toAdd);
	Rows const & get_rows() const;

	std::string get_note() const;
	bool operator == (LayoutPageTemplate const & other) const;



	static auto create(std::string const & name);
	static bool isStartOfNewPage(std::string const & name);
	static std::string getNormalPageCaptionFromHeader(std::string const & line);
	static bool isStartOfSelectorPage(std::string const & line);

	struct SelectorPageOptions
	{
		std::string const m_caption;
		CommandID const m_id;
		SelectorPageOptions(std::string const & caption, CommandID const & id) : m_caption(caption), m_id(id) {};
	};
	static SelectorPageOptions getSelectorPageCaptionAndID(std::string const & line);
private:
	static std::string & REQUIRED_PREFIX_() { static auto result = std::string{ "page:" }; return result; }
	static std::string & REQUIRED_OPTIONS_SELECTOR_PREFIX_() { static auto result = std::string{ "optionsSelectorPage:" }; return result; }
};
std::ostream & operator << (std::ostream & target, LayoutPageTemplate const & toDump);


// This class contains all the information about the user-defined part of the layout
// There are also pages, which are generated by this tool automatically(environment selection, commands library). Those are not in this class.
struct LayoutUserInformation
{
	typedef std::map<CommandID, LayoutPageTemplate> OptionsSelctorsContainer;
private:
	std::vector<LayoutPageTemplate> m_layoutPages; // TODO: maybe will switch to std::list, so the references don't get invalidated (more stable code, but currently this feature is not necessary)
	OptionsSelctorsContainer m_optionsSelectionPages;
public:
	std::vector<LayoutPageTemplate> const & getLayoutPages() const;
	void push(LayoutPageTemplate const & toPush);
	void insert_selector_page(CommandID const & id, LayoutPageTemplate const & toInsert);

	LayoutPageTemplate & getCurrentlyLastLayoutPage();
	LayoutPageTemplate & getLayoutPageForOptionSelector(CommandID const & commandID);

	bool contains_selector(CommandID const & commandID) const;
	OptionsSelctorsContainer::const_iterator find_selector(CommandID const & commandID) const;
	OptionsSelctorsContainer::const_iterator non_existent_selector() const;

	static auto parseConfigFile(std::istream & dataToParse);
	bool operator == (LayoutUserInformation const & other) const;
};
} //namespace core
} //namespace hat

#ifdef HAT_CORE_HEADERONLY_MODE
#include "user_defined_layout.cpp"
#endif //HAT_CORE_HEADERONLY_MODE

#endif //USER_DEFINED_LAYOUT_HPP