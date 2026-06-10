#include "rd_parser.h"

#include "../error/recovery.h"

#include <sstream>

namespace {

std::unique_ptr<AstNode> makeBinaryOp(const std::string& op, int line, int col,
                                      std::unique_ptr<AstNode> left,
                                      std::unique_ptr<AstNode> right) {
    auto node = makeNode(AstNodeKind::BinaryOp, op, line, col);
    node->children.push_back(std::move(left));
    node->children.push_back(std::move(right));
    return node;
}

std::unique_ptr<AstNode> makeUnaryOp(const std::string& op, int line, int col,
                                     std::unique_ptr<AstNode> operand) {
    auto node = makeNode(AstNodeKind::UnaryOp, op, line, col);
    node->children.push_back(std::move(operand));
    return node;
}

std::string typeLabel(const AstNode* ty) {
    return ty ? ty->label : "unknown";
}

SymbolKind typeToKind(const AstNode* ty) {
    if (ty && ty->label.rfind("array", 0) == 0) {
        return SymbolKind::Array;
    }
    return SymbolKind::Variable;
}

std::vector<std::string> collectParamTypes(const AstNode* subprogramHead) {
    std::vector<std::string> types;
    if (!subprogramHead) {
        return types;
    }
    for (const auto& child : subprogramHead->children) {
        if (child->kind != AstNodeKind::ParamDecl) {
            continue;
        }
        std::string currentType;
        int pending = 0;
        for (const auto& part : child->children) {
            if (part->kind == AstNodeKind::Variable) {
                if (currentType.empty()) {
                    ++pending;
                } else {
                    types.push_back(currentType);
                }
            } else if (part->kind == AstNodeKind::TypeNode) {
                currentType = typeLabel(part.get());
                while (pending-- > 0) {
                    types.push_back(currentType);
                }
            }
        }
    }
    return types;
}

}  // namespace

RecursiveDescentParser::RecursiveDescentParser(TokenStream& ts, ErrorHandler& errors,
                                             SymbolTableManager& symtab,
                                             const GrammarAnalysis& grammar)
    : ts_(ts), errors_(errors), symtab_(symtab), grammar_(grammar) {}

void RecursiveDescentParser::syntaxError(const std::string& expected) {
    const Token& t = ts_.current();
    errors_.report(ErrorKind::Syntactic, t.line, t.column,
                   "unexpected token '" + t.lexeme + "'", expected, ts_.currentSymbol());
    panic_ = true;
}

void RecursiveDescentParser::sync(const std::string& nt) {
    auto follow = ParseRecovery::followOf(grammar_, nt);
    follow.insert(";");
    follow.insert("end");
    follow.insert("$");
    ParseRecovery::synchronize(ts_, follow);
    panic_ = false;
}

bool RecursiveDescentParser::expect(const std::string& sym) {
    if (ts_.match(sym)) {
        return true;
    }
    syntaxError(sym);
    sync("statement");
    return ts_.match(sym);
}

bool RecursiveDescentParser::parse() {
    symtab_.beginScope();
    root_ = parseProgram();
    expect("$");
    return root_ != nullptr && !errors_.hasErrors();
}

std::unique_ptr<AstNode> RecursiveDescentParser::parseProgram() {
    const int startLine = ts_.current().line;
    const int startCol = ts_.current().column;
    if (!expect("PROGRAM")) {
        return nullptr;
    }
    const Token& nameTok = ts_.current();
    if (!expect("ID")) {
        return nullptr;
    }

    auto node = makeNode(AstNodeKind::Program, nameTok.lexeme, startLine, startCol);
    symtab_.insert(nameTok.lexeme, SymbolKind::Program, "program", nameTok.line);

    if (!expect("(")) {
        return nullptr;
    }
    auto params = parseIdentifierList();
    if (params) {
        node->value = params->value;
    }
    if (!expect(")")) {
        return nullptr;
    }
    if (!expect(";")) {
        return nullptr;
    }

    parseDeclarations(node.get());
    parseSubprogramDeclarations(node.get());
    auto body = parseCompoundStatement();
    if (body) {
        node->children.push_back(std::move(body));
    }

    return node;
}

std::unique_ptr<AstNode> RecursiveDescentParser::parseIdentifierList() {
    std::ostringstream names;
    const Token id = ts_.current();
    if (!expect("ID")) {
        return nullptr;
    }
    names << id.lexeme;
    parseIdentifierListRest(names);
    auto node = makeNode(AstNodeKind::Variable, id.lexeme, id.line, id.column);
    node->value = names.str();
    return node;
}

void RecursiveDescentParser::parseIdentifierListRest(std::ostringstream& names) {
    if (!ts_.matches(",")) {
        return;
    }
    expect(",");
    const Token id = ts_.current();
    expect("ID");
    names << ", " << id.lexeme;
    parseIdentifierListRest(names);
}

void RecursiveDescentParser::parseDeclarations(AstNode* program) {
    parseDeclarationsBody(program);
}

void RecursiveDescentParser::parseDeclarationsBody(AstNode* program) {
    if (!ts_.matches("VAR")) {
        return;
    }
    expect("VAR");
    parseDeclarationList(program);
}

void RecursiveDescentParser::parseDeclarationList(AstNode* program) {
    if (!ts_.matches("ID")) {
        return;
    }
    const int line = ts_.current().line;
    const int col = ts_.current().column;
    auto ids = parseIdentifierList();
    expect(":");
    auto ty = parseType();
    expect(";");

    auto decl = makeNode(AstNodeKind::VarDecl, "var", line, col);
    const std::string typeName = typeLabel(ty.get());
    const SymbolKind kind = typeToKind(ty.get());

    if (ids && !ids->value.empty()) {
        std::istringstream iss(ids->value);
        std::string name;
        while (std::getline(iss, name, ',')) {
            const size_t start = name.find_first_not_of(' ');
            const size_t end = name.find_last_not_of(' ');
            if (start == std::string::npos) {
                continue;
            }
            name = name.substr(start, end - start + 1);
            decl->children.push_back(makeNode(AstNodeKind::Variable, name, line, col));
            if (!symtab_.insert(name, kind, typeName, line)) {
                errors_.report(ErrorKind::Semantic, line, col,
                               "redeclaration of '" + name + "'");
            }
        }
    }
    if (ty) {
        decl->children.push_back(std::move(ty));
    }
    program->children.push_back(std::move(decl));
    parseDeclarationList(program);
}

std::unique_ptr<AstNode> RecursiveDescentParser::parseType() {
    if (ts_.matches("integer") || ts_.matches("real")) {
        return parseStandardType();
    }
    if (ts_.matches("ARRAY")) {
        const int line = ts_.current().line;
        const int col = ts_.current().column;
        expect("ARRAY");
        expect("[");
        const Token lo = ts_.current();
        expect("num");
        expect("..");
        const Token hi = ts_.current();
        expect("num");
        expect("]");
        expect("OF");
        auto st = parseStandardType();
        const std::string elem = typeLabel(st.get());
        auto node = makeNode(AstNodeKind::TypeNode,
                             "array[" + lo.lexeme + ".." + hi.lexeme + "] of " + elem, line, col);
        if (st) {
            node->children.push_back(std::move(st));
        }
        return node;
    }
    syntaxError("type");
    return nullptr;
}

std::unique_ptr<AstNode> RecursiveDescentParser::parseStandardType() {
    const Token& t = ts_.current();
    if (ts_.matches("integer")) {
        expect("integer");
        return makeNode(AstNodeKind::TypeNode, "integer", t.line, t.column);
    }
    if (ts_.matches("real")) {
        expect("real");
        return makeNode(AstNodeKind::TypeNode, "real", t.line, t.column);
    }
    syntaxError("integer or real");
    return nullptr;
}

void RecursiveDescentParser::parseSubprogramDeclarations(AstNode* program) {
    parseSubprogramDeclarationsBody(program);
}

void RecursiveDescentParser::parseSubprogramDeclarationsBody(AstNode* program) {
    if (!ts_.matches("FUNCTION") && !ts_.matches("PROCEDURE")) {
        return;
    }
    auto sub = parseSubprogramDeclaration();
    if (sub) {
        program->children.push_back(std::move(sub));
    }
    expect(";");
    parseSubprogramDeclarationsBody(program);
}

std::unique_ptr<AstNode> RecursiveDescentParser::parseSubprogramDeclaration() {
    symtab_.beginScope();
    auto head = parseSubprogramHead();
    if (!head) {
        symtab_.endScope();
        return nullptr;
    }
    parseDeclarations(head.get());
    auto body = parseCompoundStatement();
    if (body) {
        head->children.push_back(std::move(body));
    }
    symtab_.endScope();
    return head;
}

std::unique_ptr<AstNode> RecursiveDescentParser::parseSubprogramHead() {
    if (ts_.matches("FUNCTION")) {
        const int line = ts_.current().line;
        const int col = ts_.current().column;
        expect("FUNCTION");
        const Token& id = ts_.current();
        expect("ID");
        auto node = makeNode(AstNodeKind::FunctionDecl, id.lexeme, line, col);
        parseArguments(node.get());
        expect(":");
        auto st = parseStandardType();
        const std::string retType = typeLabel(st.get());
        if (st) {
            node->children.push_back(std::move(st));
        }
        expect(";");
        symtab_.insert(id.lexeme, SymbolKind::Function, retType, id.line);
        symtab_.setParamTypes(id.lexeme, collectParamTypes(node.get()));
        return node;
    }
    if (ts_.matches("PROCEDURE")) {
        const int line = ts_.current().line;
        const int col = ts_.current().column;
        expect("PROCEDURE");
        const Token& id = ts_.current();
        expect("ID");
        auto node = makeNode(AstNodeKind::ProcedureDecl, id.lexeme, line, col);
        parseArguments(node.get());
        expect(";");
        symtab_.insert(id.lexeme, SymbolKind::Procedure, "", id.line);
        symtab_.setParamTypes(id.lexeme, collectParamTypes(node.get()));
        return node;
    }
    syntaxError("FUNCTION or PROCEDURE");
    return nullptr;
}

void RecursiveDescentParser::parseArguments(AstNode* parent) {
    if (!ts_.matches("(")) {
        return;
    }
    expect("(");
    auto params = parseParameterList();
    if (params) {
        parent->children.push_back(std::move(params));
    }
    expect(")");
}

std::unique_ptr<AstNode> RecursiveDescentParser::parseParameterList() {
    const int line = ts_.current().line;
    const int col = ts_.current().column;
    auto node = makeNode(AstNodeKind::ParamDecl, "params", line, col);
    auto ids = parseIdentifierList();
    expect(":");
    auto ty = parseType();
    insertParameters(node.get(), ids.get(), ty.get());
    if (ids) {
        std::istringstream iss(ids->value);
        std::string name;
        while (std::getline(iss, name, ',')) {
            const size_t start = name.find_first_not_of(' ');
            const size_t end = name.find_last_not_of(' ');
            if (start == std::string::npos) {
                continue;
            }
            name = name.substr(start, end - start + 1);
            node->children.push_back(makeNode(AstNodeKind::Variable, name, line, col));
        }
    }
    if (ty) {
        node->children.push_back(std::move(ty));
    }
    parseParameterListRest(node.get());
    return node;
}

void RecursiveDescentParser::insertParameters(AstNode* /*paramNode*/, const AstNode* ids,
                                              const AstNode* ty) {
    if (!ids || !ty) {
        return;
    }
    const std::string typeName = typeLabel(ty);
    std::istringstream iss(ids->value);
    std::string name;
    while (std::getline(iss, name, ',')) {
        const size_t start = name.find_first_not_of(' ');
        const size_t end = name.find_last_not_of(' ');
        if (start == std::string::npos) {
            continue;
        }
        name = name.substr(start, end - start + 1);
        if (!symtab_.insert(name, SymbolKind::Parameter, typeName, ids->line)) {
            errors_.report(ErrorKind::Semantic, ids->line, ids->column,
                           "redeclaration of parameter '" + name + "'");
        }
    }
}

void RecursiveDescentParser::parseParameterListRest(AstNode* parent) {
    if (!ts_.matches(";")) {
        return;
    }
    expect(";");
    auto ids = parseIdentifierList();
    expect(":");
    auto ty = parseType();
    insertParameters(parent, ids.get(), ty.get());
    if (ty) {
        parent->children.push_back(std::move(ty));
    }
    if (ids) {
        std::istringstream iss(ids->value);
        std::string name;
        while (std::getline(iss, name, ',')) {
            const size_t start = name.find_first_not_of(' ');
            const size_t end = name.find_last_not_of(' ');
            if (start == std::string::npos) {
                continue;
            }
            name = name.substr(start, end - start + 1);
            parent->children.push_back(
                makeNode(AstNodeKind::Variable, name, ids->line, ids->column));
        }
    }
    parseParameterListRest(parent);
}

std::unique_ptr<AstNode> RecursiveDescentParser::parseCompoundStatement() {
    auto node = makeNode(AstNodeKind::Block, "begin", ts_.current().line, ts_.current().column);
    symtab_.beginScope();
    expect("begin");
    parseOptionalStatements(node.get());
    expect("end");
    symtab_.endScope();
    return node;
}

void RecursiveDescentParser::parseOptionalStatements(AstNode* parent) {
    if (!ts_.matches("ID") && !ts_.matches("IF") && !ts_.matches("WHILE") && !ts_.matches("begin")) {
        return;
    }
    parseStatementList(parent);
}

void RecursiveDescentParser::parseStatementList(AstNode* parent) {
    auto st = parseStatement();
    if (st) {
        parent->children.push_back(std::move(st));
    }
    parseStatementListRest(parent);
}

void RecursiveDescentParser::parseStatementListRest(AstNode* parent) {
    if (!ts_.matches(";")) {
        return;
    }
    expect(";");
    auto st = parseStatement();
    if (st) {
        parent->children.push_back(std::move(st));
    }
    parseStatementListRest(parent);
}

std::unique_ptr<AstNode> RecursiveDescentParser::parseStatement() {
    if (ts_.matches("ID")) {
        const Token id = ts_.current();
        expect("ID");
        if (ts_.matches(":=") || ts_.matches("[")) {
            auto lhs = parseVariable(id);
            expect(":=");
            auto rhs = parseExpression();
            auto assign = makeNode(AstNodeKind::Assign, ":=", id.line, id.column);
            assign->children.push_back(std::move(lhs));
            if (rhs) {
                assign->children.push_back(std::move(rhs));
            }
            return assign;
        }
        auto call = makeNode(AstNodeKind::ProcCall, id.lexeme, id.line, id.column);
        parseProcedureSuffix(call.get());
        return call;
    }
    if (ts_.matches("begin")) {
        return parseCompoundStatement();
    }
    if (ts_.matches("IF")) {
        const int line = ts_.current().line;
        const int col = ts_.current().column;
        auto node = makeNode(AstNodeKind::If, "if", line, col);
        expect("IF");
        auto cond = parseExpression();
        if (cond) {
            node->children.push_back(std::move(cond));
        }
        expect("THEN");
        auto thenBranch = parseStatement();
        if (thenBranch) {
            node->children.push_back(std::move(thenBranch));
        }
        expect("ELSE");
        auto elseBranch = parseStatement();
        if (elseBranch) {
            node->children.push_back(std::move(elseBranch));
        }
        return node;
    }
    if (ts_.matches("WHILE")) {
        const int line = ts_.current().line;
        const int col = ts_.current().column;
        auto node = makeNode(AstNodeKind::While, "while", line, col);
        expect("WHILE");
        auto cond = parseExpression();
        if (cond) {
            node->children.push_back(std::move(cond));
        }
        expect("DO");
        auto body = parseStatement();
        if (body) {
            node->children.push_back(std::move(body));
        }
        return node;
    }
    syntaxError("statement");
    sync("statement");
    return nullptr;
}

std::unique_ptr<AstNode> RecursiveDescentParser::parseVariable(const Token& id) {
    auto node = makeNode(AstNodeKind::Variable, id.lexeme, id.line, id.column);
    if (!ts_.matches("[")) {
        return node;
    }
    expect("[");
    auto index = parseExpression();
    expect("]");
    auto arr = makeNode(AstNodeKind::ArrayAccess, id.lexeme, id.line, id.column);
    arr->children.push_back(std::move(node));
    if (index) {
        arr->children.push_back(std::move(index));
    }
    return arr;
}

void RecursiveDescentParser::parseProcedureSuffix(AstNode* call) {
    if (!ts_.matches("(")) {
        return;
    }
    expect("(");
    parseExpressionList(call);
    expect(")");
}

void RecursiveDescentParser::parseExpressionList(AstNode* parent) {
    auto expr = parseExpression();
    if (expr) {
        parent->children.push_back(std::move(expr));
    }
    parseExpressionListRest(parent);
}

void RecursiveDescentParser::parseExpressionListRest(AstNode* parent) {
    if (!ts_.matches(",")) {
        return;
    }
    expect(",");
    auto expr = parseExpression();
    if (expr) {
        parent->children.push_back(std::move(expr));
    }
    parseExpressionListRest(parent);
}

std::unique_ptr<AstNode> RecursiveDescentParser::parseExpression() {
    auto left = parseSimpleExpression();
    if (!left) {
        return nullptr;
    }
    if (!ts_.matches("relop")) {
        return left;
    }
    const Token& op = ts_.current();
    expect("relop");
    auto right = parseSimpleExpression();
    return makeBinaryOp(op.lexeme, op.line, op.column, std::move(left), std::move(right));
}

std::unique_ptr<AstNode> RecursiveDescentParser::parseSimpleExpression() {
    std::unique_ptr<AstNode> left;
    if (ts_.matches("+") || ts_.matches("-")) {
        const Token sign = ts_.current();
        if (ts_.matches("+")) {
            expect("+");
        } else {
            expect("-");
        }
        auto term = parseTerm();
        left = makeUnaryOp(sign.lexeme, sign.line, sign.column, std::move(term));
    } else {
        left = parseTerm();
    }
    while (ts_.matches("addop")) {
        const Token& op = ts_.current();
        expect("addop");
        auto right = parseTerm();
        left = makeBinaryOp(op.lexeme, op.line, op.column, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<AstNode> RecursiveDescentParser::parseTerm() {
    auto left = parseFactor();
    while (ts_.matches("mulop")) {
        const Token& op = ts_.current();
        expect("mulop");
        auto right = parseFactor();
        left = makeBinaryOp(op.lexeme, op.line, op.column, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<AstNode> RecursiveDescentParser::parseFactor() {
    if (ts_.matches("ID")) {
        const Token& id = ts_.current();
        expect("ID");
        if (ts_.matches("(")) {
            auto call = makeNode(AstNodeKind::FuncCall, id.lexeme, id.line, id.column);
            expect("(");
            parseExpressionList(call.get());
            expect(")");
            return call;
        }
        if (ts_.matches("[")) {
            return parseVariable(id);
        }
        return makeNode(AstNodeKind::Variable, id.lexeme, id.line, id.column);
    }
    if (ts_.matches("num")) {
        const Token& n = ts_.current();
        expect("num");
        return makeNode(AstNodeKind::Constant, n.lexeme, n.line, n.column);
    }
    if (ts_.matches("(")) {
        expect("(");
        auto expr = parseExpression();
        expect(")");
        return expr;
    }
    if (ts_.matches("NOT")) {
        const int line = ts_.current().line;
        const int col = ts_.current().column;
        expect("NOT");
        auto operand = parseFactor();
        return makeUnaryOp("not", line, col, std::move(operand));
    }
    syntaxError("factor");
    return nullptr;
}
