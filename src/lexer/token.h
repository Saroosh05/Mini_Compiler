#ifndef TOKEN_H
#define TOKEN_H

#include <string>

// Token kinds for the assigned Pascal subset.
// Grammar terminals (PROGRAM, begin, relop, ...) map via tokenGrammarSymbol().
enum class TokenKind {
    // Keywords (grammar uses mixed case; lexeme holds source spelling)
    KW_PROGRAM,
    KW_VAR,
    KW_INTEGER,
    KW_REAL,
    KW_ARRAY,
    KW_FUNCTION,
    KW_PROCEDURE,
    KW_BEGIN,
    KW_END,
    KW_IF,
    KW_THEN,
    KW_ELSE,
    KW_WHILE,
    KW_DO,
    KW_NOT,
    KW_OF,
    KW_AND,
    KW_OR,
    KW_DIV,
    KW_MOD,
    KW_READ,
    KW_WRITE,

    ID,
    NUM_INT,
    NUM_REAL,

    ASSIGN,   // :=
    RANGE,    // ..
    RELOP,
    ADDOP,
    MULOP,

    LPAREN,    // (
    RPAREN,    // )
    LBRACK,    // [
    RBRACK,    // ]
    COMMA,     // ,
    SEMICOLON, // ;
    COLON,     // :
    DOT,       // .

    END_OF_FILE,
    ERROR
};

struct Token {
    TokenKind kind{TokenKind::END_OF_FILE};
    std::string lexeme;
    std::string attribute;  // numeric value, keyword spelling, relop text, etc.
    int line{1};
    int column{1};
};

const char* tokenKindName(TokenKind kind);
const char* tokenGrammarSymbol(TokenKind kind);
// Terminal symbol for parsing tables (maps +/-/to literal, relop/addop/mulop classes).
std::string terminalGrammarSymbol(const Token& token);

#endif
