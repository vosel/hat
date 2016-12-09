// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef	LAYOUT_PARSING_VERIFICATOR
#define LAYOUT_PARSING_VERIFICATOR

#include "../hat-core/user_defined_layout.hpp"
#include <string>
#include <sstream>

namespace hat {
namespace test {
class LayoutConfigParsingVerificator
{
	std::stringstream configStream;
	hat::core::LayoutUserInformation accumulatedConfig;
	std::string m_currentPageKey;
private:
	hat::core::LayoutPageTemplate & getCurrentlyConstructedPage();
public:
	void startNewPage(std::string const & caption);
	void startNewOptionsSelectorPage(std::string const & caption, std::string const & id);

	void addRow(std::vector<std::string> const & values);
	void verifyConfigIsOK();
	std::string getAccumulatedConfigText() const { return configStream.str(); };
	hat::core::LayoutUserInformation & getAccumulatedConfig();
};

} // namespace test
} // namespace hat
#endif // LAYOUT_PARSING_VERIFICATOR
