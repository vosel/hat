// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#include "layout_parsing_verificator.hpp"
#include "../external_dependencies/Catch/single_include/catch.hpp"
namespace hat {
namespace test {

hat::core::LayoutPageTemplate & LayoutConfigParsingVerificator::getCurrentlyConstructedPage()
{
	if (m_currentPageKey.size() > 0) {
		return accumulatedConfig.getLayoutPageForOptionSelector(hat::core::CommandID{ m_currentPageKey });
	} else {
		return accumulatedConfig.getCurrentlyLastLayoutPage();
	}
}

void LayoutConfigParsingVerificator::startNewPage(std::string const & caption)
{
	accumulatedConfig.push(hat::core::LayoutPageTemplate(caption));
	m_currentPageKey = "";
	configStream << "page:" << caption << "\n";
}
void LayoutConfigParsingVerificator::startNewOptionsSelectorPage(std::string const & caption, std::string const & id)
{
	REQUIRE(id.size() > 0);
	accumulatedConfig.insert_selector_page(hat::core::CommandID{ id }, hat::core::LayoutPageTemplate(caption));
	m_currentPageKey = id;
	configStream << "optionsSelectorPage:" << id << ";" << caption << "\n";
}

void LayoutConfigParsingVerificator::addRow(std::vector<std::string> const & values)
{
	getCurrentlyConstructedPage().push_row({});
	for (auto const & value : values) {
		getCurrentlyConstructedPage().push_elem_into_last_row(hat::core::LayoutElementTemplate::create(value));
		configStream << value << ";";
	}
	configStream << "\n";
}

void LayoutConfigParsingVerificator::verifyConfigIsOK()
{
	std::string configurationToReadAndTest = configStream.str();
	std::stringstream mystream(configurationToReadAndTest);
	auto test = hat::core::LayoutUserInformation::parseConfigFile(mystream);
	REQUIRE(test == accumulatedConfig);
}

hat::core::LayoutUserInformation & LayoutConfigParsingVerificator::getAccumulatedConfig()
{
	return accumulatedConfig;
}

} // namespace test
} // namespace hat
