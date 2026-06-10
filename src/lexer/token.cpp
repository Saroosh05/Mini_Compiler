#include "token.h"

const char* tokenKindName(TokenKind kind) {
    switch (kind) {
        case TokenKind::KW_PROGRAM: return "KEYWORD";
        case TokenKind::KW_VAR: return "KEYWORD";
        case TokenKind::KW_INTEGER: return "KEYWORD";
        case TokenKind::KW_REAL: return "KEYWORD";
        case TokenKind::KW_ARRAY: return "KEYWORD";
        case TokenKind::KW_FUNCTION: return "KEYWORD";
        case TokenKind::KW_PROCEDURE: return "KEYWORD";
        case TokenKind::KW_BEGIN: return "KEYWORD";
        case TokenKind::KW_END: return "KEYWORD";
        case TokenKind::KW_IF: return "KEYWORD";
        case TokenKind::KW_THEN: return "KEYWORD";
        case TokenKind::KW_ELSE: return "KEYWORD";
        case TokenKind::KW_WHILE: return "KEYWORD";
        case TokenKind::KW_DO: return "KEYWORD";
        case TokenKind::KW_NOT: return "KEYWORD";
        case TokenKind::KW_OF: return "KEYWORD";
        case TokenKind::KW_AND: return "KEYWORD";
        case TokenKind::KW_OR: return "KEYWORD";
        case TokenKind::KW_DIV: return "KEYWORD";
        case TokenKind::KW_MOD: return "KEYWORD";
        case TokenKind::KW_READ: return "KEYWORD";
        case TokenKind::KW_WRITE: return "KEYWORD";
        case TokenKind::ID: return "ID";
        case TokenKind::NUM_INT: return "NUM";
        case TokenKind::NUM_REAL: return "NUM";
        case TokenKind::ASSIGN: return "ASSIGN";
        case TokenKind::RANGE: return "RANGE";
        case TokenKind::RELOP: return "RELOP";
        case TokenKind::ADDOP: return "ADDOP";
        case TokenKind::MULOP: return "MULOP";
        case TokenKind::LPAREN: return "LPAREN";
        case TokenKind::RPAREN: return "RPAREN";
        case TokenKind::LBRACK: return "LBRACK";
        case TokenKind::RBRACK: return "RBRACK";
        case TokenKind::COMMA: return "COMMA";
        case TokenKind::SEMICOLON: return "SEMICOLON";
        case TokenKind::COLON: return "COLON";
        case TokenKind::DOT: return "DOT";
        case TokenKind::END_OF_FILE: return "EOF";
        case TokenKind::ERROR: return "ERROR";
    }
    return "?";
}

// Symbol names used in grammar_ll1.txt / parser tables.
const char* tokenGrammarSymbol(TokenKind kind) {
    switch (kind) {
        case TokenKind::KW_PROGRAM: return "PROGRAM";
        case TokenKind::KW_VAR: return "VAR";
        case TokenKind::KW_INTEGER: return "integer";
        case TokenKind::KW_REAL: return "real";
        case TokenKind::KW_ARRAY: return "ARRAY";
        case TokenKind::KW_FUNCTION: return "FUNCTION";
        case TokenKind::KW_PROCEDURE: return "PROCEDURE";
        case TokenKind::KW_BEGIN: return "begin";
        case TokenKind::KW_END: return "end";
        case TokenKind::KW_IF: return "IF";
        case TokenKind::KW_THEN: return "THEN";
        case TokenKind::KW_ELSE: return "ELSE";
        case TokenKind::KW_WHILE: return "WHILE";
        case TokenKind::KW_DO: return "DO";
        case TokenKind::KW_NOT: return "NOT";
        case TokenKind::KW_OF: return "OF";
        case TokenKind::KW_AND: return "and";
        case TokenKind::KW_OR: return "or";
        case TokenKind::KW_DIV: return "div";
        case TokenKind::KW_MOD: return "mod";
        case TokenKind::KW_READ: return "read";
        case TokenKind::KW_WRITE: return "write";
        case TokenKind::ID: return "ID";
        case TokenKind::NUM_INT: return "num";
        case TokenKind::NUM_REAL: return "num";
        case TokenKind::ASSIGN: return ":=";
        case TokenKind::RANGE: return "..";
        case TokenKind::RELOP: return "relop";
        case TokenKind::ADDOP: return "addop";
        case TokenKind::MULOP: return "mulop";
        case TokenKind::LPAREN: return "(";
        case TokenKind::RPAREN: return ")";
        case TokenKind::LBRACK: return "[";
        case TokenKind::RBRACK: return "]";
        case TokenKind::COMMA: return ",";
        case TokenKind::SEMICOLON: return ";";
        case TokenKind::COLON: return ":";
        case TokenKind::DOT: return ".";
        case TokenKind::END_OF_FILE: return "$";
        case TokenKind::ERROR: return "ERROR";
    }
    return "?";
}

std::string terminalGrammarSymbol(const Token& token) {
    if (token.kind == TokenKind::END_OF_FILE) {
        return "$";
    }
    if (token.kind == TokenKind::ADDOP || token.kind == TokenKind::MULOP) {
        return token.lexeme;
    }
    return tokenGrammarSymbol(token.kind);
}
