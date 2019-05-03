#include "images_loader.hpp"
#include <tau/common/ARGB_image_resource.h>

#include <iostream>

#include <boost/gil/extension/io/png_io.hpp>

#ifdef HAT_IMAGES_SUPPORT

namespace hat {
namespace tool {

template <typename BoostGilImageView>
tau::common::ARGB_ImageResource simpleLoadImage(BoostGilImageView const & imageView, hat::core::ImagePhysicalInfo const & toLoad) {
	auto const img_width = imageView.width();
	auto const img_height = imageView.height();
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

tau::common::ARGB_ImageResource simpleLoadImage(hat::core::ImagePhysicalInfo const & toLoad) {
	auto imgRGBA = boost::gil::rgb8_image_t{};
	try {
		boost::gil::png_read_image(toLoad.filepath, imgRGBA);
	} catch (std::ios_base::failure & exception) {
		std::cout << exception.what() << "\n";
		throw std::runtime_error("Could not read one of the images.");
	}
	return simpleLoadImage(boost::gil::view(imgRGBA), toLoad);
}


std::vector<std::pair<tau::common::ImageID, tau::common::ARGB_ImageResource>> loadImages(
	std::vector<std::pair<hat::core::ImageID, hat::core::ImagePhysicalInfo>> const & data)
{
	std::vector<std::pair<tau::common::ImageID, tau::common::ARGB_ImageResource>> result;
	result.reserve(data.size());

	//TODO: implement this better - currently we re-read the same image files.
	// note: we should populate something like this and work with it: std::map<std::string, std::vector<std::pair<hat::core::ImageID, hat::core::ImagePhysicalInfo>>> sorted_data;
	// the keys should be filenames, so we will be able to process each filename only once, no matter how many crop rects are specified for it.
	for (auto & element : data) {
		result.push_back(
			std::pair<tau::common::ImageID, tau::common::ARGB_ImageResource>(
				tau::common::ImageID(element.first.getValue()), simpleLoadImage(element.second)));
	}
	return result;
}

} // namespace tool
} // namespace hat

#endif //HAT_IMAGES_SUPPORT