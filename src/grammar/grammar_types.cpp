#include "grammar_types.h"

bool Grammar::isNonTerminal(const std::string& symbol) const {
    return nonTerminals.count(symbol) > 0;
}

bool Grammar::isEpsilon(const std::string& symbol) const {
    return symbol == GRAMMAR_EPSILON;
}
