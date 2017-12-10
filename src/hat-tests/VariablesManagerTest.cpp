// This source file is part of the 'hat' open source project.
// Copyright (c) 2017, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#include "variables_manager_testing_utils.hpp"
#include "../external_dependencies/Catch/single_include/catch.hpp"
#include <sstream>
#include <functional>

namespace {

//simple util template, which is useful in determining the a container has all of the elements from given set
template<typename T, typename U>
void checkContainsAll(T const & container, U const & elementsToCheck)
{
	auto missingElementIter = std::find_if(
		std::begin(elementsToCheck),
		std::end(elementsToCheck),
		[&](U::value_type const & toCheck) -> bool
		{
			return std::find(std::begin(container), std::end(container), toCheck) == std::end(container);
		});
	REQUIRE(missingElementIter == std::end(elementsToCheck));
}
}

TEST_CASE("text variables manager basic functionality (single-variable cases)")
{
	std::string value1 = hat::test::getUniqueIdString();
	std::string value2 = hat::test::getUniqueIdString();
	size_t const FIRST_OPERATION_INDEX = 10; //just a random number here
	size_t const SECOND_OPERATION_INDEX = FIRST_OPERATION_INDEX + 10; //just a random number here (should not be equal to the previous one)
	hat::test::MyVariableTestDetails VARIABLE_THAT_SHOULD_BE_APPENDED_TO(value1, value1+value2);
	hat::test::MyVariableTestDetails VARIABLE_THAT_SHOULD_BE_ASSIGNED_TO(value1, value2);
	hat::test::MyVariableTestDetails VARIABLE_THAT_SHOULD_NOT_BE_CHANGED_ON_FIRST_COMMAND(value1, value2);
	WHEN ("Variables manager is created") {
		hat::core::VariablesManager variablesManager;
		THEN ("It should not contain anything") {
			REQUIRE_FALSE(variablesManager.variableExists(VARIABLE_THAT_SHOULD_BE_APPENDED_TO.id));
			REQUIRE_FALSE(variablesManager.variableExists(VARIABLE_THAT_SHOULD_BE_ASSIGNED_TO.id));
			REQUIRE_FALSE(variablesManager.variableExists(VARIABLE_THAT_SHOULD_NOT_BE_CHANGED_ON_FIRST_COMMAND.id));
		}
		WHEN("A set of variables are added to the manager") {
			variablesManager.declareVariable(VARIABLE_THAT_SHOULD_BE_APPENDED_TO.id);
			variablesManager.declareVariable(VARIABLE_THAT_SHOULD_BE_ASSIGNED_TO.id);
			variablesManager.declareVariable(VARIABLE_THAT_SHOULD_NOT_BE_CHANGED_ON_FIRST_COMMAND.id);
			THEN("They should become accessible from the manager by id.") {
				REQUIRE(variablesManager.variableExists(VARIABLE_THAT_SHOULD_BE_APPENDED_TO.id));
				REQUIRE(variablesManager.variableExists(VARIABLE_THAT_SHOULD_BE_ASSIGNED_TO.id));
				REQUIRE(variablesManager.variableExists(VARIABLE_THAT_SHOULD_NOT_BE_CHANGED_ON_FIRST_COMMAND.id));
				AND_THEN("Their values should be empty strings") {
					verifyValue(variablesManager, VARIABLE_THAT_SHOULD_BE_APPENDED_TO, "");
					verifyValue(variablesManager, VARIABLE_THAT_SHOULD_BE_ASSIGNED_TO, "");
					verifyValue(variablesManager, VARIABLE_THAT_SHOULD_NOT_BE_CHANGED_ON_FIRST_COMMAND, "");
				}
			}
			AND_WHEN("Initial value is provided for the variables") {
				setInitialValue(variablesManager, VARIABLE_THAT_SHOULD_BE_APPENDED_TO);
				setInitialValue(variablesManager, VARIABLE_THAT_SHOULD_BE_ASSIGNED_TO);
				setInitialValue(variablesManager, VARIABLE_THAT_SHOULD_NOT_BE_CHANGED_ON_FIRST_COMMAND);
				THEN("These initial values should be returned by the manager on the requests") {
					verifyInitialValue(variablesManager, VARIABLE_THAT_SHOULD_BE_APPENDED_TO);
					verifyInitialValue(variablesManager, VARIABLE_THAT_SHOULD_BE_ASSIGNED_TO);
					verifyInitialValue(variablesManager, VARIABLE_THAT_SHOULD_NOT_BE_CHANGED_ON_FIRST_COMMAND);
				}
				AND_WHEN("The simple variable operations are registered for different commands execution") {
					variablesManager.addOperationToExecuteOnCommand(FIRST_OPERATION_INDEX, 
						std::make_shared<hat::core::AppendText>(VARIABLE_THAT_SHOULD_BE_APPENDED_TO.id, value2));
					variablesManager.addOperationToExecuteOnCommand(FIRST_OPERATION_INDEX, 
						std::make_shared<hat::core::AssignText>(VARIABLE_THAT_SHOULD_BE_ASSIGNED_TO.id, value2));
					variablesManager.addOperationToExecuteOnCommand(SECOND_OPERATION_INDEX, 
						std::make_shared<hat::core::AssignText>(VARIABLE_THAT_SHOULD_NOT_BE_CHANGED_ON_FIRST_COMMAND.id, value2));
					THEN("The variables values should not change yet") {
						verifyInitialValue(variablesManager, VARIABLE_THAT_SHOULD_BE_APPENDED_TO);
						verifyInitialValue(variablesManager, VARIABLE_THAT_SHOULD_BE_ASSIGNED_TO);
						verifyInitialValue(variablesManager, VARIABLE_THAT_SHOULD_NOT_BE_CHANGED_ON_FIRST_COMMAND);
					}
					AND_WHEN("The first command is executed") {
						auto changedVariables = variablesManager.executeCommandAndGetChangedVariablesList(FIRST_OPERATION_INDEX);
						THEN("The list of changed variables should contain the IDs for variables registered for it"){
							checkContainsAll(changedVariables,
								std::vector<hat::core::VariableID>{ VARIABLE_THAT_SHOULD_BE_ASSIGNED_TO.id, VARIABLE_THAT_SHOULD_BE_APPENDED_TO.id }
							);
						}
						AND_THEN("The variables values should be updated accordingly") {
							verifyUpdatedValue(variablesManager, VARIABLE_THAT_SHOULD_BE_APPENDED_TO);
							verifyUpdatedValue(variablesManager, VARIABLE_THAT_SHOULD_BE_ASSIGNED_TO);
						}
						AND_THEN("The variable, which should remain unchanged, should do so") {
							verifyInitialValue(variablesManager, VARIABLE_THAT_SHOULD_NOT_BE_CHANGED_ON_FIRST_COMMAND);
						}
						AND_WHEN("The second command is executed") {
							auto changedVariables2 = variablesManager.executeCommandAndGetChangedVariablesList(SECOND_OPERATION_INDEX);
							THEN("The list of changed variables should contain the ID for variable registered for it"){
								checkContainsAll(changedVariables2,
									std::vector<hat::core::VariableID>{ VARIABLE_THAT_SHOULD_NOT_BE_CHANGED_ON_FIRST_COMMAND.id }
								);
							}
							AND_THEN("All the variables values should be in updated state") {
								verifyUpdatedValue(variablesManager, VARIABLE_THAT_SHOULD_BE_APPENDED_TO);
								verifyUpdatedValue(variablesManager, VARIABLE_THAT_SHOULD_BE_ASSIGNED_TO);
								verifyUpdatedValue(variablesManager, VARIABLE_THAT_SHOULD_NOT_BE_CHANGED_ON_FIRST_COMMAND);
							}
						}
					}
				}
			}
		}
	}
}

TEST_CASE("variables manager inter-variable assignments tests")
{
	// Initialisation code (all the invariants, which should be held there are tested in another test case (see above))
	std::string value1 = hat::test::getUniqueIdString();
	std::string value2 = hat::test::getUniqueIdString();
	size_t const OPERATION_INDEX_TO_USE = 10; //just a random number here
	hat::test::MyVariableTestDetails VARIABLE_THAT_SHOULD_BE_APPENDED_TO(value1, value1+value2);
	hat::test::MyVariableTestDetails VARIABLE_THAT_SHOULD_BE_ASSIGNED_TO(value1, value2);
	hat::test::MyVariableTestDetails VARIABLE_THAT_HOLDS_THE_PARAMETER_VALUE(value2, value2);
	
	hat::core::VariablesManager variablesManager;

	variablesManager.declareVariable(VARIABLE_THAT_SHOULD_BE_APPENDED_TO.id);
	variablesManager.declareVariable(VARIABLE_THAT_SHOULD_BE_ASSIGNED_TO.id);
	variablesManager.declareVariable(VARIABLE_THAT_HOLDS_THE_PARAMETER_VALUE.id);

	setInitialValue(variablesManager, VARIABLE_THAT_SHOULD_BE_APPENDED_TO);
	setInitialValue(variablesManager, VARIABLE_THAT_SHOULD_BE_ASSIGNED_TO);
	setInitialValue(variablesManager, VARIABLE_THAT_HOLDS_THE_PARAMETER_VALUE);

	variablesManager.addOperationToExecuteOnCommand(OPERATION_INDEX_TO_USE, 
		std::make_shared<hat::core::AppendValue>(VARIABLE_THAT_SHOULD_BE_APPENDED_TO.id, VARIABLE_THAT_HOLDS_THE_PARAMETER_VALUE.id));
	variablesManager.addOperationToExecuteOnCommand(OPERATION_INDEX_TO_USE, 
		std::make_shared<hat::core::AssignValue>(VARIABLE_THAT_SHOULD_BE_ASSIGNED_TO.id, VARIABLE_THAT_HOLDS_THE_PARAMETER_VALUE.id));
	// end of initialisation code

	WHEN("The command is executed") {
		auto changedVariables = variablesManager.executeCommandAndGetChangedVariablesList(OPERATION_INDEX_TO_USE);
		THEN("The list of changed variables should contain the IDs for variables registered for it"){
			checkContainsAll(changedVariables,
				std::vector<hat::core::VariableID>{ VARIABLE_THAT_SHOULD_BE_APPENDED_TO.id, VARIABLE_THAT_SHOULD_BE_ASSIGNED_TO.id }
			);
		}
		AND_THEN("All the variables values should be in updated state") {
			verifyUpdatedValue(variablesManager, VARIABLE_THAT_SHOULD_BE_APPENDED_TO);
			verifyUpdatedValue(variablesManager, VARIABLE_THAT_SHOULD_BE_ASSIGNED_TO);
			verifyInitialValue(variablesManager, VARIABLE_THAT_HOLDS_THE_PARAMETER_VALUE); //since we don't change this variable, it's updated value should be equal to it's initial value
			verifyUpdatedValue(variablesManager, VARIABLE_THAT_HOLDS_THE_PARAMETER_VALUE);
		}
	}
}


namespace {

//This function is here for two purposes: 
//  1) cast the types of the compared objects to the base class
//  2) check that the comparisons work correctly both ways
bool getEqualityComparisonResult(hat::core::VariableOperation const & first, hat::core::VariableOperation const & second) {
	auto left = first == second;
	auto right = second == first;
	
	REQUIRE(left == right);
	return left;
}

// Little helper function for unit-testing comparison operations between two operation objects
template<typename OperationType>
void checkInequalityForTwoStringOperationObjects(std::function<OperationType (std::string const & first, std::string const & second)> objectFactoryMethod)
{
	std::string string_var1 = hat::test::getUniqueIdString();
	std::string string_var2 = hat::test::getUniqueIdString();
	std::string string_var3 = hat::test::getUniqueIdString();
	auto referenceObject = objectFactoryMethod(string_var1, string_var2);
	auto differentObjectAtFirstParam = objectFactoryMethod(string_var3, string_var2);
	auto differentObjectAtSecondParam = objectFactoryMethod(string_var1, string_var3);
	
	//Check difference in one parameter
	REQUIRE_FALSE(getEqualityComparisonResult(referenceObject, differentObjectAtFirstParam));
	REQUIRE_FALSE(getEqualityComparisonResult(referenceObject, differentObjectAtSecondParam));

	//Check difference in both parameters
	REQUIRE_FALSE(getEqualityComparisonResult(differentObjectAtFirstParam, differentObjectAtSecondParam));
}

}

// Simple unit test, which ensures that our comparison operations work correctly (different types of the operations should never be equal)
TEST_CASE("Equality comparisons for the variable operation objects")
{
	namespace hc = hat::core;
	//All the string variables are the same here on purpose - we want to ensure that the type is checked during comparison
	auto const string_id = hc::VariableID{hat::test::getUniqueIdString()};
	auto const uint_var = size_t{10};

	auto clearTailCharsObj = hc::ClearTailCharacters{string_id, uint_var};
	auto appendValueObj = hc::AppendValue{string_id, string_id};
	auto assignValueObj = hc::AssignValue{string_id, string_id};
	auto appendTextObj = hc::AppendText{string_id, string_id.getValue()};
	auto assignTextObj = hc::AssignText{string_id, string_id.getValue()};

	auto clearTailCharsObj2 = hc::ClearTailCharacters{string_id, uint_var};
	auto appendValueObj2 = hc::AppendValue{string_id, string_id};
	auto assignValueObj2 = hc::AssignValue{string_id, string_id};
	auto appendTextObj2 = hc::AppendText{string_id, string_id.getValue()};
	auto assignTextObj2 = hc::AssignText{string_id, string_id.getValue()};

	REQUIRE_FALSE(getEqualityComparisonResult(clearTailCharsObj, appendValueObj));
	REQUIRE_FALSE(getEqualityComparisonResult(clearTailCharsObj, assignValueObj));
	REQUIRE_FALSE(getEqualityComparisonResult(clearTailCharsObj, appendTextObj));
	REQUIRE_FALSE(getEqualityComparisonResult(clearTailCharsObj, assignTextObj));
	REQUIRE      (getEqualityComparisonResult(clearTailCharsObj, clearTailCharsObj2));

	REQUIRE_FALSE(getEqualityComparisonResult(appendValueObj, assignValueObj));
	REQUIRE_FALSE(getEqualityComparisonResult(appendValueObj, appendTextObj));
	REQUIRE_FALSE(getEqualityComparisonResult(appendValueObj, assignTextObj));
	REQUIRE      (getEqualityComparisonResult(appendValueObj, appendValueObj2));

	REQUIRE_FALSE(getEqualityComparisonResult(assignValueObj, appendTextObj));
	REQUIRE_FALSE(getEqualityComparisonResult(assignValueObj, assignTextObj));
	REQUIRE      (getEqualityComparisonResult(assignValueObj, assignValueObj2));

	REQUIRE_FALSE(getEqualityComparisonResult(appendTextObj, assignTextObj));
	REQUIRE      (getEqualityComparisonResult(appendTextObj, appendTextObj2));

	REQUIRE      (getEqualityComparisonResult(assignTextObj, assignTextObj2));

	checkInequalityForTwoStringOperationObjects<hc::AppendValue>(
		[](std::string const & first, std::string const & second) {
			return hc::AppendValue{hc::VariableID{first}, hc::VariableID{second}};
		}
	);
	checkInequalityForTwoStringOperationObjects<hc::AssignValue>(
		[](std::string const & first, std::string const & second) {
			return hc::AssignValue{hc::VariableID{first}, hc::VariableID{second}};
		}
	);
	checkInequalityForTwoStringOperationObjects<hc::AppendText>(
		[](std::string const & first, std::string const & second) {
			return hc::AppendText{hc::VariableID{first}, second};
		}
	);
	checkInequalityForTwoStringOperationObjects<hc::AssignText>(
		[](std::string const & first, std::string const & second) {
			return hc::AssignText{hc::VariableID{first}, second};
		}
	);

	//TODO: [low priority] refactoring - get rid of the code duplication here - we can generalize the logic from checkInequalityForTwoStringOperationObjects() template, so it will also handle the variableID->size_t parameters pair. This will allow us to get rid of the code below, which is copy-pasted from checkInequalityForTwoStringOperationObjects()
	auto const string_id2 = hc::VariableID{hat::test::getUniqueIdString()};
	auto const uint_var2 = size_t{uint_var + 1};
	auto clearTailCharsObj_diff0 = hc::ClearTailCharacters{string_id2, uint_var};
	auto clearTailCharsObj_diff1 = hc::ClearTailCharacters{string_id, uint_var2};

	//Check difference in one parameter
	REQUIRE_FALSE(getEqualityComparisonResult(clearTailCharsObj, clearTailCharsObj_diff0));
	REQUIRE_FALSE(getEqualityComparisonResult(clearTailCharsObj, clearTailCharsObj_diff1));

	//Check difference in both parameters
	REQUIRE_FALSE(getEqualityComparisonResult(clearTailCharsObj_diff0, clearTailCharsObj_diff1));
}

TEST_CASE("variables manager 'clear tail characters' operation test")
{
	std::string updated_value = hat::test::getUniqueIdString();
	std::string const TAIL_TO_CLEAR = "ABC";
	std::string orig_value = updated_value + TAIL_TO_CLEAR;
	size_t const OPERATION_INDEX_TO_USE = 20; //just a random number here
	hat::test::MyVariableTestDetails VARIABLE_THAT_SHOULD_BE_CHANGED_WITH_BACKSPACE(orig_value, updated_value);
	hat::test::MyVariableTestDetails EMPTY_VARIABLE("", ""); //backspace should not change the value of this variable

	hat::core::VariablesManager variablesManager;
	variablesManager.declareVariable(VARIABLE_THAT_SHOULD_BE_CHANGED_WITH_BACKSPACE.id);
	variablesManager.declareVariable(EMPTY_VARIABLE.id);

	setInitialValue(variablesManager, VARIABLE_THAT_SHOULD_BE_CHANGED_WITH_BACKSPACE);
	setInitialValue(variablesManager, EMPTY_VARIABLE);

	variablesManager.addOperationToExecuteOnCommand(OPERATION_INDEX_TO_USE, 
		std::make_shared<hat::core::ClearTailCharacters>(
			VARIABLE_THAT_SHOULD_BE_CHANGED_WITH_BACKSPACE.id, TAIL_TO_CLEAR.size()));

	variablesManager.addOperationToExecuteOnCommand(OPERATION_INDEX_TO_USE, 
		std::make_shared<hat::core::ClearTailCharacters>(
			EMPTY_VARIABLE.id, 1));
	WHEN("The command is executed") {
		auto changedVariables = variablesManager.executeCommandAndGetChangedVariablesList(OPERATION_INDEX_TO_USE);
		THEN("The list of changed variables should contain the IDs for variables registered for it (even if the backspace operation didn't change the value - for consistency sake)"){
			checkContainsAll(changedVariables,
				std::vector<hat::core::VariableID>{ VARIABLE_THAT_SHOULD_BE_CHANGED_WITH_BACKSPACE.id, EMPTY_VARIABLE.id }
			);
		}
		THEN("A last character of the variable's value should be removed") {
			verifyUpdatedValue(variablesManager, VARIABLE_THAT_SHOULD_BE_CHANGED_WITH_BACKSPACE);
		}
		AND_THEN("An empty variable should still be empty") {
			verifyUpdatedValue(variablesManager, EMPTY_VARIABLE);
		}
	}
}

//  Test that the order of operations is preserved during construction, 
//  is noticeable for the hat::core::VariableManager's equality operator
//  and actually matters for the behaviour of the manager:
TEST_CASE("Preservation of the operations order") {
	auto const INITIAL_VALUE = std::string{"xyz"};
	auto const STR_TO_APPEND = std::string{"abc"};
	size_t const CHARACTERS_TO_CLEAR = 2;
	size_t const COMMAND_INDEX_TO_USE = 123;
	
	auto const EXPECTED_RESULT_VALUE_0 = INITIAL_VALUE.substr(0, 1) + STR_TO_APPEND;
	auto const EXPECTED_RESULT_VALUE_1 = INITIAL_VALUE + STR_TO_APPEND.substr(0, 1);
	
	auto const VAR_ID = hat::core::VariableID{ hat::test::getUniqueIdString() };
	
	hat::core::VariablesManager variablesManager0;
	hat::core::VariablesManager variablesManager1;
	variablesManager0.declareVariable(VAR_ID);
	variablesManager0.setVariableInitialValue(VAR_ID, INITIAL_VALUE);
	
	variablesManager1.declareVariable(VAR_ID);
	variablesManager1.setVariableInitialValue(VAR_ID, INITIAL_VALUE);

	//So far, there should be no difference between varaible menagers:
	REQUIRE(variablesManager0 == variablesManager1);
	
	WHEN("operations are added for the same command ID, but in different order") {
		auto const OPERATION_0 = std::make_shared<hat::core::ClearTailCharacters>(VAR_ID, CHARACTERS_TO_CLEAR);
		auto const OPERATION_1 = std::make_shared<hat::core::AppendText>(VAR_ID, STR_TO_APPEND);

		variablesManager0.addOperationToExecuteOnCommand(COMMAND_INDEX_TO_USE, OPERATION_0);
		variablesManager0.addOperationToExecuteOnCommand(COMMAND_INDEX_TO_USE, OPERATION_1);
		
		variablesManager1.addOperationToExecuteOnCommand(COMMAND_INDEX_TO_USE, OPERATION_1);
		variablesManager1.addOperationToExecuteOnCommand(COMMAND_INDEX_TO_USE, OPERATION_0);
		
		THEN("the variable managers should become different") {
			REQUIRE_FALSE(variablesManager0 == variablesManager1);
		}
		AND_WHEN("the command is executed") {
			auto const listOfUpdatedVariables0 = variablesManager0.executeCommandAndGetChangedVariablesList(COMMAND_INDEX_TO_USE);
			auto const listOfUpdatedVariables1 = variablesManager1.executeCommandAndGetChangedVariablesList(COMMAND_INDEX_TO_USE);
			THEN("the variable should change to different values (for each manager)") {
				REQUIRE(variablesManager0.getValue(VAR_ID) == EXPECTED_RESULT_VALUE_0);
				REQUIRE(variablesManager1.getValue(VAR_ID) == EXPECTED_RESULT_VALUE_1);
				AND_THEN("the variable is listed between changed ones") { // sanity check: this logic is tested in another unit test. Here we check it just because we can.
					auto const expectedListOfUpdatedVariables = std::vector<hat::core::VariableID>{VAR_ID};
					REQUIRE(listOfUpdatedVariables0 == expectedListOfUpdatedVariables);
					REQUIRE(listOfUpdatedVariables1 == expectedListOfUpdatedVariables);
				}
			}
		}
	}
}
