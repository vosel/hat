// This source file is part of the 'hat' open source project.
// Copyright (c) 2017, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#include "variables_managers_config_building_utils.hpp"

namespace {
//This is a function for removing code duplication for all the 'assign-' and 'append-' functions. It should not be used outside of this translation unit.
std::string util_two_string_operation_on_command_unsafe(std::string const & operationStringId, std::string const & command_id_str, std::string const & environments_str, std::string const & id_str, std::string const & value)
{
	std::stringstream data;
	data << operationStringId << "\t" << environments_str << "\t" << command_id_str << "\t" << id_str << "\t" << value;
	return data.str();
}
}

namespace hat {
namespace test {

// Utility functions for defining config file lines:
// Note: the 'unsafe' versions of the functions are useful in tests for testing the errors.
// In testing the normal situations, the type-safe versions of the functions should be used.
std::string define_var_unsafe(std::string const & id_str)
{
	std::stringstream data;
	data << hat::core::ConfigFilesKeywords::defineInternalLabelVariable() << "\t" << id_str;
	return data.str();
}

std::string define_var(hat::core::VariableID const & id)
{
	return define_var_unsafe(id.getValue());
}

std::string set_initial_value_unsafe(
	std::string const & id_str,
	std::string const & environments_str,
	std::string const & value)
{
	std::stringstream data;
	data << hat::core::ConfigFilesKeywords::textFeedbackSetInitialValue()
		<< "\t" << environments_str << "\t" << id_str << "\t" << value;
	return data.str();
}

std::string set_initial_value(
	hat::core::VariableID const & id,
	EnvironmentsStringWrapper const & environments,
	std::string const & value)
{
	return set_initial_value_unsafe(id.getValue(), environments.getValue(), value);
}

std::string set_initial_value(
	hat::test::MyVariableTestDetails const & variable,
	EnvironmentsStringWrapper const & environments)
{
	return set_initial_value_unsafe(variable.id.getValue(), environments.getValue(), variable.m_initialValue);
}

std::string append_text_unsafe(std::string const & command_id_str, std::string const & id_str, std::string const & environments_str, std::string const & value)
{
	return util_two_string_operation_on_command_unsafe(
		hat::core::ConfigFilesKeywords::textFeedbackAppendValue(), command_id_str, environments_str, id_str, value);
}

std::string append_text(hat::core::CommandID const & triggerCommand, hat::core::VariableID const & id, EnvironmentsStringWrapper const & environments, std::string const & value)
{
	return append_text_unsafe(triggerCommand.getValue(), id.getValue(), environments.getValue(), value);
}

std::string append_text(hat::core::CommandID const & triggerCommand, hat::test::MyVariableTestDetails const & variable, EnvironmentsStringWrapper const & environments)
{
	auto calculatedStringToAppend = variable.m_updatedValue.substr(variable.m_initialValue.size());
	return append_text_unsafe(triggerCommand.getValue(), variable.id.getValue(), environments.getValue(), calculatedStringToAppend);
}

std::string assign_text_unsafe(std::string const & command_id_str, std::string const & id_str, std::string const & environments_str, std::string const & value)
{
	return util_two_string_operation_on_command_unsafe(
		hat::core::ConfigFilesKeywords::textFeedbackAssignValue(), command_id_str, environments_str, id_str, value);
}

std::string assign_text(hat::core::CommandID const & triggerCommand, hat::core::VariableID const & id, EnvironmentsStringWrapper const & environments, std::string const & value)
{
	return assign_text_unsafe(triggerCommand.getValue(), id.getValue(), environments.getValue(), value);
}

std::string assign_text(hat::core::CommandID const & triggerCommand, hat::test::MyVariableTestDetails const & variable, EnvironmentsStringWrapper const & environments)
{
	//Note: in this situation, we can assume that the caller wants to use the value from variable's 'updated value' field as a parameter for the operation.
	return assign_text_unsafe(triggerCommand.getValue(), variable.id.getValue(), environments.getValue(), variable.m_updatedValue);
}

std::string append_value_unsafe(std::string const & command_id_str, std::string const & id_str, std::string const & environments_str, std::string const & data_source_variable_id_str)
{
	return util_two_string_operation_on_command_unsafe(
		hat::core::ConfigFilesKeywords::textFeedbackAppendFromAnotherVariable(), command_id_str, environments_str, id_str, data_source_variable_id_str);
}

std::string append_value(hat::core::CommandID const & triggerCommand, hat::core::VariableID const & id, EnvironmentsStringWrapper const & environments, hat::core::VariableID const & data_source_variable_id)
{
	return append_value_unsafe(triggerCommand.getValue(), id.getValue(), environments.getValue(), data_source_variable_id.getValue());
}

std::string assign_value_unsafe(std::string const & command_id_str, std::string const & id_str, std::string const & environments_str, std::string const & data_source_variable_id_str)
{
	return util_two_string_operation_on_command_unsafe(
		hat::core::ConfigFilesKeywords::textFeedbackAssignFromAnotherVariable(), command_id_str, environments_str, id_str, data_source_variable_id_str);
}

std::string assign_value(hat::core::CommandID const & triggerCommand, hat::core::VariableID const & id, EnvironmentsStringWrapper const & environments, hat::core::VariableID const & data_source_variable_id)
{
	return assign_value_unsafe(triggerCommand.getValue(), id.getValue(), environments.getValue(), data_source_variable_id.getValue());
}

std::string clear_last_chars_unsafe(std::string const & command_id_str, std::string const & id_str, std::string const & environments_str, std::string const & charactersCount)
{
	std::stringstream data;
	data << hat::core::ConfigFilesKeywords::textFeedbackClearLastCharacter() << "\t" << environments_str << "\t" << command_id_str << "\t" << id_str << "\t" << charactersCount;
	return data.str();
}

std::string clear_last_chars(hat::core::CommandID const & triggerCommand, hat::core::VariableID const & id, EnvironmentsStringWrapper const & environments, size_t charactersCount)
{
	std::stringstream charactersCountStream;
	charactersCountStream << charactersCount; //This is done here because we want to pass std::string to the 'unsafe' version of the funtion. The unsafe version needs string because we want it to be able to create ill-formed results (for unit testing of the error-handling code).
	return clear_last_chars_unsafe(triggerCommand.getValue(), id.getValue(), environments.getValue(), charactersCountStream.str());
}

}// namespace test
}// namespace hat
