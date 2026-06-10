#ifndef TYPE_SYSTEM_H
#define TYPE_SYSTEM_H

#include <string>

// Pascal subset type names used in the symbol table and semantic analysis.
namespace TypeNames {
inline constexpr const char* INTEGER = "integer";
inline constexpr const char* REAL = "real";
inline constexpr const char* BOOLEAN = "boolean";
inline constexpr const char* UNKNOWN = "unknown";
}  // namespace TypeNames

namespace TypeSystem {

bool isInteger(const std::string& type);
bool isReal(const std::string& type);
bool isBoolean(const std::string& type);
bool isNumeric(const std::string& type);
bool isArrayType(const std::string& typeName);
std::string arrayElementType(const std::string& arrayType);

bool isRelationalOp(const std::string& op);
bool isAdditiveOp(const std::string& op);
bool isMultiplicativeOp(const std::string& op);

// Arithmetic keeps integer results unless one operand is real.
std::string resultOfArithmetic(const std::string& left, const std::string& right);

// Relational operators on numeric operands return boolean.
std::string resultOfRelational(const std::string& left, const std::string& right);

// Unary plus/minus preserve numeric type; NOT requires boolean operand.
std::string resultOfUnary(const std::string& op, const std::string& operand);

// Assignment allows integer to real, but rejects real to integer.
bool assignmentCompatible(const std::string& lhs, const std::string& rhs);

// Call arguments use the same rules as assignment to formals.
bool argumentCompatible(const std::string& formal, const std::string& actual);

// Conditions in IF/WHILE: boolean or integer (Pascal-style truthiness).
bool conditionCompatible(const std::string& type);

}  // namespace TypeSystem

#endif
