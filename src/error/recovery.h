#ifndef RECOVERY_H
#define RECOVERY_H

#include "../grammar_analysis/grammar_analysis.h"
#include "../parser/token_stream.h"

#include <set>
#include <string>

// Panic-mode recovery: skip tokens until one in syncSet (FOLLOW-based).
class ParseRecovery {
public:
    static void synchronize(TokenStream& ts, const std::set<std::string>& syncSet);
    static std::set<std::string> followOf(const GrammarAnalysis& ga, const std::string& nt);
};

#endif
