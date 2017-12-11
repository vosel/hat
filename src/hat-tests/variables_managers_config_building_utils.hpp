#ifndef	UI_TEXT_VARIABLES_CONFIG_BUILDING_UTILS
#define UI_TEXT_VARIABLES_CONFIG_BUILDING_UTILS

#include "../hat-core/commands_data_extraction.hpp"
#include "variables_manager_testing_utils.hpp"


namespace hat {
namespace test {

//This wrapper is used for better type safety in the testing code
struct EnvironmentsStringWrapper {
	explicit EnvironmentsStringWrapper(std::string const & data) : m_value(data) {}
	std::string const & getValue() const { return m_value; };
	bool operator == (EnvironmentsStringWrapper const & other) const { return m_value == other.m_value; }
private:
	std::string m_value;
};

// Utility functions for defining config file lines:
// Note: the 'unsafe' versions of the functions are useful in tests for testing the errors.
// In testing the normal situations, the type-safe versions of the functions should be used.
std::string define_var_unsafe(std::string const & id_str);
std::string define_var(hat::core::VariableID const & id);

std::string set_initial_value_unsafe(std::string const & id_str, std::string const & environments_str, std::string const & value);
std::string set_initial_value(hat::core::VariableID const & id, EnvironmentsStringWrapper const & environments, std::string const & value);
std::string set_initial_value(hat::test::MyVariableTestDetails const & variable, EnvironmentsStringWrapper const & environments);

std::string append_text_unsafe(std::string const & command_id_str, std::string const & id_str, std::string const & environments_str, std::string const & value);
std::string append_text(hat::core::CommandID const & triggerCommand, hat::core::VariableID const & id, EnvironmentsStringWrapper const & environments, std::string const & value);

// Note: use this function with caution. Here we assume that the user understands that the 'variable' passed here should represent the situation rights before and after the 'append' operation. Otherwise, it could yeild unexpected results.
std::string append_text(hat::core::CommandID const & triggerCommand, hat::test::MyVariableTestDetails const & variable, EnvironmentsStringWrapper const & environments);

std::string assign_text_unsafe(std::string const & command_id_str, std::string const & id_str, std::string const & environments_str, std::string const & value);
std::string assign_text(hat::core::CommandID const & triggerCommand, hat::core::VariableID const & id, EnvironmentsStringWrapper const & environments, std::string const & value);
std::string assign_text(hat::core::CommandID const & triggerCommand, hat::test::MyVariableTestDetails const & variable, EnvironmentsStringWrapper const & environments);

std::string append_value_unsafe(std::string const & command_id_str, std::string const & id_str, std::string const & environments_str, std::string const & data_source_variable_id_str);
std::string assign_value_unsafe(std::string const & command_id_str, std::string const & id_str, std::string const & environments_str, std::string const & data_source_variable_id_str);
std::string append_value(hat::core::CommandID const & triggerCommand, hat::core::VariableID const & id, EnvironmentsStringWrapper const & environments, hat::core::VariableID const & data_source_variable_id);
std::string assign_value(hat::core::CommandID const & triggerCommand, hat::core::VariableID const & id, EnvironmentsStringWrapper const & environments, hat::core::VariableID const & data_source_variable_id);

std::string clear_last_chars_unsafe(std::string const & command_id_str, std::string const & id_str, std::string const & environments_str, std::string const & charactersCount);
std::string clear_last_chars(hat::core::CommandID const & triggerCommand, hat::core::VariableID const & id, EnvironmentsStringWrapper const & environments, size_t charactersCount);

}// namespace test
}// namespace hat

#endif //UI_TEXT_VARIABLES_CONFIG_BUILDING_UTILS