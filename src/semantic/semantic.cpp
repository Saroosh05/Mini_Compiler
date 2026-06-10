#include "semantic.h"

#include "../types/type_system.h"

SemanticAnalyzer::SemanticAnalyzer(SymbolTableManager& symtab, ErrorHandler& errors)
    : symtab_(symtab), errors_(errors) {}

bool SemanticAnalyzer::analyze(AstNode* root) {
    if (!root) {
        return false;
    }
    scopeDepth_ = 0;
    symtab_.setLookupDepth(0);
    checkNode(root);
    return !errors_.hasErrors();
}

void SemanticAnalyzer::enterScope() {
    ++scopeDepth_;
    symtab_.setLookupDepth(scopeDepth_);
}

void SemanticAnalyzer::exitScope() {
    if (scopeDepth_ > 0) {
        --scopeDepth_;
    }
    symtab_.setLookupDepth(scopeDepth_);
}

void SemanticAnalyzer::annotate(AstNode* node, const std::string& type) {
    if (node) {
        node->inferredType = type;
    }
}

void SemanticAnalyzer::checkIdentifierUse(const AstNode* node, const char* context) {
    if (!node) {
        return;
    }
    const auto found = symtab_.lookup(node->label);
    if (!found) {
        errors_.report(ErrorKind::Semantic, node->line, node->column,
                       std::string("undeclared identifier '") + node->label + "' in " + context);
        return;
    }
    if (found->kind == SymbolKind::Procedure && node->kind == AstNodeKind::Variable) {
        errors_.report(ErrorKind::Semantic, node->line, node->column,
                       "procedure '" + node->label + "' used as variable");
    }
}

void SemanticAnalyzer::checkArrayAccess(AstNode* node, const char* context) {
    if (!node) {
        return;
    }
    checkIdentifierUse(node, context);
    const auto sym = symtab_.lookup(node->label);
    if (sym && !TypeSystem::isArrayType(sym->typeName)) {
        errors_.report(ErrorKind::Semantic, node->line, node->column,
                       "identifier '" + node->label + "' is not an array");
    }
    if (node->children.size() > 1) {
        const std::string indexType = inferType(node->children[1].get());
        if (indexType != TypeNames::UNKNOWN && !TypeSystem::isInteger(indexType)) {
            errors_.report(ErrorKind::Semantic, node->children[1]->line,
                           node->children[1]->column,
                           "array index must be integer, got " + indexType);
        }
    }
    const std::string elem =
        sym ? TypeSystem::arrayElementType(sym->typeName) : TypeNames::UNKNOWN;
    annotate(node, elem);
}

std::string SemanticAnalyzer::inferType(AstNode* node) {
    if (!node) {
        return TypeNames::UNKNOWN;
    }

    switch (node->kind) {
        case AstNodeKind::Constant: {
            const std::string t =
                node->label.find('.') != std::string::npos ? TypeNames::REAL : TypeNames::INTEGER;
            annotate(node, t);
            return t;
        }
        case AstNodeKind::Variable: {
            checkIdentifierUse(node, "expression");
            const auto e = symtab_.lookup(node->label);
            if (!e) {
                annotate(node, TypeNames::UNKNOWN);
                return TypeNames::UNKNOWN;
            }
            if (TypeSystem::isArrayType(e->typeName)) {
                errors_.report(ErrorKind::Semantic, node->line, node->column,
                               "array '" + node->label + "' requires subscript");
                annotate(node, TypeNames::UNKNOWN);
                return TypeNames::UNKNOWN;
            }
            annotate(node, e->typeName);
            return e->typeName;
        }
        case AstNodeKind::ArrayAccess:
            checkArrayAccess(node, "expression");
            return node->inferredType;
        case AstNodeKind::FuncCall: {
            checkCall(node, true);
            const auto e = symtab_.lookup(node->label);
            const std::string t = e ? e->typeName : TypeNames::UNKNOWN;
            annotate(node, t);
            return t;
        }
        case AstNodeKind::UnaryOp: {
            const std::string operand =
                node->children.empty() ? TypeNames::UNKNOWN : inferType(node->children[0].get());
            const std::string t = TypeSystem::resultOfUnary(node->label, operand);
            if (node->label == "not" && operand != TypeNames::UNKNOWN &&
                !TypeSystem::isBoolean(operand)) {
                errors_.report(ErrorKind::Semantic, node->line, node->column,
                               "operand of NOT must be boolean");
            }
            annotate(node, t);
            return t;
        }
        case AstNodeKind::BinaryOp: {
            const std::string left =
                node->children.size() > 0 ? inferType(node->children[0].get()) : TypeNames::UNKNOWN;
            const std::string right =
                node->children.size() > 1 ? inferType(node->children[1].get()) : TypeNames::UNKNOWN;
            std::string t = TypeNames::UNKNOWN;
            if (TypeSystem::isRelationalOp(node->label)) {
                t = TypeSystem::resultOfRelational(left, right);
                if (left != TypeNames::UNKNOWN && right != TypeNames::UNKNOWN &&
                    !TypeSystem::isNumeric(left)) {
                    errors_.report(ErrorKind::Semantic, node->line, node->column,
                                   "relational operator requires numeric operands");
                }
            } else if (TypeSystem::isAdditiveOp(node->label) ||
                       TypeSystem::isMultiplicativeOp(node->label)) {
                t = TypeSystem::resultOfArithmetic(left, right);
                if (left != TypeNames::UNKNOWN && right != TypeNames::UNKNOWN &&
                    (!TypeSystem::isNumeric(left) || !TypeSystem::isNumeric(right))) {
                    errors_.report(ErrorKind::Semantic, node->line, node->column,
                                   "arithmetic operator requires numeric operands");
                }
            }
            annotate(node, t);
            return t;
        }
        default:
            annotate(node, TypeNames::UNKNOWN);
            return TypeNames::UNKNOWN;
    }
}

void SemanticAnalyzer::checkCondition(AstNode* node, const char* context) {
    if (!node) {
        return;
    }
    const std::string t = inferType(node);
    if (t != TypeNames::UNKNOWN && !TypeSystem::conditionCompatible(t)) {
        errors_.report(ErrorKind::Semantic, node->line, node->column,
                       std::string("invalid ") + context + " condition type: " + t);
    }
}

void SemanticAnalyzer::checkAssignment(AstNode* node) {
    if (!node || node->children.size() < 2) {
        return;
    }
    AstNode* lhs = node->children[0].get();
    AstNode* rhs = node->children[1].get();

    std::string lt = TypeNames::UNKNOWN;

    if (lhs->kind == AstNodeKind::Variable) {
        checkIdentifierUse(lhs, "assignment");
        const auto sym = symtab_.lookup(lhs->label);
        if (sym && sym->kind == SymbolKind::Procedure) {
            errors_.report(ErrorKind::Semantic, lhs->line, lhs->column,
                           "cannot assign to procedure '" + lhs->label + "'");
        }
        if (sym && TypeSystem::isArrayType(sym->typeName)) {
            errors_.report(ErrorKind::Semantic, lhs->line, lhs->column,
                           "array '" + lhs->label + "' requires subscript");
            annotate(lhs, TypeNames::UNKNOWN);
            lt = TypeNames::UNKNOWN;
        } else {
            lt = inferType(lhs);
        }
    } else if (lhs->kind == AstNodeKind::ArrayAccess) {
        checkArrayAccess(lhs, "array assignment");
        lt = lhs->inferredType;
    } else {
        lt = inferType(lhs);
    }

    const std::string rt = inferType(rhs);

    if (lt != TypeNames::UNKNOWN && rt != TypeNames::UNKNOWN &&
        !TypeSystem::assignmentCompatible(lt, rt)) {
        errors_.report(ErrorKind::Semantic, node->line, node->column,
                       "type mismatch in assignment: " + lt + " := " + rt);
    }
    annotate(node, lt);
}

void SemanticAnalyzer::checkCall(AstNode* node, bool asFunction) {
    if (!node) {
        return;
    }
    const auto sym = symtab_.lookup(node->label);
    if (!sym) {
        errors_.report(ErrorKind::Semantic, node->line, node->column,
                       "undeclared identifier '" + node->label + "'");
        return;
    }

    if (asFunction) {
        if (sym->kind == SymbolKind::Procedure) {
            errors_.report(ErrorKind::Semantic, node->line, node->column,
                           "procedure '" + node->label + "' cannot be used in an expression");
        } else if (sym->kind != SymbolKind::Function) {
            errors_.report(ErrorKind::Semantic, node->line, node->column,
                           "'" + node->label + "' is not a function");
        }
    } else {
        if (sym->kind != SymbolKind::Procedure && sym->kind != SymbolKind::Function) {
            errors_.report(ErrorKind::Semantic, node->line, node->column,
                           "'" + node->label + "' is not callable");
        }
    }

    const int expected = sym->paramCount;
    const int given = static_cast<int>(node->children.size());
    if (expected != given) {
        errors_.report(ErrorKind::Semantic, node->line, node->column,
                       "call to '" + node->label + "' expects " + std::to_string(expected) +
                           " argument(s), got " + std::to_string(given));
    }

    const size_t checkCount =
        std::min(node->children.size(), sym->paramTypes.size());
    for (size_t i = 0; i < checkCount; ++i) {
        const std::string actual = inferType(node->children[i].get());
        const std::string& formal = sym->paramTypes[i];
        if (actual != TypeNames::UNKNOWN && !TypeSystem::argumentCompatible(formal, actual)) {
            errors_.report(ErrorKind::Semantic, node->children[i]->line,
                           node->children[i]->column,
                           "argument " + std::to_string(i + 1) + " of '" + node->label +
                               "' expects " + formal + ", got " + actual);
        }
    }

    for (const auto& arg : node->children) {
        if (arg->inferredType.empty()) {
            inferType(arg.get());
        }
    }

    if (asFunction && sym->kind == SymbolKind::Function) {
        annotate(node, sym->typeName);
    } else {
        annotate(node, TypeNames::UNKNOWN);
    }
}

void SemanticAnalyzer::checkNode(AstNode* node) {
    if (!node) {
        return;
    }

    switch (node->kind) {
        case AstNodeKind::Program:
            for (const auto& ch : node->children) {
                checkNode(ch.get());
            }
            break;
        case AstNodeKind::Block:
            enterScope();
            for (const auto& ch : node->children) {
                checkNode(ch.get());
            }
            exitScope();
            break;
        case AstNodeKind::FunctionDecl:
        case AstNodeKind::ProcedureDecl:
            enterScope();
            for (const auto& ch : node->children) {
                checkNode(ch.get());
            }
            exitScope();
            break;
        case AstNodeKind::Assign:
            checkAssignment(node);
            break;
        case AstNodeKind::If:
            if (!node->children.empty()) {
                checkCondition(node->children[0].get(), "if");
            }
            for (size_t i = 1; i < node->children.size(); ++i) {
                checkNode(node->children[i].get());
            }
            break;
        case AstNodeKind::While:
            if (!node->children.empty()) {
                checkCondition(node->children[0].get(), "while");
            }
            for (size_t i = 1; i < node->children.size(); ++i) {
                checkNode(node->children[i].get());
            }
            break;
        case AstNodeKind::ProcCall:
            checkCall(node, false);
            break;
        case AstNodeKind::FuncCall:
            inferType(node);
            break;
        case AstNodeKind::BinaryOp:
        case AstNodeKind::UnaryOp:
            inferType(node);
            break;
        case AstNodeKind::ArrayAccess:
            checkArrayAccess(node, "array access");
            break;
        case AstNodeKind::Variable:
            checkIdentifierUse(node, "expression");
            break;
        default:
            for (const auto& ch : node->children) {
                checkNode(ch.get());
            }
            break;
    }
}
