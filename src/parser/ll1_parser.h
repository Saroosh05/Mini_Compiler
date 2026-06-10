#ifndef LL1_PARSER_H
#define LL1_PARSER_H

#include "../error/error_handler.h"
#include "../grammar_analysis/grammar_analysis.h"
#include "token_stream.h"

#include <ostream>
#include <string>

class LL1Parser {
public:
    LL1Parser(TokenStream& ts, ErrorHandler& errors, const GrammarAnalysis& grammar);

    bool parse();
    void setTrace(bool on) { trace_ = on; }
    void printTrace(std::ostream& out) const;
    const std::vector<std::string>& traceLines() const { return traceLog_; }

private:
    TokenStream& ts_;
    ErrorHandler& errors_;
    const GrammarAnalysis& grammar_;
    bool trace_{false};
    std::vector<std::string> traceLog_;

    bool parseStep();
};

#endif
