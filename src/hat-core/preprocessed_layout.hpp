// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef _PREPROCESSED_LAYOUT_HPP
#define _PREPROCESSED_LAYOUT_HPP

#include "command_id.hpp"
#include "variables_manager.hpp"
#include <memory>
#include <ostream>
#include <string>
#include <vector>

//The code here represents a layout configuration, which is currently displayed on the client's screen. It is an independent from TAU library representation. The generation of layout json for TAU is done from the objects defined here.
namespace hat {
namespace core {

//TODO: rename all the 'Reresentation' to the 'Representation' (typo in current names - missing the letter 'p')
struct InternalLayoutPageRepresentation;

struct InternalLayoutElementRepresentation
{
	friend std::ostream & operator << (std::ostream & target, InternalLayoutElementRepresentation const & toDump);
private:
	size_t switchToAnotherEnvironment;
	bool shouldSwitchToAnotherEnvironment{ false }; // this is the last possible option - if the button does not reference any command, or shows the options selection page, we will use this.
	std::string m_note;
	bool m_isButton{ false }; // If this flag is set, the element will look like a button, even if it is not active
	CommandID m_referencedCommandID;
	VariableID m_referencedVariableID;
	bool m_referencesVariable{ false };

	bool optionsPagesEqual(InternalLayoutElementRepresentation const & other) const;

	// This page (if provided) is shown when the given button is pressed. When a button is pressed on the options page, it is automatically switched back to the original page.
	std::shared_ptr<InternalLayoutPageRepresentation> m_optionsPage; // TODO: make this unique_ptr (will have to add move constructors, so it can compile with unique ptr)
public:
	bool isActive() const;

	InternalLayoutElementRepresentation() = default;
	InternalLayoutElementRepresentation(std::string const & note) : m_note(note) {}

	InternalLayoutElementRepresentation & setSwitchToAnotherEnv(size_t envIndex);
	InternalLayoutElementRepresentation & referencedCommand(CommandID const & id);
	InternalLayoutElementRepresentation & setButtonFlag(bool flag);
	InternalLayoutElementRepresentation & setNote(std::string const & note);
	InternalLayoutElementRepresentation & setReferencingVariableID(VariableID const & id);
	InternalLayoutElementRepresentation & resetOptionsPage(std::shared_ptr<InternalLayoutPageRepresentation> optionsPage);
	InternalLayoutElementRepresentation & setCommandButtonAttrs(std::string const & note, CommandID const & command);
	InternalLayoutElementRepresentation & setSelectorButtonAttrs(std::string const & note, std::shared_ptr<InternalLayoutPageRepresentation> options);

	std::pair<bool, size_t> switchingToAnotherEnvironment_info() const;
	std::string getNote() const;
	CommandID getReferencedCommand() const;
	VariableID getReferencedVariable() const;
	bool referencesVariable() const { return m_referencesVariable; };
	bool is_button() const; // TODO: rename for the consistency sake
	InternalLayoutPageRepresentation const * getOptionsPagePtr() const;

	bool operator == (InternalLayoutElementRepresentation const & other) const;
};
std::ostream & operator << (std::ostream & target, InternalLayoutElementRepresentation const & toDump);

struct InternalLayoutPageRepresentation
{
	typedef std::vector<std::vector<InternalLayoutElementRepresentation>> LayoutContainer;
private:
	std::string m_caption;
	LayoutContainer m_userDefinedLayout;
public:
	InternalLayoutPageRepresentation() = default;
	InternalLayoutPageRepresentation(std::string const & note) :m_caption(note) {};
	LayoutContainer const & getLayout() const;
	std::string getNote() const;
	void pushElement(InternalLayoutElementRepresentation const & element);
	void pushRow(std::vector<InternalLayoutElementRepresentation> const & row);

	InternalLayoutElementRepresentation & getCurrentlyLastElement();

	bool hasActiveUserDefinedButtons() const;
	bool operator == (InternalLayoutPageRepresentation const & other) const;
};

//This class stores all the data needed for generation of the layout json. It is totally independent from everything else (the user does not need anything else to build the json from this)
//On the other hand, it does not have any information, which is specific to json (e.g. it does not contain string IDs for elements and pages, which will end up in the result json)
//Also it does not contain any excessive data - all the information, which should not be in the json, is not here.
//For example, if the element is selected from a set of different options, the unselected stuff is not in this object.
class InternalLayoutRepresentation
{
public:
	typedef std::vector<InternalLayoutPageRepresentation> PagesContainer;
private:
	PagesContainer m_topPagesList;
public:
	InternalLayoutRepresentation() = default;
	InternalLayoutRepresentation(std::vector<InternalLayoutPageRepresentation> const & pages) : m_topPagesList(pages) {};
	bool operator == (InternalLayoutRepresentation const & other) const;
	PagesContainer const & getPages() const;
	void push_page(InternalLayoutPageRepresentation const & page);
};

} //namespace core
} //namespace hat

#ifdef HAT_CORE_HEADERONLY_MODE
#include "preprocessed_layout.cpp"
#endif //HAT_CORE_HEADERONLY_MODE

#endif //_PREPROCESSED_LAYOUT_HPP