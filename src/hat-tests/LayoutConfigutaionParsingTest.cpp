// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#include "layout_parsing_verificator.hpp"
#include "../hat-core/user_defined_layout.hpp"
#include <sstream>
#include "../external_dependencies/Catch/single_include/catch.hpp"

using namespace std::string_literals;

namespace Catch {
template<> std::string toString(hat::core::LayoutElementTemplate const & toDump)
{
	std::stringstream result;
	result << '[' << toDump.getOptions().size() << "]:{";
	for (auto const & option : toDump.getOptions()) {
		result << option.getValue() << ", ";
	}
	result << '}';
	return result.str();
}
}

TEST_CASE("layout element template parsing", "[layout_config]")
{
	auto emptyTemplate(hat::core::LayoutElementTemplate::create(""s));
	REQUIRE(emptyTemplate == hat::core::LayoutElementTemplate{});

	auto oneElementTemplateString(hat::core::LayoutElementTemplate::create("element0"));
	REQUIRE(oneElementTemplateString == hat::core::LayoutElementTemplate({ "element0" }));

	auto severalElementsTemplatesString1(hat::core::LayoutElementTemplate::create("element0,element1"));
	REQUIRE(severalElementsTemplatesString1 == hat::core::LayoutElementTemplate({ "element0"s, "element1"s }));

	auto severalElementsTemplatesString2(hat::core::LayoutElementTemplate::create("\telement0 , element1\t"));
	REQUIRE(severalElementsTemplatesString1 == severalElementsTemplatesString2);
}

TEST_CASE("layout page template parsing", "[layout_config]")
{
	//REQUIRE_THROWS(hat::core::LayoutPageTemplate::create("string without proper page prefix"s));

	auto testPageTemplate(hat::core::LayoutPageTemplate::create("test page"s));
	auto PAGE_NAME = std::string{ "test page"s };
	REQUIRE(testPageTemplate.get_note() == PAGE_NAME);

	REQUIRE_THROWS(testPageTemplate.decode_and_store_row_description_string(""s));

	testPageTemplate.decode_and_store_row_description_string("cell00,cell01;cell1; cell2 ;cell3\t"s);
	REQUIRE(testPageTemplate.get_rows().size() == 1);
	REQUIRE(testPageTemplate.get_rows()[0].size() == 4);

	testPageTemplate.decode_and_store_row_description_string("cell_1"s);
	testPageTemplate.decode_and_store_row_description_string("cell_1;cell_2,cell_2_1"s);
	testPageTemplate.decode_and_store_row_description_string("cell_1;cell_2"s);

	hat::core::LayoutPageTemplate testReference(PAGE_NAME);
	auto row0 = std::vector<hat::core::LayoutElementTemplate>{
		hat::core::LayoutElementTemplate::create("cell00,cell01"s),
		hat::core::LayoutElementTemplate::create("cell1"s),
		hat::core::LayoutElementTemplate::create("cell2"s),
		hat::core::LayoutElementTemplate::create("cell3"s) };

	auto row1 = std::vector<hat::core::LayoutElementTemplate>{ hat::core::LayoutElementTemplate::create("cell_1"s) };
	auto row2 = std::vector<hat::core::LayoutElementTemplate>{
		hat::core::LayoutElementTemplate::create("cell_1"s),
		hat::core::LayoutElementTemplate::create("cell_2,cell_2_1"s) };
	auto row3 = std::vector<hat::core::LayoutElementTemplate>{
		hat::core::LayoutElementTemplate::create("cell_1"s),
		hat::core::LayoutElementTemplate::create("cell_2"s) };

	testReference.push_row(row0);
	testReference.push_row(row1);
	testReference.push_row(row2);
	testReference.push_row(row3);
	REQUIRE(testReference == testPageTemplate); //TODO: this test on failure dumps weird output. Need to debug this when visual studio is installed
}

TEST_CASE("simple layout config file parsing", "[layout_config]")
{
	std::stringstream mystream("some data that does not\nstart from expected prefix"s);
	REQUIRE_THROWS(hat::core::LayoutUserInformation::parseConfigFile(mystream));

	hat::core::LayoutPageTemplate testReferencePage1("page1");
	auto row0 = std::vector<hat::core::LayoutElementTemplate>{
		hat::core::LayoutElementTemplate::create("cell00,cell01"s),
		hat::core::LayoutElementTemplate::create("cell1"s),
		hat::core::LayoutElementTemplate::create("cell2"s),
		hat::core::LayoutElementTemplate::create("cell3"s) };

	auto row1 = std::vector<hat::core::LayoutElementTemplate>{ hat::core::LayoutElementTemplate::create("cell_1"s) };
	auto row2 = std::vector<hat::core::LayoutElementTemplate>{
		hat::core::LayoutElementTemplate::create("cell_1"s),
		hat::core::LayoutElementTemplate::create("cell_2,cell_2_1"s) };
	auto row3 = std::vector<hat::core::LayoutElementTemplate>{
		hat::core::LayoutElementTemplate::create("cell_1"s),
		hat::core::LayoutElementTemplate::create("cell_2"s) };

	testReferencePage1.push_row(row0);
	testReferencePage1.push_row(row1);
	testReferencePage1.push_row(row2);
	testReferencePage1.push_row(row3);

	hat::core::LayoutPageTemplate testReferencePage2("page2");
	auto row2_0 = std::vector<hat::core::LayoutElementTemplate>{
		hat::core::LayoutElementTemplate::create("qwe"s),
		hat::core::LayoutElementTemplate::create("rty"s),
		hat::core::LayoutElementTemplate::create("asd"s),
		hat::core::LayoutElementTemplate::create("fgh"s) };
	auto row2_1 = std::vector<hat::core::LayoutElementTemplate>{ hat::core::LayoutElementTemplate::create("cell_1"s) };

	testReferencePage2.push_row(row2_0);
	testReferencePage2.push_row(row2_1);
	hat::core::LayoutUserInformation referenceLayoutInfo;
	referenceLayoutInfo.push(testReferencePage1);
	referenceLayoutInfo.push(testReferencePage2);

	std::stringstream mystream2("page:page1\ncell00,cell01;cell1;cell2;cell3\ncell_1\ncell_1;cell_2,cell_2_1\ncell_1;cell_2\npage:page2\nqwe;rty;asd;fgh\ncell_1"s);
	auto test = hat::core::LayoutUserInformation::parseConfigFile(mystream2);
	REQUIRE(test == referenceLayoutInfo);
}

TEST_CASE("layout config file parsing 2", "[layout_config]") {
	hat::test::LayoutConfigParsingVerificator tester;
	tester.startNewPage("page caption");
	tester.addRow({ "e11", "e12", "e13" });
	tester.addRow({ "e21", "e22", "e23" });
	tester.addRow({ "e31", "", "e33" });
	tester.addRow({ "", "e42", "" });
	tester.startNewOptionsSelectorPage("selector page caption", "selector");
	tester.addRow({ "e11_1,e11_2", "e12_1,e12_2" });
	tester.addRow({ "", "" });
	tester.addRow({ "", "22", "rew" });
	tester.addRow({ "", "3123", "ewr" });
	tester.startNewPage("page caption");
	tester.addRow({ "", "" });
	tester.addRow({ "", "22", "rew" });
	tester.verifyConfigIsOK();
}
