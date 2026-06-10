#ifndef AST_H
#define AST_H

#include <memory>
#include <ostream>
#include <string>
#include <vector>

// Semantic AST node kinds represent program meaning, not grammar derivations.
enum class AstNodeKind {
    Program,
    Block,
    VarDecl,
    TypeNode,
    FunctionDecl,
    ProcedureDecl,
    ParamDecl,
    Assign,
    If,
    While,
    ProcCall,
    FuncCall,
    ArrayAccess,
    BinaryOp,
    UnaryOp,
    Variable,
    Constant
};

struct AstNode {
    AstNodeKind kind{AstNodeKind::Program};
    std::string label;
    std::string value;
    std::string inferredType;
    int line{0};
    int column{0};
    std::vector<std::unique_ptr<AstNode>> children;

    AstNode(AstNodeKind k, std::string lbl, int ln = 0, int col = 0);
};

class AstPrinter {
public:
    void print(std::ostream& out, const AstNode* root) const;

private:
    std::string formatLabel(const AstNode* node) const;
    bool suppressChildren(const AstNode* node) const;
    void printTree(std::ostream& out, const AstNode* node, const std::string& prefix,
                   bool isLast) const;
};

std::unique_ptr<AstNode> makeNode(AstNodeKind kind, const std::string& label, int line = 0,
                                  int col = 0);

#endif
