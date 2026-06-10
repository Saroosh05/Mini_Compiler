#ifndef RD_PARSER_H
#define RD_PARSER_H

#include "../ast/ast.h"
#include "../error/error_handler.h"
#include "../grammar_analysis/grammar_analysis.h"
#include "../symbol_table/symbol_table.h"
#include "token_stream.h"

#include <memory>
#include <sstream>
#include <string>

class RecursiveDescentParser {
public:
    RecursiveDescentParser(TokenStream& ts, ErrorHandler& errors, SymbolTableManager& symtab,
                           const GrammarAnalysis& grammar);

    bool parse();
    const AstNode* ast() const { return root_.get(); }
    std::unique_ptr<AstNode> releaseAst() { return std::move(root_); }

private:
    TokenStream& ts_;
    ErrorHandler& errors_;
    SymbolTableManager& symtab_;
    const GrammarAnalysis& grammar_;
    std::unique_ptr<AstNode> root_;
    bool panic_{false};

    void syntaxError(const std::string& expected);
    void sync(const std::string& nt);
    bool expect(const std::string& sym);

    std::unique_ptr<AstNode> parseProgram();
    std::unique_ptr<AstNode> parseIdentifierList();
    void parseIdentifierListRest(std::ostringstream& names);
    void parseDeclarations(AstNode* program);
    void parseDeclarationsBody(AstNode* program);
    void parseDeclarationList(AstNode* program);
    std::unique_ptr<AstNode> parseType();
    std::unique_ptr<AstNode> parseStandardType();
    void parseSubprogramDeclarations(AstNode* program);
    void parseSubprogramDeclarationsBody(AstNode* program);
    std::unique_ptr<AstNode> parseSubprogramDeclaration();
    std::unique_ptr<AstNode> parseSubprogramHead();
    void parseArguments(AstNode* parent);
    std::unique_ptr<AstNode> parseParameterList();
    void insertParameters(AstNode* paramNode, const AstNode* ids, const AstNode* ty);
    void parseParameterListRest(AstNode* parent);
    std::unique_ptr<AstNode> parseCompoundStatement();
    void parseOptionalStatements(AstNode* parent);
    void parseStatementList(AstNode* parent);
    void parseStatementListRest(AstNode* parent);
    std::unique_ptr<AstNode> parseStatement();
    std::unique_ptr<AstNode> parseVariable(const Token& id);
    void parseProcedureSuffix(AstNode* call);
    void parseExpressionList(AstNode* parent);
    void parseExpressionListRest(AstNode* parent);
    std::unique_ptr<AstNode> parseExpression();
    std::unique_ptr<AstNode> parseSimpleExpression();
    std::unique_ptr<AstNode> parseTerm();
    std::unique_ptr<AstNode> parseFactor();
};

#endif
