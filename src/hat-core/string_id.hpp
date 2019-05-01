// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef STRING_ID_HPP
#define STRING_ID_HPP
#include <string>

namespace hat {
namespace core {

// This is a base class for the various string wrapper classes, used in the project.
// We are ushing crtp here in order to ensure that the semantically different strings
// don't get mixed up.
template<typename DescendantType>
class StringID
{
	std::string m_value;
protected:
	StringID() {};
	StringID(std::string const & value) : m_value(value) {};
public:
	bool operator == (DescendantType const & other) const
	{
		return m_value == other.m_value;
	}

	bool operator < (DescendantType const & other) const
	{
		return m_value < other.m_value;
	}

	std::string const & getValue() const
	{
		return m_value;
	}

	bool nonEmpty() const
	{
		return m_value.size() > 0;
	}
};

} //namespace core
} //namespace hat

#endif //STRING_ID_HPP