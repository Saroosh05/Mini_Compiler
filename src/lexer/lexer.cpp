#include "lexer.h"

#include "../error/error_handler.h"

#include <cctype>
#include <unordered_map>

namespace {

bool isIdentStart(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool isIdentPart(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

}  // namespace

Lexer::Lexer(const std::string& sourcePath) : buffer_(sourcePath) {}

bool Lexer::init() {
    if (!buffer_.open()) {
        return false;
    }
    initialized_ = true;
    eof_delivered_ = false;
    tokens_.clear();
    return true;
}

void Lexer::setErrorHandler(ErrorHandler* handler) {
    errors_ = handler;
}

std::string Lexer::toLower(const std::string& s) {
    std::string out = s;
    for (char& ch : out) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return out;
}

std::optional<TokenKind> Lexer::keywordKind(const std::string& lower) {
    static const std::unordered_map<std::string, TokenKind> table = {
        {"program", TokenKind::KW_PROGRAM},     {"var", TokenKind::KW_VAR},
        {"integer", TokenKind::KW_INTEGER},     {"real", TokenKind::KW_REAL},
        {"array", TokenKind::KW_ARRAY},         {"function", TokenKind::KW_FUNCTION},
        {"procedure", TokenKind::KW_PROCEDURE}, {"begin", TokenKind::KW_BEGIN},
        {"end", TokenKind::KW_END},             {"if", TokenKind::KW_IF},
        {"then", TokenKind::KW_THEN},           {"else", TokenKind::KW_ELSE},
        {"while", TokenKind::KW_WHILE},         {"do", TokenKind::KW_DO},
        {"not", TokenKind::KW_NOT},             {"of", TokenKind::KW_OF},
        {"and", TokenKind::KW_AND},             {"or", TokenKind::KW_OR},
        {"div", TokenKind::KW_DIV},             {"mod", TokenKind::KW_MOD},
        {"read", TokenKind::KW_READ},           {"write", TokenKind::KW_WRITE},
    };
    const auto it = table.find(lower);
    if (it == table.end()) {
        return std::nullopt;
    }
    return it->second;
}

Token Lexer::makeToken(TokenKind kind, const std::string& lexeme, const std::string& attribute,
                       int line, int col) {
    Token t;
    t.kind = kind;
    t.lexeme = lexeme;
    t.attribute = attribute.empty() ? lexeme : attribute;
    t.line = line;
    t.column = col;
    tokens_.push_back(t);
    return t;
}

void Lexer::reportLexicalError(int line, int col, const std::string& message) {
    if (errors_) {
        errors_->report(ErrorKind::Lexical, line, col, message);
    }
}

std::optional<Token> Lexer::nextToken() {
    if (!initialized_ || eof_delivered_) {
        return std::nullopt;
    }

    skipWhitespaceAndComments();

    if (buffer_.eof()) {
        eof_delivered_ = true;
        return makeToken(TokenKind::END_OF_FILE, "$", "$", buffer_.line(), buffer_.column());
    }

    const auto tok = scanToken();
    if (!tok) {
        eof_delivered_ = true;
        return makeToken(TokenKind::END_OF_FILE, "$", "$", buffer_.line(), buffer_.column());
    }
    return tok;
}

void Lexer::skipWhitespaceAndComments() {
    while (!buffer_.eof()) {
        const char c = buffer_.peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            buffer_.advance();
            continue;
        }
        if (c == '{') {
            const int startLine = buffer_.line();
            const int startCol = buffer_.column();
            buffer_.advance();
            while (!buffer_.eof()) {
                const char ch = buffer_.peek();
                if (ch == '}') {
                    buffer_.advance();
                    break;
                }
                if (ch == '\0') {
                    reportLexicalError(startLine, startCol, "unterminated comment");
                    return;
                }
                buffer_.advance();
            }
            continue;
        }
        break;
    }
}

std::optional<Token> Lexer::scanToken() {
    const int startLine = buffer_.line();
    const int startCol = buffer_.column();
    const char c = buffer_.peek();

    if (buffer_.eof() || c == '\0') {
        return std::nullopt;
    }

    // Identifier / keyword
    if (isIdentStart(c)) {
        std::string lexeme;
        while (!buffer_.eof() && isIdentPart(buffer_.peek())) {
            lexeme.push_back(buffer_.advance());
        }
        const std::string lower = toLower(lexeme);
        if (auto kw = keywordKind(lower)) {
            return makeToken(*kw, lexeme, lower, startLine, startCol);
        }
        return makeToken(TokenKind::ID, lexeme, lexeme, startLine, startCol);
    }

    // Number: integer, real (digits '.' digits), or integer before '..'
    if (std::isdigit(static_cast<unsigned char>(c))) {
        std::string lexeme;
        while (!buffer_.eof() && std::isdigit(static_cast<unsigned char>(buffer_.peek()))) {
            lexeme.push_back(buffer_.advance());
        }

        if (!buffer_.eof() && buffer_.peek() == '.' && buffer_.peek2() == '.') {
            // e.g. 1..10: leave ".." for RANGE token on the next call.
            return makeToken(TokenKind::NUM_INT, lexeme, lexeme, startLine, startCol);
        }
        if (!buffer_.eof() && buffer_.peek() == '.') {
            const char dot = buffer_.advance();
            if (!buffer_.eof() && std::isdigit(static_cast<unsigned char>(buffer_.peek()))) {
                lexeme.push_back(dot);
                while (!buffer_.eof() &&
                       std::isdigit(static_cast<unsigned char>(buffer_.peek()))) {
                    lexeme.push_back(buffer_.advance());
                }
                return makeToken(TokenKind::NUM_REAL, lexeme, lexeme, startLine, startCol);
            }
            reportLexicalError(buffer_.line(), buffer_.column(),
                               "invalid number after '" + lexeme + dot + "'");
            return makeToken(TokenKind::ERROR, lexeme + std::string(1, dot), lexeme,
                             startLine, startCol);
        }

        return makeToken(TokenKind::NUM_INT, lexeme, lexeme, startLine, startCol);
    }

    // Two-character operators
    if (c == ':') {
        buffer_.advance();
        if (!buffer_.eof() && buffer_.peek() == '=') {
            buffer_.advance();
            return makeToken(TokenKind::ASSIGN, ":=", ":=", startLine, startCol);
        }
        return makeToken(TokenKind::COLON, ":", ":", startLine, startCol);
    }

    if (c == '.') {
        buffer_.advance();
        while (!buffer_.eof() &&
               (buffer_.peek() == ' ' || buffer_.peek() == '\t')) {
            buffer_.advance();
        }
        if (!buffer_.eof() && buffer_.peek() == '.') {
            buffer_.advance();
            return makeToken(TokenKind::RANGE, "..", "..", startLine, startCol);
        }
        return makeToken(TokenKind::DOT, ".", ".", startLine, startCol);
    }

    if (c == '<') {
        buffer_.advance();
        std::string op = "<";
        if (!buffer_.eof() && (buffer_.peek() == '=' || buffer_.peek() == '>')) {
            op.push_back(buffer_.advance());
        }
        if (op == "<>") {
            return makeToken(TokenKind::RELOP, "<>", "<>", startLine, startCol);
        }
        return makeToken(TokenKind::RELOP, op, op, startLine, startCol);
    }

    if (c == '>') {
        buffer_.advance();
        std::string op = ">";
        if (!buffer_.eof() && buffer_.peek() == '=') {
            op.push_back(buffer_.advance());
        }
        return makeToken(TokenKind::RELOP, op, op, startLine, startCol);
    }

    if (c == '=') {
        buffer_.advance();
        return makeToken(TokenKind::RELOP, "=", "=", startLine, startCol);
    }

    if (c == '+') {
        buffer_.advance();
        return makeToken(TokenKind::ADDOP, "+", "+", startLine, startCol);
    }

    if (c == '-') {
        buffer_.advance();
        return makeToken(TokenKind::ADDOP, "-", "-", startLine, startCol);
    }

    if (c == '*') {
        buffer_.advance();
        return makeToken(TokenKind::MULOP, "*", "*", startLine, startCol);
    }

    if (c == '/') {
        buffer_.advance();
        return makeToken(TokenKind::MULOP, "/", "/", startLine, startCol);
    }

    // Single-character delimiters
    buffer_.advance();
    switch (c) {
        case '(': return makeToken(TokenKind::LPAREN, "(", "(", startLine, startCol);
        case ')': return makeToken(TokenKind::RPAREN, ")", ")", startLine, startCol);
        case '[': return makeToken(TokenKind::LBRACK, "[", "[", startLine, startCol);
        case ']': return makeToken(TokenKind::RBRACK, "]", "]", startLine, startCol);
        case ',': return makeToken(TokenKind::COMMA, ",", ",", startLine, startCol);
        case ';': return makeToken(TokenKind::SEMICOLON, ";", ";", startLine, startCol);
        default: break;
    }

    std::string bad(1, c);
    reportLexicalError(startLine, startCol, "illegal character '" + bad + "'");
    return makeToken(TokenKind::ERROR, bad, bad, startLine, startCol);
}
