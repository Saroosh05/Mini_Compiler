#ifndef GRAMMAR_TYPES_H
#define GRAMMAR_TYPES_H

#include <set>
#include <string>
#include <vector>

inline const std::string GRAMMAR_EPSILON = "epsilon";
inline const std::string GRAMMAR_END = "$";

struct Production {
    int index{0};
    std::string lhs;
    std::vector<std::string> rhs;
};

struct Grammar {
    std::vector<Production> productions;
    std::string startSymbol;
    std::set<std::string> nonTerminals;
    std::set<std::string> terminals;

    bool isNonTerminal(const std::string& symbol) const;
    bool isEpsilon(const std::string& symbol) const;
};

#endif
