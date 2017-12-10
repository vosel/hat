// This source file is part of the 'hat' open source project.
// Copyright (c) 2017, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef HAT_CORE_HEADERONLY_MODE
#include "variables_manager.hpp"
#endif
#include <sstream>
#include <algorithm>
#include <iterator>

#ifndef HAT_CORE_HEADERONLY_MODE
#define LINKAGE_RESTRICTION 
#else
#define LINKAGE_RESTRICTION inline
#endif

namespace hat {
namespace core {

namespace {
	// Simple helper function in order to avoid the code duplication.
	void throwIfTryingToAssignToUnknownVariable(VariablesManager const & manager, VariableID const & variableID)
	{
		if (!manager.variableExists(variableID)) {
			std::stringstream error;
			error << "Trying to set value to undeclared variable. Please make sure that all the variables are declared before they are assigned to.";
			error << " VariableID: " << variableID.getValue();
			throw std::runtime_error(error.str());
		}
	}
}

	LINKAGE_RESTRICTION void VariablesManager::declareVariable(VariableID const & variableID)
	{
		if (variableExists(variableID)) {
			std::stringstream error;
			error << "Variable re-declaration. Please make sure that all the variables are declared only once.";
			error << " VariableID: " << variableID.getValue();
			throw std::runtime_error(error.str());
		}
		m_variablesWithValues[variableID] = "";
	}
	
	LINKAGE_RESTRICTION bool VariablesManager::variableExists(VariableID const & variableID) const
	{
		return m_variablesWithValues.find(variableID) != m_variablesWithValues.end();
	}

	LINKAGE_RESTRICTION void VariablesManager::setVariableInitialValue(VariableID const & variableID, std::string const & value)
	{
		throwIfTryingToAssignToUnknownVariable(*this, variableID);
		//if (m_variablesWithValues[variableID].size() > 0) {
		//	//TODO: maybe report a warning to the user here - overwriting the information, which has no chance to be used
		//}
		m_variablesWithValues[variableID] = value;
	}

	LINKAGE_RESTRICTION std::string VariablesManager::getValue(VariableID const & variableID) const
	{
		return m_variablesWithValues.at(variableID);
	}

	LINKAGE_RESTRICTION void VariablesManager::addOperationToExecuteOnCommand(size_t commandIndex, std::shared_ptr<VariableOperation> operation)
	{
		m_operationsToExecute[commandIndex].data().push_back(operation);
	}

	LINKAGE_RESTRICTION void VariablesManager::updateValue(VariableID const & targetVariableID, std::string const & newValue)
	{
		throwIfTryingToAssignToUnknownVariable(*this, targetVariableID);
		if (m_trackVariablesUpdates) {
			m_updatedVariables.insert(targetVariableID);
		}
		m_variablesWithValues[targetVariableID] = newValue;
	}

	LINKAGE_RESTRICTION std::vector<VariableID> VariablesManager::executeCommandAndGetChangedVariablesList(size_t triggeredCommandIndex)
	{
		auto operationsListToExecute = m_operationsToExecute[triggeredCommandIndex];
		for (auto & operation : operationsListToExecute.data()) {
			operation->preformOperation(*this);
		}
		std::vector<VariableID> result;
		result.reserve(m_updatedVariables.size());
		std::copy(std::begin(m_updatedVariables), std::end(m_updatedVariables), std::back_inserter(result));
		m_updatedVariables.clear();
		return result;
	}
	
	LINKAGE_RESTRICTION bool OperationsList::operator == (OperationsList const & other) const
	{
		if (m_data.size() == other.m_data.size()) {
			for (size_t i = 0; i < m_data.size(); ++i) {
				auto const & myOperation = *(m_data[i]);
				auto const & otherOperation = *(other.m_data[i]);
				if (!(myOperation == otherOperation)) {
					return false;
				}
			}
			return true;
		}
		return false;
	}

	LINKAGE_RESTRICTION bool VariablesManager::operator == (VariablesManager const & other) const
	{
		return (m_trackVariablesUpdates == other.m_trackVariablesUpdates)
			&& (m_updatedVariables == other.m_updatedVariables)
			&& (m_variablesWithValues == other.m_variablesWithValues)
			&& (m_operationsToExecute == other.m_operationsToExecute);
	}

	LINKAGE_RESTRICTION void AppendText::preformOperation(VariablesManager & targetVariablesManager) {
		auto newValue = targetVariablesManager.getValue(m_targetVariable) + m_stringValue;
		targetVariablesManager.updateValue(m_targetVariable, newValue);
	}
	
	LINKAGE_RESTRICTION void AssignText::preformOperation(VariablesManager & targetVariablesManager) {
		targetVariablesManager.updateValue(m_targetVariable, m_stringValue);
	}

	LINKAGE_RESTRICTION void AppendValue::preformOperation(VariablesManager & targetVariablesManager) {
		auto newValue = targetVariablesManager.getValue(m_targetVariable) + targetVariablesManager.getValue(m_sourceVariableID);
		targetVariablesManager.updateValue(m_targetVariable, newValue);
	}

	LINKAGE_RESTRICTION void AssignValue::preformOperation(VariablesManager & targetVariablesManager) {
		targetVariablesManager.updateValue(m_targetVariable, targetVariablesManager.getValue(m_sourceVariableID));
	}

	LINKAGE_RESTRICTION void ClearTailCharacters::preformOperation(VariablesManager & targetVariablesManager)
	{
		auto currentValue = targetVariablesManager.getValue(m_targetVariable);
		if (currentValue.size() <= m_charactersCountToClear) {
			targetVariablesManager.updateValue(m_targetVariable, "");
		} else {
			size_t const newLengthOfValueString = currentValue.size() - m_charactersCountToClear; // Note: the result of this operation is always > 0 here (see the if statements's condition)
			targetVariablesManager.updateValue(m_targetVariable, currentValue.substr(0, newLengthOfValueString));
		}
	}

	LINKAGE_RESTRICTION bool AppendText::operator == (AppendText const & other) const
	{
		return (other.m_stringValue == m_stringValue) && (other.m_targetVariable == m_targetVariable);
	}

	LINKAGE_RESTRICTION bool AssignText::operator == (AssignText const & other) const
	{
		return (other.m_stringValue == m_stringValue) && (other.m_targetVariable == m_targetVariable);
	}

	LINKAGE_RESTRICTION bool AppendValue::operator == (AppendValue const & other) const
	{
		return (other.m_sourceVariableID == m_sourceVariableID) && (other.m_targetVariable == m_targetVariable);
	}

	LINKAGE_RESTRICTION bool AssignValue::operator == (AssignValue const & other) const
	{
		return (other.m_sourceVariableID == m_sourceVariableID) && (other.m_targetVariable == m_targetVariable);
	}

	LINKAGE_RESTRICTION bool ClearTailCharacters::operator == (ClearTailCharacters const & other) const
	{
		return (other.m_charactersCountToClear == m_charactersCountToClear) && (other.m_targetVariable == m_targetVariable);
	}
} // namespace core
} // namespace hat
#undef LINKAGE_RESTRICTION