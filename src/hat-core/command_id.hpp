// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef _COMMAND_ID_HPP
#define _COMMAND_ID_HPP

#include "string_id.hpp"
namespace hat {
namespace core {

class CommandID: public StringID<CommandID> {
public:
	CommandID() : StringID() {}
	explicit CommandID(std::string const & value) : StringID(value) {};
};
} //namespace core
} //namespace hat

#endif //_COMMAND_ID_HPP