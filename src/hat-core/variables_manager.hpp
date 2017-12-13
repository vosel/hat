// This source file is part of the 'hat' open source project.
// Copyright (c) 2017, Yuriy Vosel.
// Licensed under Boost Software License.
// See LICENSE.txt for the licence information.

#ifndef _VARIABLES_MANAGER_HPP
#define _VARIABLES_MANAGER_HPP

#include <string>
#include <map>
#include <set>
#include <vector>
#include <memory>
namespace hat {
namespace core {

// Note: this variable_id is a string identifier for the text variable from the user's point of view (currently used in text feedback labels).
// This string is used in the config files. It is not used for the element IDs inside the 'tau' layouts (auto-generated IDs are used there).
class VariableID
{
	std::string m_value;
public:
	VariableID() {};
	explicit VariableID(std::string const & value) : m_value(value) {};
	std::string const & getValue() const { return m_value; }
	bool operator == (VariableID const & other) const { return m_value == other.m_value; }
	static VariableID createFromUserString(std::string const & stringValue);
};

} // namespace core
} // namespace hat

namespace std {
template<> 
struct less<hat::core::VariableID> {
	bool operator()(hat::core::VariableID const & left, hat::core::VariableID const & right) const {
		return std::less<std::string>{}(left.getValue(), right.getValue());
	}
};
} // namespace std

namespace hat {
namespace core {

class VariablesManager;

struct AppendText;
struct AssignText;
struct AppendValue;
struct AssignValue;
struct ClearTailCharacters;

struct VariableOperation {
	virtual void preformOperation(VariablesManager & targetVariablesManager) = 0;
	virtual bool operator == (VariableOperation const & other) const = 0;
	virtual bool operator == (AppendText const & other) const { return false; };
	virtual bool operator == (AssignText const & other) const { return false; };
	virtual bool operator == (AppendValue const & other) const { return false; };
	virtual bool operator == (AssignValue const & other) const { return false; };
	virtual bool operator == (ClearTailCharacters const & other) const { return false; };
};

struct SingleTargetVariableOperation : public VariableOperation{
	VariableID m_targetVariable;
	SingleTargetVariableOperation(VariableID const & targetVariable) : m_targetVariable(targetVariable) {
	}
};

struct VariableUpdateOperationWithConstantParameter : public SingleTargetVariableOperation  {
	std::string m_stringValue;
	VariableUpdateOperationWithConstantParameter(VariableID const & targetVariable, std::string const & stringValue) :
		SingleTargetVariableOperation(targetVariable), m_stringValue(stringValue) {}
};

struct VariableUpdateOperationWithVariableParameter : public SingleTargetVariableOperation  {
	VariableID m_sourceVariableID;
	VariableUpdateOperationWithVariableParameter(VariableID const & targetVariable, VariableID const & variableForSourceValue) :
		SingleTargetVariableOperation(targetVariable), m_sourceVariableID(variableForSourceValue) {}
};

struct AppendText : public VariableUpdateOperationWithConstantParameter {
	AppendText(VariableID const & targetVariable, std::string const & stringValue) :
		VariableUpdateOperationWithConstantParameter(targetVariable, stringValue) {}
	void preformOperation(VariablesManager & targetVariablesManager) override;
	bool operator == (VariableOperation const & other) const override { return other.operator==(*this); }
	bool operator == (AppendText const & other) const override;
};

struct AssignText : public VariableUpdateOperationWithConstantParameter {
	AssignText(VariableID const & targetVariable, std::string const & stringValue) :
		VariableUpdateOperationWithConstantParameter(targetVariable, stringValue) {}
	void preformOperation(VariablesManager & targetVariablesManager) override;
	bool operator == (VariableOperation const & other) const override { return other.operator==(*this); }
	bool operator == (AssignText const & other) const override;
};

struct AppendValue : public VariableUpdateOperationWithVariableParameter {
	AppendValue(VariableID const & targetVariable, VariableID const & sourceVariableID) :
		VariableUpdateOperationWithVariableParameter(targetVariable, sourceVariableID) {}
	void preformOperation(VariablesManager & targetVariablesManager) override;
	bool operator == (VariableOperation const & other) const override { return other.operator==(*this); }
	bool operator == (AppendValue const & other) const override;
};

struct AssignValue : public VariableUpdateOperationWithVariableParameter {
	AssignValue(VariableID const & targetVariable, VariableID const & sourceVariableID) :
		VariableUpdateOperationWithVariableParameter(targetVariable, sourceVariableID) {}
	void preformOperation(VariablesManager & targetVariablesManager) override;
	bool operator == (VariableOperation const & other) const override { return other.operator==(*this); }
	bool operator == (AssignValue const & other) const override;
};

struct ClearTailCharacters : public SingleTargetVariableOperation
{
private:
	size_t m_charactersCountToClear;
public:
	ClearTailCharacters(VariableID const & targetVariable, size_t amountOfCharactersToClear) :
		SingleTargetVariableOperation(targetVariable), m_charactersCountToClear(amountOfCharactersToClear) {}
	void preformOperation(VariablesManager & targetVariablesManager) override;
	bool operator == (VariableOperation const & other) const override { return other.operator==(*this); }
	bool operator == (ClearTailCharacters const & other) const override;
};

// This is just a simple wrapper around std::vector of shared pointers, which overloads the equality comparison (does a deep comparison instead of a shallow one)
struct OperationsList
{
	typedef std::vector<std::shared_ptr<VariableOperation>> InternalData;
private:
	InternalData m_data;
public:
	InternalData & data() { return m_data; };
	bool operator == (OperationsList const & other) const;
};

class VariablesManager {
	bool m_trackVariablesUpdates{ true };
	std::set<VariableID> m_updatedVariables;
	std::map<VariableID, std::string> m_variablesWithValues;
	std::map<size_t, OperationsList> m_operationsToExecute;
public:
	void declareVariable(VariableID const & variableID);
	void setVariableInitialValue(VariableID const & variableID, std::string const & value);
	bool variableExists(VariableID const & variableID) const;

	std::string getValue(VariableID const & variableID) const;
	
	void addOperationToExecuteOnCommand(size_t commandIndex, std::shared_ptr<VariableOperation> operation);

	void updateValue(VariableID const & targetVariableID, std::string const & newValue);
	//TODO: refactoring, optimisation : return std::vector<std::pair<VariableID, std::string>> here. It is easier for the user code - we don't have to query the variables updated values in separate call
	std::vector<VariableID> executeCommandAndGetChangedVariablesList(size_t triggeredCommandIndex);
	bool operator == (VariablesManager const & other) const;
};

} // namespace core
} // namespace hat

#ifdef HAT_CORE_HEADERONLY_MODE
#include "variables_manager.cpp"
#endif //HAT_CORE_HEADERONLY_MODE

#endif //_VARIABLES_MANAGER_HPP