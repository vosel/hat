// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef HAT_CORE_HEADERONLY_MODE
#include "preprocessed_layout.hpp"
#endif

#ifndef HAT_CORE_HEADERONLY_MODE
#define LINKAGE_RESTRICTION 
#else
#define LINKAGE_RESTRICTION inline
#endif

namespace hat {
namespace core {

LINKAGE_RESTRICTION	InternalLayoutElementRepresentation & InternalLayoutElementRepresentation::referencedCommand(CommandID const & id)
{
	m_referencedCommandID = id;
	m_isButton = true;
	return *this;
}

LINKAGE_RESTRICTION	InternalLayoutElementRepresentation & InternalLayoutElementRepresentation::setButtonFlag(bool flag)
{
	m_isButton = flag;
	return *this;
}

LINKAGE_RESTRICTION InternalLayoutElementRepresentation & InternalLayoutElementRepresentation::resetOptionsPage(std::shared_ptr<InternalLayoutPageRepresentation> optionsPage)
{
	m_optionsPage = optionsPage;
	return *this;
}

LINKAGE_RESTRICTION InternalLayoutElementRepresentation & InternalLayoutElementRepresentation::setNote(std::string const & note)
{
	m_note = note;
	return *this;
}

LINKAGE_RESTRICTION InternalLayoutElementRepresentation & InternalLayoutElementRepresentation::setCommandButtonAttrs(std::string const & note, CommandID const & command)
{
	m_isButton = true;
	m_referencedCommandID = command;
	m_note = note;
	return *this;
}

LINKAGE_RESTRICTION InternalLayoutElementRepresentation & InternalLayoutElementRepresentation::setSelectorButtonAttrs(std::string const & note, std::shared_ptr<InternalLayoutPageRepresentation> options)
{
	m_isButton = true;
	m_note = note;
	m_optionsPage = options;
	return *this;
}

LINKAGE_RESTRICTION std::ostream & operator << (std::ostream & target, InternalLayoutElementRepresentation const & toDump) {
	target << (toDump.m_isButton ? "button:" : "note:") << toDump.m_note;
	if (toDump.m_referencedCommandID.nonEmpty()) {
		target << ",command:'" << toDump.m_referencedCommandID.getValue() << "'";
	}
	if (toDump.m_optionsPage) {
		target << ",optionsPage:set";
	}
	if (toDump.shouldSwitchToAnotherEnvironment) {
		target << ",switchToEnv:" << toDump.switchToAnotherEnvironment;
	}
	return target;
}

LINKAGE_RESTRICTION bool InternalLayoutElementRepresentation::operator == (InternalLayoutElementRepresentation const & other) const {
	auto DEBUG_RESULT = false; //TODO: remove this debug var
	if (m_note == other.m_note) {
		if (m_isButton && other.m_isButton) {
			if (shouldSwitchToAnotherEnvironment && other.shouldSwitchToAnotherEnvironment) {
				DEBUG_RESULT = /*return*/ switchToAnotherEnvironment == other.switchToAnotherEnvironment;
			} else if (!shouldSwitchToAnotherEnvironment && !other.shouldSwitchToAnotherEnvironment) {
				DEBUG_RESULT = /*return*/ (m_referencedCommandID == other.m_referencedCommandID) && (optionsPagesEqual(other));
			}
		} else if (!m_isButton && !other.m_isButton) {
			DEBUG_RESULT = /*return*/ true;
		}
	}
	return DEBUG_RESULT/*false*/;
}

LINKAGE_RESTRICTION std::pair<bool, size_t> InternalLayoutElementRepresentation::switchingToAnotherEnvironment_info() const
{
	return{ shouldSwitchToAnotherEnvironment, switchToAnotherEnvironment };
}

LINKAGE_RESTRICTION std::string InternalLayoutElementRepresentation::getNote() const
{
	return m_note;
}

LINKAGE_RESTRICTION CommandID InternalLayoutElementRepresentation::getReferencedCommand() const
{
	return m_referencedCommandID;
}

LINKAGE_RESTRICTION bool InternalLayoutElementRepresentation::is_button() const
{
	return m_isButton;
}

LINKAGE_RESTRICTION InternalLayoutPageRepresentation const * InternalLayoutElementRepresentation::getOptionsPagePtr() const
{
	return m_optionsPage.get();
}

LINKAGE_RESTRICTION InternalLayoutElementRepresentation & InternalLayoutElementRepresentation::setSwitchToAnotherEnv(size_t envIndex)
{
	if ((m_referencedCommandID.nonEmpty()) || (m_optionsPage)) {
		throw std::runtime_error("The button for switching environments should not reference any other command, or have additional options.");
	}
	shouldSwitchToAnotherEnvironment = true;
	switchToAnotherEnvironment = envIndex;
	return *this;
}

LINKAGE_RESTRICTION bool InternalLayoutElementRepresentation::optionsPagesEqual(InternalLayoutElementRepresentation const & other) const {

	if (m_optionsPage.get() && other.m_optionsPage.get()) {
		return *m_optionsPage == *other.m_optionsPage;
	}
	return m_optionsPage == other.m_optionsPage; // this takes care about the set vs unset shared ptr (if both are unset, true should be returned, otherwise, false)
}

LINKAGE_RESTRICTION bool InternalLayoutPageRepresentation::operator == (InternalLayoutPageRepresentation const & other) const {
	// local variables are created for debugging purposes
	auto captionsEqual = m_caption == other.m_caption;
	auto layoutsEqual = m_userDefinedLayout == other.m_userDefinedLayout;
	return captionsEqual && layoutsEqual;
}

LINKAGE_RESTRICTION bool InternalLayoutElementRepresentation::isActive() const {
	return m_isButton && ((m_referencedCommandID.nonEmpty()) || (m_optionsPage && m_optionsPage->hasActiveUserDefinedButtons()) || shouldSwitchToAnotherEnvironment);
}

LINKAGE_RESTRICTION bool InternalLayoutPageRepresentation::hasActiveUserDefinedButtons() const
{
	for (auto const & row : m_userDefinedLayout) {
		for (auto const & elem : row) {
			if (elem.isActive()) {
				return true;
			}
		}
	}
	return false;
}

LINKAGE_RESTRICTION InternalLayoutPageRepresentation::LayoutContainer const & InternalLayoutPageRepresentation::getLayout() const
{
	return m_userDefinedLayout;
}

LINKAGE_RESTRICTION std::string InternalLayoutPageRepresentation::getNote() const
{
	return m_caption;
}

LINKAGE_RESTRICTION void InternalLayoutPageRepresentation::pushElement(InternalLayoutElementRepresentation const & element)
{
	m_userDefinedLayout.back().push_back(element);
}

LINKAGE_RESTRICTION void InternalLayoutPageRepresentation::pushRow(std::vector<InternalLayoutElementRepresentation> const & row)
{
	m_userDefinedLayout.push_back(row);
}

LINKAGE_RESTRICTION InternalLayoutElementRepresentation & InternalLayoutPageRepresentation::getCurrentlyLastElement()
{
	return m_userDefinedLayout.back().back();
}

LINKAGE_RESTRICTION bool InternalLayoutRepresentation::operator == (InternalLayoutRepresentation const & other) const {
	return m_topPagesList == other.m_topPagesList;
}

LINKAGE_RESTRICTION InternalLayoutRepresentation::PagesContainer const & InternalLayoutRepresentation::getPages() const
{
	return m_topPagesList;
}

LINKAGE_RESTRICTION void InternalLayoutRepresentation::push_page(InternalLayoutPageRepresentation const & page)
{
	m_topPagesList.push_back(page);
}
} //namespace core
} //namespace hat
#undef LINKAGE_RESTRICTION