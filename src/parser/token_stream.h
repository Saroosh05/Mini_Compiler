#ifndef TOKEN_STREAM_H
#define TOKEN_STREAM_H

#include "../lexer/lexer.h"
#include "../lexer/token.h"

#include <string>
#include <vector>

// Parser-facing view of the token stream (pull lexer once, then index).
class TokenStream {
public:
    explicit TokenStream(Lexer& lexer);
    explicit TokenStream(const std::vector<Token>& tokens);

    const Token& current() const;
    const Token& peek(int offset = 0) const;
    bool atEnd() const;
    std::string currentSymbol() const;

    bool matches(const std::string& grammarSymbol) const;
    bool match(const std::string& grammarSymbol);
    void advance();

    size_t position() const { return pos_; }
    size_t size() const { return tokens_.size(); }
    const std::vector<Token>& all() const { return tokens_; }

    // Pascal "end." after final end is not in the grammar; strip lone DOT before EOF.
    void normalizeOptionalProgramEndDot();

private:
    std::vector<Token> tokens_;
    size_t pos_{0};
};

bool grammarSymbolMatches(const Token& token, const std::string& grammarSymbol);

#endif
