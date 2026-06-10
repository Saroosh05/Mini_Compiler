#include "token_stream.h"

#include <cstring>

bool grammarSymbolMatches(const Token& token, const std::string& grammarSymbol) {
    if (grammarSymbol == "relop") {
        return token.kind == TokenKind::RELOP;
    }
    if (grammarSymbol == "addop") {
        return token.kind == TokenKind::ADDOP;
    }
    if (grammarSymbol == "+" || grammarSymbol == "-") {
        return token.kind == TokenKind::ADDOP && token.lexeme == grammarSymbol;
    }
    if (grammarSymbol == "mulop") {
        return token.kind == TokenKind::MULOP;
    }
    if (grammarSymbol == "*" || grammarSymbol == "/") {
        return token.kind == TokenKind::MULOP && token.lexeme == grammarSymbol;
    }
    if (grammarSymbol == "num") {
        return token.kind == TokenKind::NUM_INT || token.kind == TokenKind::NUM_REAL;
    }
    if (grammarSymbol == "$") {
        return token.kind == TokenKind::END_OF_FILE;
    }
    return tokenGrammarSymbol(token.kind) == grammarSymbol;
}

static void ensureEof(std::vector<Token>& tokens) {
    if (tokens.empty() || tokens.back().kind != TokenKind::END_OF_FILE) {
        Token eof;
        eof.kind = TokenKind::END_OF_FILE;
        eof.lexeme = "$";
        eof.attribute = "$";
        tokens.push_back(eof);
    }
}

void TokenStream::normalizeOptionalProgramEndDot() {
    if (tokens_.size() < 3) {
        return;
    }
    const size_t eofIdx = tokens_.size() - 1;
    const size_t dotIdx = eofIdx - 1;
    if (tokens_[dotIdx].kind != TokenKind::DOT) {
        return;
    }
    const size_t endIdx = dotIdx - 1;
    if (endIdx >= tokens_.size()) {
        return;
    }
    if (std::strcmp(tokenGrammarSymbol(tokens_[endIdx].kind), "end") == 0) {
        tokens_.erase(tokens_.begin() + static_cast<std::ptrdiff_t>(dotIdx));
    }
}

TokenStream::TokenStream(const std::vector<Token>& tokens) : tokens_(tokens) {
    ensureEof(tokens_);
    normalizeOptionalProgramEndDot();
}

TokenStream::TokenStream(Lexer& lexer) {
    while (std::optional<Token> tok = lexer.nextToken()) {
        tokens_.push_back(*tok);
    }
    ensureEof(tokens_);
    normalizeOptionalProgramEndDot();
}

const Token& TokenStream::current() const {
    static Token empty;
    if (pos_ >= tokens_.size()) {
        return tokens_.back();
    }
    return tokens_[pos_];
}

const Token& TokenStream::peek(int offset) const {
    const size_t idx = pos_ + static_cast<size_t>(offset);
    if (idx >= tokens_.size()) {
        return tokens_.back();
    }
    return tokens_[idx];
}

bool TokenStream::atEnd() const {
    return current().kind == TokenKind::END_OF_FILE;
}

std::string TokenStream::currentSymbol() const {
    return terminalGrammarSymbol(current());
}

bool TokenStream::matches(const std::string& grammarSymbol) const {
    if (grammarSymbol == "epsilon") {
        return true;
    }
    return grammarSymbolMatches(current(), grammarSymbol);
}

bool TokenStream::match(const std::string& grammarSymbol) {
    if (!matches(grammarSymbol)) {
        return false;
    }
    if (grammarSymbol != "epsilon") {
        advance();
    }
    return true;
}

void TokenStream::advance() {
    if (!atEnd()) {
        ++pos_;
    }
}
