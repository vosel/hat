// This source file is part of the 'hat' open source project.
// Copyright (c) 2017, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#include "variables_manager_testing_utils.hpp"

#include "../external_dependencies/Catch/single_include/catch.hpp"

namespace hat {
namespace test {

//TODO: this function is probably duplicated somewhere else - need to clean it up
std::string getUniqueIdString()
{
	static size_t counter = 0;
	++counter;
	std::stringstream result;
	result << "test_id_" << counter;
	return result.str();
}

void setInitialValue(hat::core::VariablesManager & variablesManager, MyVariableTestDetails const & dataToSet) {
	variablesManager.setVariableInitialValue(dataToSet.id, dataToSet.m_initialValue);
}
void verifyValue(hat::core::VariablesManager & variablesManager, MyVariableTestDetails const & varInfo, std::string const & expectedValue) {
	REQUIRE(variablesManager.getValue(varInfo.id) == expectedValue);
}
void verifyInitialValue(hat::core::VariablesManager & variablesManager, MyVariableTestDetails const & varInfo) {
	verifyValue(variablesManager, varInfo, varInfo.m_initialValue);
}
void verifyUpdatedValue(hat::core::VariablesManager & variablesManager, MyVariableTestDetails const & varInfo) {
	verifyValue(variablesManager, varInfo, varInfo.m_updatedValue);
}

} // namespace test
} // namespace hat