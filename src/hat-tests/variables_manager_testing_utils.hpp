#ifndef	VARIABLES_MANAGER_TESTING_UTILS_HPP
#define VARIABLES_MANAGER_TESTING_UTILS_HPP

#include "../hat-core/variables_manager.hpp"
namespace hat {
namespace test {


std::string getUniqueIdString();

// A simple class, which is used to aggregate the interesting data for the variable's testing procedure. It is making the test code more readable.
struct MyVariableTestDetails
{
	hat::core::VariableID const id;
	std::string const m_initialValue;
	std::string const m_updatedValue;
	MyVariableTestDetails(
		std::string const & expectedInitialValue,
		std::string const & expectedUpdatedValue)
		: id(getUniqueIdString()), m_initialValue(expectedInitialValue), m_updatedValue(expectedUpdatedValue) {
	}
};

void setInitialValue(hat::core::VariablesManager & variablesManager, MyVariableTestDetails const & dataToSet);
void verifyValue(hat::core::VariablesManager & variablesManager, MyVariableTestDetails const & varInfo, std::string const & expectedValue);
void verifyInitialValue(hat::core::VariablesManager & variablesManager, MyVariableTestDetails const & varInfo);
void verifyUpdatedValue(hat::core::VariablesManager & variablesManager, MyVariableTestDetails const & varInfo);

} // namespace test
} // namespace hat
#endif //VARIABLES_MANAGER_TESTING_UTILS_HPP