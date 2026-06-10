#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "../ast/ast.h"
#include "../error/error_handler.h"
#include "../symbol_table/symbol_table.h"

class SemanticAnalyzer {
public:
    SemanticAnalyzer(SymbolTableManager& symtab, ErrorHandler& errors);

    bool analyze(AstNode* root);

private:
    SymbolTableManager& symtab_;
    ErrorHandler& errors_;
    int scopeDepth_{0};

    void enterScope();
    void exitScope();
    void checkNode(AstNode* node);
    void checkAssignment(AstNode* node);
    void checkCall(AstNode* node, bool asFunction);
    void checkCondition(AstNode* node, const char* context);
    void checkArrayAccess(AstNode* node, const char* context);
    void checkIdentifierUse(const AstNode* node, const char* context);
    std::string inferType(AstNode* node);
    void annotate(AstNode* node, const std::string& type);
};

#endif
