// This source file is part of the 'hat' open source project.
// Copyright (c) 2016, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#include "../hat-core/abstract_engine.hpp"
#include <utility>

#include "../external_dependencies/Catch/single_include/catch.hpp"

using namespace std::string_literals;

//Test case for the code correctness for the encode/decode procedures, which are used to package the various numeric codes and indicies into layout element IDs for TAU library
TEST_CASE("ID encode/decode") {

	auto myTestData = std::vector<std::pair<std::string, size_t>>();

	for (size_t i = 0; i < 10000; ++i) {
		myTestData.push_back(std::make_pair(hat::core::encodeNumberInTauIdentifier('a', i), i));
		size_t j = 10000 - i;
		myTestData.push_back(std::make_pair(hat::core::encodeNumberInTauIdentifier('a', j), j));
	}

	std::random_shuffle(myTestData.begin(), myTestData.end());

	for (auto const & elem : myTestData) {
		if (hat::core::getEncodedNumberFromTauIdentifier(elem.first) != elem.second) {
			std::stringstream message;
			message << "Error during verification of the ID encoding logic.\nFor the layout element ID='" << elem.first << "'\nGot the decoded result: '" << elem.second << "'";
			FAIL(message.str());
		}
	}
}

namespace {
//NOTE: this is a very non-generic implementation. It could be implemented much better. Should be not very hard to add variadic templates to ensure that any set of parameters can be tested.
class CallExpectationTester
{
public:
	struct Expectation
	{
		std::vector<size_t> expectedParameterValues;
		std::string expectedFunctionName;
		Expectation(std::string const & funcID) : expectedFunctionName(funcID) {};
		Expectation & param(size_t value) { expectedParameterValues.push_back(value); return *this; };
	};
private:
	std::vector<Expectation> m_expectedCalls;
	size_t m_verifiedCount{ 0 };
public:

	CallExpectationTester & expect(std::string const & funcID, size_t param)
	{
		return expect(Expectation(funcID).param(param));
	}

	CallExpectationTester & expect(Expectation const & e)
	{
		m_expectedCalls.push_back(e);
		return *this;
	}
	void verifyFunctionCall(std::string const & funcID, std::vector<size_t> params)
	{
		if (m_verifiedCount >= m_expectedCalls.size()) {
			FAIL("Too much functions are called. The expectations list is exhausted.");
		}
		if (m_expectedCalls[m_verifiedCount].expectedFunctionName != funcID) {
			std::stringstream errorMessage;
			errorMessage << "Unexpected call at position " << m_verifiedCount << " of the expectation list.";
			FAIL(errorMessage.str());
		}
		if (m_expectedCalls[m_verifiedCount].expectedParameterValues != params) {
			std::stringstream errorMessage;
			errorMessage << "Wrong parameters values for the call at position " << m_verifiedCount << " of the expectation list.";
			// Multiline messages for FAIL cause a crash of the testing framework. TODO: figure out a workaround
			errorMessage << "Expected values: { ";
			for (auto exp_param : m_expectedCalls[m_verifiedCount].expectedParameterValues) {
				errorMessage << exp_param << " ";
			}
			errorMessage << "}.";
			//errorMessage << "\n";
			errorMessage << "Actual values: { ";
			for (auto actual_param : params) {
				errorMessage << actual_param << " ";
			}
			errorMessage << "}";
			FAIL(errorMessage.str());
		}
		++m_verifiedCount;
	}

	bool allExpectationsFulfilled() const {
		return m_verifiedCount == m_expectedCalls.size();
	}
};

std::string generateUniqueStr()
{
	static long counter;
	std::stringstream result;
	result << "method_" << counter;
	return result.str();
}
class MyTestEngine : public hat::core::AbstractEngine
{
protected:
	virtual bool setNewEnvironment(size_t envIndex) override
	{
		m_callsExpectationsTester.verifyFunctionCall(FUNC_ID_setNewEnvironment, std::vector<size_t>{envIndex});
		return setNewEnvResult;
	}
	virtual void stickCurrentTopWindowToSelectedEnvironment() override
	{
		m_callsExpectationsTester.verifyFunctionCall(FUNC_ID_stickCurrentTopWindowToSelectedEnvironment, std::vector<size_t>{});
	}
	virtual bool canSendTheCommmandForEnvironment() const override
	{
		m_callsExpectationsTester.verifyFunctionCall(FUNC_ID_canSendTheCommmandForEnvironment, std::vector<size_t>{});
		return canSendCommandsToWindowRightNow;
	}
	virtual void executeCommandForCurrentlySelectedEnvironment(size_t commandIndex) override
	{
		m_callsExpectationsTester.verifyFunctionCall(FUNC_ID_executeCommandForCurrentlySelectedEnvironment, std::vector<size_t>{commandIndex});
	}
	virtual void switchLayout_wrongTopmostWindow() override
	{
		m_callsExpectationsTester.verifyFunctionCall(FUNC_ID_switchLayout_wrongTopmostWindow, std::vector<size_t>{});
	}
	virtual void switchLayout_restoreToNormalLayout() override
	{
		m_callsExpectationsTester.verifyFunctionCall(FUNC_ID_switchLayout_restoreToNormalLayout, std::vector<size_t>{});
	}
public:
	//TODO: auto-generate this stuff for each of the overriden methods:
	const std::string FUNC_ID_setNewEnvironment;
	const std::string FUNC_ID_stickCurrentTopWindowToSelectedEnvironment;
	const std::string FUNC_ID_canSendTheCommmandForEnvironment;
	const std::string FUNC_ID_executeCommandForCurrentlySelectedEnvironment;
	const std::string FUNC_ID_switchLayout_wrongTopmostWindow;
	const std::string FUNC_ID_switchLayout_restoreToNormalLayout;

	MyTestEngine() :
		FUNC_ID_setNewEnvironment(generateUniqueStr()),
		FUNC_ID_stickCurrentTopWindowToSelectedEnvironment(generateUniqueStr()),
		FUNC_ID_canSendTheCommmandForEnvironment(generateUniqueStr()),
		FUNC_ID_executeCommandForCurrentlySelectedEnvironment(generateUniqueStr()),
		FUNC_ID_switchLayout_wrongTopmostWindow(generateUniqueStr()),
		FUNC_ID_switchLayout_restoreToNormalLayout(generateUniqueStr())
	{}
	mutable CallExpectationTester m_callsExpectationsTester;
	bool canSendCommandsToWindowRightNow;
	bool setNewEnvResult;
};
}

SCENARIO("AbstractEngine's basic commands processsing")
{
	using namespace hat::core;
	GIVEN("Initial state of the abstract engine without window tracking (or with the tracked window on top)") {
		MyTestEngine eng;
		eng.canSendCommandsToWindowRightNow = true;
		WHEN("Command button is pressed") {
			auto encodedCommandIndex = size_t{ 10 };
			auto buttonIdRepresentingAction = AbstractEngine::generateTauIdentifierForCommand(encodedCommandIndex);
			eng.m_callsExpectationsTester.expect(eng.FUNC_ID_canSendTheCommmandForEnvironment);
			eng.m_callsExpectationsTester.expect(eng.FUNC_ID_executeCommandForCurrentlySelectedEnvironment, encodedCommandIndex);
			auto callResult = eng.buttonOnLayoutClicked(buttonIdRepresentingAction);
			THEN("Expected callback is executed and the result of button processing is NONE") {
				REQUIRE(callResult == FeedbackFromButtonClick::NONE);
				CHECK(eng.m_callsExpectationsTester.allExpectationsFulfilled());
			}
		}
		WHEN("Environment switch button is pressed") {
			auto environmentToSwitchTo = size_t{ 23 };
			auto buttonIdRepresentingAction = AbstractEngine::generateTauIdentifierForEnvSwitching(environmentToSwitchTo);
			eng.m_callsExpectationsTester.expect(eng.FUNC_ID_setNewEnvironment, environmentToSwitchTo);
			auto callResult = eng.buttonOnLayoutClicked(buttonIdRepresentingAction);
			THEN("The environment switching callback is executed and result of button processing is UPDATE_LAYOUT") {
				REQUIRE(callResult == FeedbackFromButtonClick::UPDATE_LAYOUT);
				CHECK(eng.m_callsExpectationsTester.allExpectationsFulfilled());
			}
		}
		WHEN("'Stick to topmost window' button is pressed") { // This is the situation when there is only one environment, so it is automatically picked up as selected.
			auto buttonIdRepresentingAction = AbstractEngine::generateStickEnvironmentToWindowCommand();
			eng.m_callsExpectationsTester.expect(eng.FUNC_ID_stickCurrentTopWindowToSelectedEnvironment);
			auto callResult = eng.buttonOnLayoutClicked(buttonIdRepresentingAction);
			THEN("Expected callback is called and UPDATE_LAYOUT is returned") {
				REQUIRE(callResult == FeedbackFromButtonClick::UPDATE_LAYOUT);
				CHECK(eng.m_callsExpectationsTester.allExpectationsFulfilled());
			}
		}
		WHEN("Tracked window goes out of reachability, commands are not executed") {
			eng.canSendCommandsToWindowRightNow = false;
			auto encodedCommandIndex = size_t{ 235 };
			auto buttonIdRepresentingAction = AbstractEngine::generateTauIdentifierForCommand(encodedCommandIndex);
			eng.m_callsExpectationsTester.expect(eng.FUNC_ID_canSendTheCommmandForEnvironment);
			eng.m_callsExpectationsTester.expect(eng.FUNC_ID_switchLayout_wrongTopmostWindow);
			auto callResult = eng.buttonOnLayoutClicked(buttonIdRepresentingAction);
			THEN("The command is saved as pending, layout is refreshed to a special page stub for unreachable window (which is requested with special callback)") {
				REQUIRE(callResult == FeedbackFromButtonClick::UPDATE_LAYOUT);
				REQUIRE(eng.hasPendingCommand());
				CHECK(eng.m_callsExpectationsTester.allExpectationsFulfilled());
			}
			WHEN("Focus is restored, retry button is pressed") {
				eng.canSendCommandsToWindowRightNow = true;
				auto buttonIdRepresentingAction = AbstractEngine::generateResendPendingCommandButtonID();
				eng.m_callsExpectationsTester.expect(eng.FUNC_ID_canSendTheCommmandForEnvironment);
				eng.m_callsExpectationsTester.expect(eng.FUNC_ID_executeCommandForCurrentlySelectedEnvironment, encodedCommandIndex);
				eng.m_callsExpectationsTester.expect(eng.FUNC_ID_switchLayout_restoreToNormalLayout);
				auto callResult = eng.buttonOnLayoutClicked(buttonIdRepresentingAction);
				THEN("Required methods are called, layout is refreshed") {
					REQUIRE(callResult == FeedbackFromButtonClick::UPDATE_LAYOUT);
					CHECK(eng.m_callsExpectationsTester.allExpectationsFulfilled());
				}
			}
			WHEN("Cancel is pressed") {
				auto buttonIdRepresentingAction = AbstractEngine::generateClearPendingCommandButtonID();
				eng.m_callsExpectationsTester.expect(eng.FUNC_ID_switchLayout_restoreToNormalLayout);
				auto callResult = eng.buttonOnLayoutClicked(buttonIdRepresentingAction);
				THEN("Pending command is removed, child class notified, and layout refreshed") {
					REQUIRE(callResult == FeedbackFromButtonClick::UPDATE_LAYOUT);
					REQUIRE_FALSE(eng.hasPendingCommand());
					CHECK(eng.m_callsExpectationsTester.allExpectationsFulfilled());
				}
			}
		}

	}

}
