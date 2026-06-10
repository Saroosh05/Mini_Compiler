#include "recovery.h"

void ParseRecovery::synchronize(TokenStream& ts, const std::set<std::string>& syncSet) {
    syncSet.count("$");
    while (!ts.atEnd()) {
        if (syncSet.count(ts.currentSymbol())) {
            return;
        }
        ts.advance();
    }
}

std::set<std::string> ParseRecovery::followOf(const GrammarAnalysis& ga,
                                              const std::string& nt) {
    const auto& follow = ga.follow();
    const auto it = follow.find(nt);
    if (it != follow.end()) {
        return it->second;
    }
    return {};
}
