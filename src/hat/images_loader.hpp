// This source file is part of the 'hat' open source project.
// Copyright (c) 2019, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.
#ifndef HAT_IMAGES_LOADER_HPP
#define HAT_IMAGES_LOADER_HPP

#include <tau/common/element_id.h>
#include "../hat-core/image_id.hpp"
#include "../hat-core/commands_data_extraction.hpp"
#include <string>
#include <tau/common/ARGB_image_resource.h>
#include <functional>

#ifdef HAT_IMAGES_SUPPORT
namespace hat {
namespace tool {

typedef std::vector<std::pair<hat::core::ImageID, hat::core::ImagePhysicalInfo>> ImageFilesRegionsList;
typedef std::vector<std::pair<tau::common::ImageID, tau::common::ARGB_ImageResource>> ImageBuffersList;

ImageBuffersList loadImages(ImageFilesRegionsList const & data, std::function<void(std::string const &)> loadingLogger);
} // namespace tool
} // namespace hat

#endif //HAT_IMAGES_SUPPORT
#endif //HAT_IMAGES_LOADER_HPP