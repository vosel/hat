#include "images_loader.hpp"
#include <tau/common/ARGB_image_resource.h>

#include <iostream>
#include <algorithm>

#include <boost/gil/extension/io/png_io.hpp>

#ifdef HAT_IMAGES_SUPPORT

namespace hat {
namespace tool {

template <typename BoostGilImageView>
tau::common::ARGB_ImageResource simpleLoadImage(BoostGilImageView const & imageView, hat::core::ImagePhysicalInfo const & toLoad) {
	auto const img_width = (size_t)imageView.width();
	auto const img_height = (size_t)imageView.height();
	//calculation of the actual crop region
	auto const crop_x = toLoad.origin.x;
	auto const crop_y = toLoad.origin.y;
	if ((crop_x >= img_width) || (crop_y >= img_height)) {
		std::stringstream error;
		error << "The crop origin (x=" << crop_x << ", y=" << crop_y << ") is located outside of the image (width="
			<< img_width << ", height="
			<< img_height << ") - this is not allowed.";
		throw std::runtime_error(error.str());
	}
	// Note: the weird form of the condition variable is done this way on purpose.
	// the toLoad.size members have default value of SIZE_MAX, so we can't use the more natural form:
	//    (crop_x + toLoad.size.x) > img_width - it will overflow, which will cause a mess.
	auto const crop_width  = ((img_width  - crop_x) > toLoad.size.x) ? toLoad.size.x :  (img_width - crop_x);
	auto const crop_height = ((img_height - crop_y) > toLoad.size.y) ? toLoad.size.y : (img_height - crop_y);

	//crop the image appropriately, copy the data to the result:
	tau::common::ARGB_ImageResource result{crop_width, crop_height};

	//TODO: implement this copying properly (more ideomatic for boost::gil)
	using boost::gil::view;
	using boost::gil::subimage_view;

	// Copy the data pixel by pixel:
	for (size_t x = 0; x < crop_width; ++x) {
		for (size_t y = 0; y < crop_height; ++y) {
			auto point = imageView(x + crop_x, y + crop_y);
			auto red = boost::gil::get_color(point, boost::gil::red_t());
			auto green = boost::gil::get_color(point, boost::gil::green_t());
			auto blue = boost::gil::get_color(point, boost::gil::blue_t());

			result.at(x, y) = tau::common::ARGB_point{255, red, green, blue};
		}
	}
	return result;
}

//All the objects in the input vector should point to the regions in the same file
std::vector<tau::common::ARGB_ImageResource> loadImagesFromSameFile(std::string const & file_path,
					std::vector<hat::core::ImagePhysicalInfo> const & toLoad) {
	auto result = std::vector<tau::common::ARGB_ImageResource> {};
	result.reserve(toLoad.size());
	if (toLoad.size() > 0) {
		auto imgRGBA = boost::gil::rgb8_image_t{};
		try {
			boost::gil::png_read_image(file_path, imgRGBA);
		} catch (std::ios_base::failure & exception) {
			std::cout << exception.what() << "\n";
			throw std::runtime_error("Could not read one of the images.");
		}

		for (auto & single_crop: toLoad) {
			if (single_crop.filepath == file_path) {
				result.push_back(simpleLoadImage(boost::gil::view(imgRGBA), single_crop));
			}
		}
	}
	return result;
}

ImageBuffersList loadImages(
	ImageFilesRegionsList const & data, std::function<void(std::string const &)> loadingLogger)
{
	std::vector<std::pair<tau::common::ImageID, tau::common::ARGB_ImageResource>> result;
	result.reserve(data.size());

	typedef std::vector<hat::core::ImageID> CacheOfImgIDs;
	typedef std::vector<hat::core::ImagePhysicalInfo> CacheOfCropInfos;
	std::map<std::string, std::pair<CacheOfImgIDs, CacheOfCropInfos>> sorted_data{};
	for (auto & entry : data) {
		sorted_data[entry.second.filepath].first.push_back(entry.first);
		sorted_data[entry.second.filepath].second.push_back(entry.second);
	}

	for (auto & allImagesForSameFile: sorted_data) {
		std::stringstream message;
		message << "Reading image file [" << allImagesForSameFile.second.second.size() << " regions should be extracted]:" << allImagesForSameFile.first;
		loadingLogger(message.str());
		auto load_result = loadImagesFromSameFile(allImagesForSameFile.first, allImagesForSameFile.second.second);
		auto & imageIDs = allImagesForSameFile.second.first;

		// Package the results into output vector:
		std::transform(imageIDs.begin(), imageIDs.end(), load_result.begin(), std::back_inserter(result),
			[](hat::core::ImageID const & imgID, tau::common::ARGB_ImageResource const & image) {
				//TODO: we should store the image resources inside shared_ptr, so that we don't have to excessively copy it around.
				return std::pair<tau::common::ImageID, tau::common::ARGB_ImageResource> {
					tau::common::ImageID{imgID.getValue()}, image
				};
			});
	}
	return result;
}

} // namespace tool
} // namespace hat

#endif //HAT_IMAGES_SUPPORT