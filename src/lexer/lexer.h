#ifndef LEXER_H
#define LEXER_H

#include "buffer.h"
#include "token.h"

#include <optional>
#include <string>
#include <vector>

class ErrorHandler;

class Lexer {
public:
    explicit Lexer(const std::string& sourcePath);

    bool init();
    void setErrorHandler(ErrorHandler* handler);

    // Returns next token, or nullopt after EOF has been returned once.
    std::optional<Token> nextToken();

    const std::vector<Token>& tokens() const { return tokens_; }

private:
    Buffer buffer_;
    ErrorHandler* errors_{nullptr};
    bool initialized_{false};
    bool eof_delivered_{false};
    std::vector<Token> tokens_;

    void skipWhitespaceAndComments();
    std::optional<Token> scanToken();

    Token makeToken(TokenKind kind, const std::string& lexeme, const std::string& attribute,
                    int line, int col);
    void reportLexicalError(int line, int col, const std::string& message);
    static std::string toLower(const std::string& s);
    static std::optional<TokenKind> keywordKind(const std::string& lower);
};

#endif
