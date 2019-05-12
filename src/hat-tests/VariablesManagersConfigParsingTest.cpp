// This source file is part of the 'hat' open source project.
// Copyright (c) 2017, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#include "commands_parsing_testing_utils.hpp"
#include "variables_manager_testing_utils.hpp"
#include "variables_managers_config_building_utils.hpp"
#include "../external_dependencies/Catch/single_include/catch.hpp"

namespace {

// This set of classes and template instantiations are used to encapsulate the relation between the configuration file text
// and the actual hat::core::VariableOperation object, which is created from it.
// By passing all the needed information to the methods of the specific class instantiation we can generate both of them.
// Note: all the operation-related variables should be passed to the object constructor.
// The parameters, which are passed to the methods are only the ones, which are not related to the operation itself,
// but to the circumstances, in which this operation should be envoked (commandID and environments)
struct VariableOperationLogic
{
	virtual std::string getConfigFileTextForOperation(
		hat::core::CommandID const & commandID,
		hat::test::EnvironmentsStringWrapper const & environmentsSetup) const = 0;
	virtual void addOperationToRealManager(
		size_t command_index,
		hat::core::VariablesManager & target) const = 0;
};

template <typename Derived>
class StringToValue : public VariableOperationLogic
{
protected:
	hat::core::VariableID const & m_variableToUpdate;
	std::string const & m_stringParameter;

	StringToValue(hat::core::VariableID const & variableToUpdate, std::string const & stringParameter)
		: m_variableToUpdate(variableToUpdate), m_stringParameter(stringParameter)
	{}

public:
	void addOperationToRealManager(
		size_t command_index,
		hat::core::VariablesManager & target) const override
	{
		target.addOperationToExecuteOnCommand(command_index,
			std::make_shared<Derived>(m_variableToUpdate, m_stringParameter));
	};
};

struct AppendTextLogic : public StringToValue<hat::core::AppendText>
{
	AppendTextLogic (hat::core::VariableID const & variableToUpdate, std::string const & stringParameter)
		: StringToValue(variableToUpdate, stringParameter)
	{}

	std::string getConfigFileTextForOperation(
		hat::core::CommandID const & commandID,
		hat::test::EnvironmentsStringWrapper const & environmentsSetup) const override
	{
		return hat::test::append_text(commandID, m_variableToUpdate, environmentsSetup, m_stringParameter);
	};
};

struct AssignTextLogic : public StringToValue<hat::core::AssignText>
{
	AssignTextLogic (hat::core::VariableID const & variableToUpdate, std::string const & stringParameter)
		: StringToValue(variableToUpdate, stringParameter)
	{}

	static AssignTextLogic createFrom(hat::test::MyVariableTestDetails const & var_info)
	{
		return AssignTextLogic{ var_info.id, var_info.m_updatedValue };
	}

	std::string getConfigFileTextForOperation(
		hat::core::CommandID const & commandID,
		hat::test::EnvironmentsStringWrapper const & environmentsSetup) const override
	{
		return hat::test::assign_text(commandID, m_variableToUpdate, environmentsSetup, m_stringParameter);
	};
};

template <typename Derived>
class ValueToValue : public VariableOperationLogic
{
protected:
	hat::core::VariableID const & m_variableToUpdate;
	hat::core::VariableID const & m_variableHoldingValueParameter;

	ValueToValue(hat::core::VariableID const & variableToUpdate, hat::core::VariableID const & variableForDataSource)
		: m_variableToUpdate(variableToUpdate), m_variableHoldingValueParameter(variableForDataSource)
	{}

public:
	void addOperationToRealManager(
		size_t command_index,
		hat::core::VariablesManager & target) const override
	{
		target.addOperationToExecuteOnCommand(command_index,
			std::make_shared<Derived>(m_variableToUpdate, m_variableHoldingValueParameter));
	};
};

struct AppendValueLogic : public ValueToValue<hat::core::AppendValue>
{
	AppendValueLogic (hat::core::VariableID const & variableToUpdate, hat::core::VariableID const & variableForDataSource)
		: ValueToValue(variableToUpdate, variableForDataSource)
	{}

	std::string getConfigFileTextForOperation(
		hat::core::CommandID const & commandID,
		hat::test::EnvironmentsStringWrapper const & environmentsSetup) const override
	{
		return hat::test::append_value(commandID, m_variableToUpdate, environmentsSetup, m_variableHoldingValueParameter);
	};
};

struct AssignValueLogic : public ValueToValue<hat::core::AssignValue>
{
	AssignValueLogic(hat::core::VariableID const & variableToUpdate, hat::core::VariableID const & variableForDataSource)
		: ValueToValue(variableToUpdate, variableForDataSource)
	{}

	std::string getConfigFileTextForOperation(
		hat::core::CommandID const & commandID,
		hat::test::EnvironmentsStringWrapper const & environmentsSetup) const override
	{
		return hat::test::assign_value(commandID, m_variableToUpdate, environmentsSetup, m_variableHoldingValueParameter);
	};
};

struct ClearTailCharactersLogic : public VariableOperationLogic
{
	size_t m_charactersCount;
	hat::core::VariableID m_variableToUpdate;

	ClearTailCharactersLogic(hat::core::VariableID const & variableToUpdate, size_t charactersCount)
		: m_variableToUpdate(variableToUpdate), m_charactersCount(charactersCount)
	{}

	void addOperationToRealManager(
		size_t command_index,
		hat::core::VariablesManager & target) const override
	{
		target.addOperationToExecuteOnCommand(command_index,
			std::make_shared<hat::core::ClearTailCharacters>(m_variableToUpdate, m_charactersCount));
	};

	std::string getConfigFileTextForOperation(
		hat::core::CommandID const & commandID,
		hat::test::EnvironmentsStringWrapper const & environmentsSetup) const override
	{
		return hat::test::clear_last_chars(commandID, m_variableToUpdate, environmentsSetup, m_charactersCount);
	};
};

// some constants, which would be used in testing below:
hat::core::CommandID const COMMAND_0 { "COMMAND_0" };
hat::core::CommandID const COMMAND_1 { "COMMAND_1" };
hat::core::CommandID const COMMAND_2 { "COMMAND_2" };
hat::core::CommandID const COMMAND_3 { "COMMAND_3" };
hat::core::CommandID const UNUSED_COMMAND { "COMMAND_NOT_USED_ON_PURPOSE" };
auto const ALL_ENVS = hat::test::EnvironmentsStringWrapper{ "*" };
auto const ENV_0    = hat::test::EnvironmentsStringWrapper{ "ENV_0" };
auto const ENV_1    = hat::test::EnvironmentsStringWrapper{ "ENV_1" };
auto const NO_ENVS  = hat::test::EnvironmentsStringWrapper{ "" };

// This is a helper function, which creates a CommandsInfoContainer object, which is ready for consuming the VariablesManagers configuration text.
auto util_getInitialContainer()
{
	auto NOT_IMPORTANT_PARAMETERS_FOR_COMMAND_IN_THESE_TESTS
		{ "\tcategoryStr\tnoteStr\tdescStr\tvalueForEnv1\tvalueForEnv2" };

	std::stringstream commands_config_stream;
	commands_config_stream
		<< hat::core::ConfigFilesKeywords::mandatoryCellsNamesInCommandsCSV()
		<< ENV_0.getValue() << "\t" << ENV_1.getValue() << "\n"
		<< COMMAND_0.getValue() << NOT_IMPORTANT_PARAMETERS_FOR_COMMAND_IN_THESE_TESTS << "\n"
		<< COMMAND_1.getValue() << NOT_IMPORTANT_PARAMETERS_FOR_COMMAND_IN_THESE_TESTS << "\n"
		<< COMMAND_2.getValue() << NOT_IMPORTANT_PARAMETERS_FOR_COMMAND_IN_THESE_TESTS << "\n"
		<< COMMAND_3.getValue() << NOT_IMPORTANT_PARAMETERS_FOR_COMMAND_IN_THESE_TESTS << "\n";

	return hat::test::simulateParseConfigFileCall(commands_config_stream.str());
}
}

TEST_CASE("Variables managers config parsing test")
{
	namespace ht = hat::test;

	auto initialContainer = util_getInitialContainer();
	auto const COMMAND_INDEX_0 = initialContainer.getCommandIndex(COMMAND_0);
	auto const COMMAND_INDEX_1 = initialContainer.getCommandIndex(COMMAND_1);
	
	// Note: the actual string values for the variables are not important to us in this unit test.
	// Since the behaviour is different for the different commands and environments,
	// we can't use the VAR_*.m_updatedValue fields for checking the values. So we will not do the command simulations here.
	// The approach in this test case is the following: we simulate text config by feeding the 
	// text to the parser, as well as building up the expected state for the variable managers for each
	// of the environments.
	// In the end we will check that the variable managers from the config parsing are equivalent
	// to the ones, which are manually built (and should be equal by construction).
	// If the parsing code does not cover any situations, we will get a mismatch and unit test failure.
	// Also here we will test all the errors, which could happen during parsing.
	auto const VAR_0 = ht::MyVariableTestDetails{ht::getUniqueIdString(), ht::getUniqueIdString()};
	auto const VAR_1 = ht::MyVariableTestDetails{ht::getUniqueIdString(), ht::getUniqueIdString()};
	auto const VAR_2 = ht::MyVariableTestDetails{ht::getUniqueIdString(), ht::getUniqueIdString()};
	auto const VAR_3 = ht::MyVariableTestDetails{ht::getUniqueIdString(), ht::getUniqueIdString()};
	auto const VAR_4 = ht::MyVariableTestDetails{ht::getUniqueIdString(), ht::getUniqueIdString()};
	
	// These variables managers are supposed to mirror the ones created from the text configuration
	auto variablesManagerRefForEnv0 = hat::core::VariablesManager{};
	auto variablesManagerRefForEnv1 = hat::core::VariablesManager{};
	std::stringstream basicConfigFileContents;

	// Several helper lamdas, which will facilitate creation of the configuration file text, as well as the reference variable managers:
	auto isEnvEnabled_0 = [](ht::EnvironmentsStringWrapper const & environmentsSetup)
		{ return ((ALL_ENVS == environmentsSetup) || (ENV_0 == environmentsSetup));};
	auto isEnvEnabled_1 = [](ht::EnvironmentsStringWrapper const & environmentsSetup)
		{ return ((ALL_ENVS == environmentsSetup) || (ENV_1 == environmentsSetup));};

	auto util_defineVariable = [&basicConfigFileContents, &variablesManagerRefForEnv0, &variablesManagerRefForEnv1] (
		ht::MyVariableTestDetails const & toDefine)
	{
		basicConfigFileContents << ht::define_var(toDefine.id) << "\n";
		variablesManagerRefForEnv0.declareVariable(toDefine.id);
		variablesManagerRefForEnv1.declareVariable(toDefine.id);
	};

	auto util_setVariableInitialValue = 
		[	&basicConfigFileContents,
			&variablesManagerRefForEnv0, &variablesManagerRefForEnv1, 
			&isEnvEnabled_0, &isEnvEnabled_1] (
		ht::MyVariableTestDetails const & variable,
		ht::EnvironmentsStringWrapper const & environmentsSetup)
	{
		basicConfigFileContents << ht::set_initial_value(variable, environmentsSetup) << "\n";
		if (isEnvEnabled_0(environmentsSetup)) {
			variablesManagerRefForEnv0.setVariableInitialValue(variable.id, variable.m_initialValue);
		}
		if (isEnvEnabled_1(environmentsSetup)) {
			variablesManagerRefForEnv1.setVariableInitialValue(variable.id, variable.m_initialValue);
		}
	};
	
	auto util_addVariableOperationTriggeredByCommand = 
		[	&basicConfigFileContents,
			&variablesManagerRefForEnv0, &variablesManagerRefForEnv1,
			&isEnvEnabled_0, &isEnvEnabled_1, &initialContainer] (
		hat::core::CommandID const & triggerCommand,
		ht::EnvironmentsStringWrapper const & environmentsSetup,
		VariableOperationLogic const & encapsulatedLogic)
	{
		basicConfigFileContents << encapsulatedLogic.getConfigFileTextForOperation(triggerCommand, environmentsSetup) << "\n";
		
		auto commandIndex = initialContainer.getCommandIndex(triggerCommand);
		if (isEnvEnabled_0(environmentsSetup)) {
			encapsulatedLogic.addOperationToRealManager(commandIndex, variablesManagerRefForEnv0);
		}
		if (isEnvEnabled_1(environmentsSetup)) {
			encapsulatedLogic.addOperationToRealManager(commandIndex, variablesManagerRefForEnv1);
		}
	};
	// -- end of helper lambdas section --

	// Start building up the config file text + reference variables managers:
	util_defineVariable(VAR_0);
	util_setVariableInitialValue(VAR_0, ALL_ENVS);

	util_defineVariable(VAR_1);
	util_setVariableInitialValue(VAR_1, ENV_0);

	util_defineVariable(VAR_2);
	util_setVariableInitialValue(VAR_2, ENV_1);
	
	util_defineVariable(VAR_3);
	util_defineVariable(VAR_4);
	
	util_addVariableOperationTriggeredByCommand(COMMAND_0, ALL_ENVS, AssignTextLogic::createFrom(VAR_0));
	util_addVariableOperationTriggeredByCommand(COMMAND_1, ENV_0,    AssignTextLogic::createFrom(VAR_0));
	util_addVariableOperationTriggeredByCommand(COMMAND_2, ENV_1,    AssignTextLogic::createFrom(VAR_0));
	
	util_addVariableOperationTriggeredByCommand(COMMAND_0, ALL_ENVS, AppendTextLogic{VAR_1.id, VAR_1.m_updatedValue});
	util_addVariableOperationTriggeredByCommand(COMMAND_1, ENV_0,    AppendTextLogic{VAR_1.id, VAR_1.m_updatedValue});
	util_addVariableOperationTriggeredByCommand(COMMAND_2, ENV_1,    AppendTextLogic{VAR_1.id, VAR_1.m_updatedValue});

	util_addVariableOperationTriggeredByCommand(COMMAND_0, ALL_ENVS, AssignValueLogic{VAR_2.id, VAR_1.id});
	util_addVariableOperationTriggeredByCommand(COMMAND_1, ENV_0,    AssignValueLogic{VAR_2.id, VAR_1.id});
	util_addVariableOperationTriggeredByCommand(COMMAND_2, ENV_1,    AssignValueLogic{VAR_2.id, VAR_1.id});

	util_addVariableOperationTriggeredByCommand(COMMAND_0, ALL_ENVS, AppendValueLogic{VAR_3.id, VAR_1.id});
	util_addVariableOperationTriggeredByCommand(COMMAND_1, ENV_0,    AppendValueLogic{VAR_3.id, VAR_1.id});
	util_addVariableOperationTriggeredByCommand(COMMAND_2, ENV_1,    AppendValueLogic{VAR_3.id, VAR_1.id});

	util_addVariableOperationTriggeredByCommand(COMMAND_0, ALL_ENVS, ClearTailCharactersLogic{VAR_4.id, 1});
	util_addVariableOperationTriggeredByCommand(COMMAND_1, ENV_0,    ClearTailCharactersLogic{VAR_4.id, 20});
	util_addVariableOperationTriggeredByCommand(COMMAND_2, ENV_1,    ClearTailCharactersLogic{VAR_4.id, 300});

	// Parse the config file text:
	auto contaierAfterCorrectConfigParsing = ht::simulateUI_variablesOperationsConfigParsing(basicConfigFileContents.str(), initialContainer);

	// Check that the reference variable managers are equal to the ones created by parser:
	REQUIRE(variablesManagerRefForEnv0 == contaierAfterCorrectConfigParsing.getVariablesManagers().getManagerForEnv_c(0));
	REQUIRE(variablesManagerRefForEnv1 == contaierAfterCorrectConfigParsing.getVariablesManagers().getManagerForEnv_c(1));
	REQUIRE_FALSE(variablesManagerRefForEnv0 == variablesManagerRefForEnv1); // sanity check - we ensure that the behaviours are indeed defferent for different envs, because the previous two checks will work correctly with default-constructed objects.

	// Check that exceptions are thrown in case of ill-formed configuration file lines:

	// Simple lambda to avoid code duplication:
	auto checkEachConfigLineThrowsException = [&](std::vector<std::string> const & erroneousConfigLines)
	{
		for (auto & invalidConfigString : erroneousConfigLines) {
			// TODO: add exception matcher to this requirement (use REQUIRE_THROWS_MATCHES here)
			REQUIRE_THROWS(
					ht::simulateUI_variablesOperationsConfigParsing(
						invalidConfigString, contaierAfterCorrectConfigParsing);
			);
		}
	};
	auto const THROWAWAY_STRING_PARAM = std::string{"any string"};
	auto const THROWAWAY_SIZE_T_PARAM = size_t{10};

	WHEN ("we try to add an operation, which references unknown variable id [both main, or parameter]") {
		auto const unknownVarId_causingException = hat::core::VariableID{ht::getUniqueIdString()};

		//each entry in the list below has exactly one parameter, which causes an exception during parsing (hence, the odd formatting):
		auto const LIST_OF_INVALID_CONFIG_ENTRIES = std::vector<std::string> {
			  ht::set_initial_value                         (  unknownVarId_causingException  , ALL_ENVS, THROWAWAY_STRING_PARAM)
			, ht::append_text                     (COMMAND_0,  unknownVarId_causingException  , ALL_ENVS, THROWAWAY_STRING_PARAM)
			, ht::assign_text                     (COMMAND_0,  unknownVarId_causingException  , ALL_ENVS, THROWAWAY_STRING_PARAM)
			, ht::append_value                    (COMMAND_0,  unknownVarId_causingException  , ALL_ENVS, VAR_0.id)
			, ht::assign_value                    (COMMAND_0,  unknownVarId_causingException  , ALL_ENVS, VAR_0.id)
			, ht::append_value(COMMAND_0, VAR_0.id, ALL_ENVS,  unknownVarId_causingException  )
			, ht::assign_value(COMMAND_0, VAR_0.id, ALL_ENVS,  unknownVarId_causingException  )
			, ht::clear_last_chars                (COMMAND_0,  unknownVarId_causingException  , ALL_ENVS, THROWAWAY_SIZE_T_PARAM)
		};

		THEN("exception is thrown") {
			checkEachConfigLineThrowsException(LIST_OF_INVALID_CONFIG_ENTRIES);
		}
	}
	WHEN ("we try to add an operation, which references the undefined command") {
		auto const LIST_OF_INVALID_CONFIG_ENTRIES = std::vector<std::string>{
			  ht::append_text      (UNUSED_COMMAND,    VAR_0.id, ALL_ENVS, THROWAWAY_STRING_PARAM)
			, ht::assign_text      (UNUSED_COMMAND,    VAR_0.id, ALL_ENVS, THROWAWAY_STRING_PARAM)
			, ht::append_value     (UNUSED_COMMAND,    VAR_0.id, ALL_ENVS, VAR_1.id)
			, ht::assign_value     (UNUSED_COMMAND,    VAR_0.id, ALL_ENVS, VAR_1.id)
			, ht::clear_last_chars (UNUSED_COMMAND,    VAR_0.id, ALL_ENVS, THROWAWAY_SIZE_T_PARAM)
		};
		THEN("exception is thrown") {
			checkEachConfigLineThrowsException(LIST_OF_INVALID_CONFIG_ENTRIES);
		}
	}
	WHEN ("we try to add an operation, which is not enabled for any environment") {
		auto const LIST_OF_INVALID_CONFIG_ENTRIES = std::vector<std::string>{
			  ht::append_text      (COMMAND_0, VAR_0.id,     NO_ENVS     , THROWAWAY_STRING_PARAM)
			, ht::assign_text      (COMMAND_0, VAR_0.id,     NO_ENVS     , THROWAWAY_STRING_PARAM)
			, ht::append_value     (COMMAND_0, VAR_0.id,     NO_ENVS     , VAR_1.id)
			, ht::assign_value     (COMMAND_0, VAR_0.id,     NO_ENVS     , VAR_1.id)
			, ht::clear_last_chars (COMMAND_0, VAR_0.id,     NO_ENVS     , THROWAWAY_SIZE_T_PARAM)
		};
		THEN("exception is thrown") {
			checkEachConfigLineThrowsException(LIST_OF_INVALID_CONFIG_ENTRIES);
		}
	}
	WHEN ("we try to add a definition for the already defined variable") {
		auto const LIST_OF_INVALID_CONFIG_ENTRIES = std::vector<std::string>{
			ht::define_var(VAR_0.id)
		};
		THEN("exception is thrown") {
			checkEachConfigLineThrowsException(LIST_OF_INVALID_CONFIG_ENTRIES);
		}
	}
	WHEN ("we try to add a variable, with not allowed symbols in the id string") {
		auto idStringWithNotAllowedSymbols = std::string{"abc#$%"};
		auto const LIST_OF_INVALID_CONFIG_ENTRIES = std::vector<std::string>{
			ht::define_var_unsafe (idStringWithNotAllowedSymbols)
		};
		THEN("exception is thrown") {
			checkEachConfigLineThrowsException(LIST_OF_INVALID_CONFIG_ENTRIES);
		}
	}
	WHEN ("we try to add a backspace operation with ill-formed characters count string") {
		auto illFormedUint = std::string{"abc"};
		auto illFormedUint2 = std::string{"-123"}; // the number should be unsigned
		auto const LIST_OF_INVALID_CONFIG_ENTRIES = std::vector<std::string>{
			  ht::clear_last_chars_unsafe(
				COMMAND_0.getValue(), VAR_0.id.getValue(), ALL_ENVS.getValue(), illFormedUint)
			, ht::clear_last_chars_unsafe(
				COMMAND_0.getValue(), VAR_0.id.getValue(), ALL_ENVS.getValue(), illFormedUint2)
		};
		
		THEN("exception is thrown") {
			checkEachConfigLineThrowsException(LIST_OF_INVALID_CONFIG_ENTRIES);
		}
	}
}
