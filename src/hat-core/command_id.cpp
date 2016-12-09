// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef HAT_CORE_HEADERONLY_MODE
#include "command_id.hpp"
#endif

#ifndef HAT_CORE_HEADERONLY_MODE
#define LINKAGE_RESTRICTION 
#else
#define LINKAGE_RESTRICTION inline
#endif

namespace hat {
namespace core {

LINKAGE_RESTRICTION bool CommandID::operator == (CommandID const & other) const
{
	return m_value == other.m_value;
}

LINKAGE_RESTRICTION bool CommandID::operator < (CommandID const & other) const
{
	return m_value < other.m_value;
}

LINKAGE_RESTRICTION std::string CommandID::getValue() const
{
	return m_value;
}

LINKAGE_RESTRICTION bool CommandID::nonEmpty() const
{
	return m_value.size() > 0;
}

} //namespace core
} //namespace hat

#undef LINKAGE_RESTRICTION