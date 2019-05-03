#include "../hat-core/command_id.hpp"
#include "../hat-core/image_id.hpp"
#include "../hat-core/commands_data_extraction.hpp"

#include "../external_dependencies/Catch/single_include/catch.hpp"
#include <sstream>

using hat::core::CommandID;
using hat::core::ImageID;
std::string const ENV0{ "ENV0" };
std::string const ENV1{ "ENV1" };
const auto ENVIRONMENTS = hat::core::CommandsInfoContainer::EnvsContainer{ENV0, ENV1};
std::string const UNKNOWN_ENV{ "UNKNOWN_ENV" };
CommandID const COMMAND_0 { "COMMAND_0" };
CommandID const COMMAND_1 { "COMMAND_1" };
CommandID const COMMAND_2 { "COMMAND_2" };
CommandID const COMMAND_3 { "COMMAND_3" };
CommandID const UNUSED_COMMAND { "COMMAND_NOT_USED_ON_PURPOSE" };
ImageID const IMAGE_0 { "IMAGE_0" };
ImageID const IMAGE_1 { "IMAGE_1" };


std::string const FILE_PATH0{ "testfilename0.png" };
std::string const FILE_PATH1{ "testfilename1.png" };
auto const DEFAULT_ORIG_POS  = hat::core::XY_Dimensions{0, 0};
auto const DEFAULT_CROP_SIZE = hat::core::XY_Dimensions{SIZE_MAX, SIZE_MAX};

namespace {
	std::string buildCSVLine(std::vector<std::string> const & data)
	{
		std::stringstream result;
		bool firstIteration = true;
		for (auto iter = data.cbegin(); iter != data.cend(); ++iter, firstIteration = false) {
			if (!firstIteration) {
				result << "\t";
			}
			result << *iter;
		}
		return result.str();
	}

	auto processConfigsLines(
		std::vector<std::string> const & img_id2_physical_properties_config_lines,
		std::vector<std::string> const & command_id2_img_id_config_lines
	)
	{
		auto result = hat::core::ImageResourcesInfosContainer{ENVIRONMENTS};
		std::stringstream stream_img_id2_physical_properties_config_lines;
		for (auto & lineToProcess : img_id2_physical_properties_config_lines) {
			//TODO: ensure that this function works as expected (need to have tests with several lines passed in here)
			stream_img_id2_physical_properties_config_lines << lineToProcess << "\n";
		}
		std::stringstream stream_command_id2_img_id_config_lines;
		for (auto & lineToProcess : command_id2_img_id_config_lines) {
			//TODO: ensure that this function works as expected (need to have tests with several lines passed in here)
			stream_command_id2_img_id_config_lines << lineToProcess << "\n";
		}

		result.consumeImageResourcesConfig(
			stream_img_id2_physical_properties_config_lines,
			stream_command_id2_img_id_config_lines);
		return result;
	}
}

// A set of test cases, which cover situations, that occur when parsing only the imgID->imgPhysicalInfo config (no externalities needed)
TEST_CASE("Simple ImageID to ImagePhysicalInfo config tests")
{
	std::string const STD_POSITION{ "12,32" };
	auto const REFERENCE_ORIG_POS  = hat::core::XY_Dimensions{12, 32};

	std::string const STD_OFFSET  { "23,43" };
	auto const REFERENCE_CROP_SIZE = hat::core::XY_Dimensions{23, 43};

	auto const BASIC_CONFIG_STRING             = buildCSVLine({ IMAGE_0.getValue(), FILE_PATH0, STD_POSITION, STD_OFFSET});
	auto const NO_OPTIONAL_SIZE_PROVIDED       = buildCSVLine({ IMAGE_0.getValue(), FILE_PATH0, STD_POSITION});
	auto const NO_OPTIONAL_PARAMETERS_PROVIDED = buildCSVLine({ IMAGE_0.getValue(), FILE_PATH0});
	WHEN("Standard config string is provided, which references IMAGE_0") {
		auto imgResourcesContainer = processConfigsLines({BASIC_CONFIG_STRING}, {});
		THEN("The expected values are extracted and specified only for IMAGE_0") {
			auto imageInfo = imgResourcesContainer.getImageInfo(IMAGE_0); // Here we should not get an exception.
			REQUIRE(imageInfo.filepath == FILE_PATH0);
			REQUIRE(imageInfo.origin   == REFERENCE_ORIG_POS);
			REQUIRE(imageInfo.size     == REFERENCE_CROP_SIZE);

			//No image details should be provided for another imageID: (IMAGE_1):
			REQUIRE_THROWS(imgResourcesContainer.getImageInfo(IMAGE_1));
		}
	}

	WHEN("No optional sizes are provided, but coordinates are") {
		auto imgResourcesContainer = processConfigsLines({NO_OPTIONAL_SIZE_PROVIDED}, {});
		THEN("The default values are substituted for the sizes, but not coordinates") {
			auto imageInfo = imgResourcesContainer.getImageInfo(IMAGE_0);
			REQUIRE(imageInfo.filepath == FILE_PATH0); // Don't really need this check (the filename is checked in the first test), but let it be here anyway
			REQUIRE(imageInfo.origin   == REFERENCE_ORIG_POS);
			REQUIRE(imageInfo.size     == DEFAULT_CROP_SIZE);
		}
	}

	WHEN("No optional coordinates are provided") {
		auto imgResourcesContainer = processConfigsLines({NO_OPTIONAL_PARAMETERS_PROVIDED}, {});
		THEN("The default values are substituted for the coordinates") {
			auto imageInfo = imgResourcesContainer.getImageInfo(IMAGE_0);
			REQUIRE(imageInfo.filepath == FILE_PATH0); // Don't really need this check (the filename is checked in the first test), but let it be here anyway
			REQUIRE(imageInfo.origin   == DEFAULT_ORIG_POS);
			REQUIRE(imageInfo.size     == DEFAULT_CROP_SIZE);
		}
	}

	//Tests for erroneous situations (exceptions are usually thrown there):
	auto const TOO_MANY_ITEMS        = buildCSVLine({ IMAGE_0.getValue(), FILE_PATH0, STD_POSITION, STD_OFFSET, "" });
	auto const NOT_ENOUGH_PARAMETERS = buildCSVLine({ IMAGE_0.getValue(),                                          });
	auto const EMPTY_IMAGE_ID        = buildCSVLine({          "",        FILE_PATH0, STD_POSITION, STD_OFFSET     });
	auto const EMPTY_FILENAME        = buildCSVLine({ IMAGE_0.getValue(),     "",     STD_POSITION, STD_OFFSET     });
	
	using Catch::Matchers::Contains;
	WHEN("Too many parameters are provided in the line, exception is thrown") {
		REQUIRE_THROWS_WITH(processConfigsLines({TOO_MANY_ITEMS}, {}), Contains("Too many parameters are provided in the configuration file line"));
	}
	WHEN("Too small amount of parameters is provided, exception is thrown") {
		REQUIRE_THROWS_WITH(processConfigsLines({NOT_ENOUGH_PARAMETERS}, {}), Contains("Not enough parameters provided in the configuration file line"));
	}
	WHEN("Image id is empty string, exception is thrown") {
		REQUIRE_THROWS_WITH(processConfigsLines({EMPTY_IMAGE_ID}, {}), Contains("ImageID is an empty string in the configuration file line"));
	}
	WHEN("Filename is an empty string, exception is thrown") {
		REQUIRE_THROWS_WITH(processConfigsLines({EMPTY_FILENAME}, {}), Contains("Filename is an empty string in the configuration file line. This is not allowed. Please provide a valid image file name."));
	}

	auto const DUPLICATED_IMAGE_ID_line0   = buildCSVLine({ IMAGE_0.getValue(), FILE_PATH0});
	auto const DUPLICATED_IMAGE_ID_line1   = buildCSVLine({ IMAGE_0.getValue(), FILE_PATH1});
	WHEN("Two lines with the same image id are provided, exception is thrown") {
		REQUIRE_THROWS_WITH(
			processConfigsLines({ DUPLICATED_IMAGE_ID_line0, DUPLICATED_IMAGE_ID_line1 }, {}),
			Contains("Duplicated image id detected in the config file. This is not allowed. Image id: " + IMAGE_0.getValue()));
	}
}

TEST_CASE("Simple CommandID->ImageID parsing test") {
	//This is a config line for the imageID->physicalImageInfo file (which is not under test in this suite)
	auto const BASIC_CONFIG_STRING_FOR_IMAGE_ID_to_PHYSICAL_IMAGE_CFG_FILE  = buildCSVLine({ IMAGE_0.getValue(), FILE_PATH0});

	auto const BASIC_CONFIG_STRING      = buildCSVLine({ COMMAND_0.getValue(),      ENV0,   IMAGE_0.getValue()});

	WHEN("Basic situation - one imageID, one command. Command is COMMAND_0 and env is ENV0. Checking that everything works in that case.") {
		auto imgResourcesContainer = processConfigsLines({BASIC_CONFIG_STRING_FOR_IMAGE_ID_to_PHYSICAL_IMAGE_CFG_FILE},
										{BASIC_CONFIG_STRING});
		THEN("The expected values are extracted and specified only for COMMAND_0 in the ENV0, nowhere else") {
			auto imagesInfo = imgResourcesContainer.getImagesInfo(ENV0);
			auto imageInfo = imagesInfo.at(COMMAND_0); // Here we should not get an exception.
			REQUIRE(imageInfo == IMAGE_0);

			//No images should be defined for another command: (COMMAND_1):
			REQUIRE(imagesInfo.find(COMMAND_1) == imagesInfo.end());

			//No images should be specified for the another env (ENV1):
			REQUIRE_FALSE(imgResourcesContainer.getImagesInfo(ENV1).size() > 0);
		}
	}

	//Tests for erroneous situations (exceptions are usually thrown there):
	auto const TOO_MANY_ITEMS        = buildCSVLine({ COMMAND_0.getValue(),      ENV0       , IMAGE_0.getValue(), "" });
	auto const NOT_ENOUGH_PARAMETERS = buildCSVLine({ COMMAND_0.getValue(),      ENV0 });
	auto const EMPTY_COMMAND_ID      = buildCSVLine({          "",               ENV0       , IMAGE_0.getValue() });
	auto const EMPTY_IMAGE_ID        = buildCSVLine({ COMMAND_0.getValue(),      ENV0       ,    ""   });
	auto const UNKNOWN_ENV_STRING    = buildCSVLine({ COMMAND_0.getValue(),      UNKNOWN_ENV, IMAGE_0.getValue() });
	

	using Catch::Matchers::Contains; // String matcher for the exceptions messages
	WHEN("Too many parameters are provided in the line, exception is thrown") {
		REQUIRE_THROWS_WITH(processConfigsLines({BASIC_CONFIG_STRING_FOR_IMAGE_ID_to_PHYSICAL_IMAGE_CFG_FILE},
						{TOO_MANY_ITEMS}), Contains("Too many parameters are provided in the configuration file line"));
	}
	WHEN("Too small amount of parameters is provided, exception is thrown") {
		REQUIRE_THROWS_WITH(processConfigsLines({BASIC_CONFIG_STRING_FOR_IMAGE_ID_to_PHYSICAL_IMAGE_CFG_FILE},
						{NOT_ENOUGH_PARAMETERS}), Contains("Not enough parameters provided in the configuration file line"));
	}
	WHEN("Command id is empty string, exception is thrown") {
		REQUIRE_THROWS_WITH(processConfigsLines({BASIC_CONFIG_STRING_FOR_IMAGE_ID_to_PHYSICAL_IMAGE_CFG_FILE},
						{EMPTY_COMMAND_ID}), Contains("CommandID is an empty string in the configuration file line"));
	}
	WHEN("Filename is an empty string, exception is thrown") {
		int debug = 0; ++debug;
		REQUIRE_THROWS_WITH(processConfigsLines({BASIC_CONFIG_STRING_FOR_IMAGE_ID_to_PHYSICAL_IMAGE_CFG_FILE},
						{EMPTY_IMAGE_ID}), Contains("ImageID is an empty string in the configuration file line. This is not allowed. Please provide a valid imageID."));
	}
	WHEN("Unknown environment is provided, exception is thrown") {
		REQUIRE_THROWS_WITH(processConfigsLines({BASIC_CONFIG_STRING_FOR_IMAGE_ID_to_PHYSICAL_IMAGE_CFG_FILE},
						{UNKNOWN_ENV_STRING}), Contains("Unknown environment id"));
	}
}
