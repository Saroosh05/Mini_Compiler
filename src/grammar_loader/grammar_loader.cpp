#include "grammar_loader.h"

#include <cctype>
#include <fstream>
#include <sstream>

namespace {

std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(start, end - start);
}

std::vector<std::string> splitRhs(const std::string& rhs) {
    std::vector<std::string> symbols;
    std::istringstream iss(rhs);
    std::string sym;
    while (iss >> sym) {
        symbols.push_back(sym);
    }
    return symbols;
}

void classifySymbols(Grammar& grammar) {
    grammar.nonTerminals.clear();
    grammar.terminals.clear();

    for (const Production& p : grammar.productions) {
        grammar.nonTerminals.insert(p.lhs);
    }

    for (const Production& p : grammar.productions) {
        for (const std::string& sym : p.rhs) {
            if (grammar.isEpsilon(sym)) {
                continue;
            }
            if (!grammar.isNonTerminal(sym)) {
                grammar.terminals.insert(sym);
            }
        }
    }

    grammar.terminals.insert(GRAMMAR_END);
}

}  // namespace

bool GrammarLoader::loadFromFile(const std::string& path, Grammar& out, std::string& error) {
    std::ifstream in(path);
    if (!in.is_open()) {
        error = "cannot open grammar file: " + path;
        return false;
    }

    Grammar grammar;
    std::string line;
    int index = 0;

    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        const size_t arrow = line.find("->");
        if (arrow == std::string::npos) {
            error = "invalid production (missing ->): " + line;
            return false;
        }

        Production prod;
        prod.lhs = trim(line.substr(0, arrow));
        prod.rhs = splitRhs(trim(line.substr(arrow + 2)));
        prod.index = index++;

        if (prod.lhs.empty()) {
            error = "empty left-hand side: " + line;
            return false;
        }
        if (prod.rhs.empty()) {
            error = "empty right-hand side: " + line;
            return false;
        }

        grammar.productions.push_back(prod);
    }

    if (grammar.productions.empty()) {
        error = "grammar file has no productions: " + path;
        return false;
    }

    grammar.startSymbol = grammar.productions.front().lhs;
    classifySymbols(grammar);
    out = std::move(grammar);
    return true;
}
