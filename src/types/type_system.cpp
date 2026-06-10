#include "type_system.h"

namespace TypeSystem {

bool isInteger(const std::string& type) { return type == TypeNames::INTEGER; }

bool isReal(const std::string& type) { return type == TypeNames::REAL; }

bool isBoolean(const std::string& type) { return type == TypeNames::BOOLEAN; }

bool isNumeric(const std::string& type) { return isInteger(type) || isReal(type); }

bool isArrayType(const std::string& typeName) { return typeName.rfind("array", 0) == 0; }

std::string arrayElementType(const std::string& arrayType) {
    const std::string marker = " of ";
    const size_t pos = arrayType.rfind(marker);
    if (pos == std::string::npos) {
        return TypeNames::UNKNOWN;
    }
    return arrayType.substr(pos + marker.size());
}

bool isRelationalOp(const std::string& op) {
    return op == "=" || op == "<>" || op == "<" || op == "<=" || op == ">" || op == ">=";
}

bool isAdditiveOp(const std::string& op) { return op == "+" || op == "-"; }

bool isMultiplicativeOp(const std::string& op) { return op == "*" || op == "/"; }

std::string resultOfArithmetic(const std::string& left, const std::string& right) {
    if (!isNumeric(left) || !isNumeric(right)) {
        return TypeNames::UNKNOWN;
    }
    if (isReal(left) || isReal(right)) {
        return TypeNames::REAL;
    }
    return TypeNames::INTEGER;
}

std::string resultOfRelational(const std::string& left, const std::string& right) {
    if (!isNumeric(left) || !isNumeric(right)) {
        return TypeNames::UNKNOWN;
    }
    return TypeNames::BOOLEAN;
}

std::string resultOfUnary(const std::string& op, const std::string& operand) {
    if (op == "not") {
        return isBoolean(operand) ? TypeNames::BOOLEAN : TypeNames::UNKNOWN;
    }
    if (op == "+" || op == "-") {
        return isNumeric(operand) ? operand : TypeNames::UNKNOWN;
    }
    return TypeNames::UNKNOWN;
}

bool assignmentCompatible(const std::string& lhs, const std::string& rhs) {
    if (lhs == TypeNames::UNKNOWN || rhs == TypeNames::UNKNOWN) {
        return true;
    }
    if (isInteger(lhs)) {
        return isInteger(rhs);
    }
    if (isReal(lhs)) {
        return isInteger(rhs) || isReal(rhs);
    }
    return false;
}

bool argumentCompatible(const std::string& formal, const std::string& actual) {
    return assignmentCompatible(formal, actual);
}

bool conditionCompatible(const std::string& type) {
    return isBoolean(type) || isInteger(type);
}

}  // namespace TypeSystem
