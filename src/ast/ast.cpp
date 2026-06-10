#include "ast.h"

#include <sstream>

AstNode::AstNode(AstNodeKind k, std::string lbl, int ln, int col)
    : kind(k), label(std::move(lbl)), line(ln), column(col) {}

std::unique_ptr<AstNode> makeNode(AstNodeKind kind, const std::string& label, int line,
                                  int col) {
    return std::make_unique<AstNode>(kind, label, line, col);
}

namespace {

std::string joinNames(const AstNode* node) {
    std::ostringstream oss;
    bool first = true;
    for (const auto& ch : node->children) {
        if (ch->kind != AstNodeKind::Variable) {
            continue;
        }
        if (!first) {
            oss << ", ";
        }
        first = false;
        oss << ch->label;
    }
    return oss.str();
}

const AstNode* findTypeChild(const AstNode* node) {
    for (const auto& ch : node->children) {
        if (ch->kind == AstNodeKind::TypeNode) {
            return ch.get();
        }
    }
    return nullptr;
}

}  // namespace

std::string AstPrinter::formatLabel(const AstNode* node) const {
    if (!node) {
        return "(null)";
    }

    switch (node->kind) {
        case AstNodeKind::Program: {
            std::ostringstream oss;
            oss << "Program(" << node->label;
            if (!node->value.empty()) {
                oss << "; " << node->value;
            }
            oss << ")";
            return oss.str();
        }
        case AstNodeKind::Block:
            return "Block";
        case AstNodeKind::VarDecl: {
            const AstNode* ty = findTypeChild(node);
            const std::string typeName = ty ? ty->label : "?";
            return "VarDecl(" + joinNames(node) + " : " + typeName + ")";
        }
        case AstNodeKind::TypeNode:
            return node->label;
        case AstNodeKind::FunctionDecl:
            return "Function(" + node->label + ")";
        case AstNodeKind::ProcedureDecl:
            return "Procedure(" + node->label + ")";
        case AstNodeKind::ParamDecl:
            return "Param(" + joinNames(node) + " : " +
                   (findTypeChild(node) ? findTypeChild(node)->label : "?") + ")";
        case AstNodeKind::Assign:
            return ":=";
        case AstNodeKind::If:
            return "If";
        case AstNodeKind::While:
            return "While";
        case AstNodeKind::ProcCall:
            return "Call(" + node->label + ")";
        case AstNodeKind::BinaryOp:
        case AstNodeKind::UnaryOp:
        case AstNodeKind::FuncCall:
        case AstNodeKind::ArrayAccess: {
            if (!node->inferredType.empty()) {
                return node->label + " : " + node->inferredType;
            }
            if (node->kind == AstNodeKind::FuncCall) {
                return "FuncCall(" + node->label + ")";
            }
            if (node->kind == AstNodeKind::ArrayAccess) {
                return "ArrayAccess(" + node->label + ")";
            }
            return node->label;
        }
        case AstNodeKind::Variable:
            return node->label;
        case AstNodeKind::Constant:
            return "Number(" + node->label + ")";
        default:
            return node->label;
    }
}

bool AstPrinter::suppressChildren(const AstNode* node) const {
    if (!node) {
        return true;
    }
    switch (node->kind) {
        case AstNodeKind::Program:
        case AstNodeKind::VarDecl:
        case AstNodeKind::ParamDecl:
        case AstNodeKind::TypeNode:
        case AstNodeKind::Variable:
        case AstNodeKind::Constant:
            return true;
        default:
            return false;
    }
}

void AstPrinter::printTree(std::ostream& out, const AstNode* node, const std::string& prefix,
                           bool isLast) const {
    out << prefix << (isLast ? "`-- " : "|-- ") << formatLabel(node) << '\n';

    if (suppressChildren(node)) {
        return;
    }

    const std::string childPrefix = prefix + (isLast ? "    " : "|   ");
    for (size_t i = 0; i < node->children.size(); ++i) {
        printTree(out, node->children[i].get(), childPrefix, i + 1 == node->children.size());
    }
}

void AstPrinter::print(std::ostream& out, const AstNode* root) const {
    if (!root) {
        out << "(empty AST)\n";
        return;
    }

    out << formatLabel(root) << '\n';
    for (size_t i = 0; i < root->children.size(); ++i) {
        printTree(out, root->children[i].get(), "", i + 1 == root->children.size());
    }
}
