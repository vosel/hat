// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef _COMMAND_ID_HPP
#define _COMMAND_ID_HPP

#include <string>
namespace hat {
namespace core {

class CommandID // TODO: rename to something like 'ButtonActionID' (since it is not only used for commands, but also for other stuff like selector IDs)
{
	std::string m_value;
public:
	CommandID() {};
	explicit CommandID(std::string const & value) : m_value(value) {};
	std::string const & getValue() const;
	bool operator == (CommandID const & other) const;
	bool operator < (CommandID const & other) const;
	bool nonEmpty() const;
};

} //namespace core
} //namespace hat

#ifdef HAT_CORE_HEADERONLY_MODE
#include "command_id.cpp"
#endif //HAT_CORE_HEADERONLY_MODE

#endif //_COMMAND_ID_HPP