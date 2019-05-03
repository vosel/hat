// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef _IMAGE_ID_HPP
#define _IMAGE_ID_HPP

#include "string_id.hpp"
namespace hat {
namespace core {

class ImageID: public StringID<ImageID> {
public:
	ImageID() : StringID() {}
	explicit ImageID(std::string const & value) : StringID(value) {};
};
} //namespace core
} //namespace hat

#endif //_IMAGE_ID_HPP