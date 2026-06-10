#include "grammar_analysis.h"

#include "../grammar_loader/grammar_loader.h"

#include <fstream>
#include <iomanip>
#include <sstream>

bool GrammarAnalysis::loadGrammar(const std::string& path) {
    return GrammarLoader::loadFromFile(path, grammar_, lastError_);
}

void GrammarAnalysis::compute() {
    first_.clear();
    follow_.clear();
    ll1Table_.clear();
    conflicts_.clear();

    for (const std::string& nt : grammar_.nonTerminals) {
        first_[nt];
        follow_[nt];
    }

    for (const std::string& t : grammar_.terminals) {
        if (t != GRAMMAR_EPSILON) {
            first_[t].insert(t);
        }
    }
    first_[GRAMMAR_END].insert(GRAMMAR_END);

    computeFirst();
    computeFollow();
    buildLL1Table();
}

void GrammarAnalysis::computeFirst() {
    bool changed = true;
    while (changed) {
        changed = false;
        for (const Production& prod : grammar_.productions) {
            const std::string& A = prod.lhs;
            bool allEpsilon = true;

            for (const std::string& sym : prod.rhs) {
                std::set<std::string> symFirst;
                if (grammar_.isEpsilon(sym)) {
                    symFirst.insert(GRAMMAR_EPSILON);
                } else if (grammar_.isNonTerminal(sym)) {
                    symFirst = first_.at(sym);
                } else {
                    symFirst.insert(sym);
                }

                for (const std::string& x : symFirst) {
                    if (x != GRAMMAR_EPSILON && first_[A].insert(x).second) {
                        changed = true;
                    }
                }

                if (!symFirst.count(GRAMMAR_EPSILON)) {
                    allEpsilon = false;
                    break;
                }
            }

            if (allEpsilon && first_[A].insert(GRAMMAR_EPSILON).second) {
                changed = true;
            }
        }
    }
}

void GrammarAnalysis::computeFollow() {
    follow_[grammar_.startSymbol].insert(GRAMMAR_END);

    bool changed = true;
    while (changed) {
        changed = false;
        for (const Production& prod : grammar_.productions) {
            const std::string& A = prod.lhs;
            std::set<std::string> trailer = follow_[A];

            for (int i = static_cast<int>(prod.rhs.size()) - 1; i >= 0; --i) {
                const std::string& B = prod.rhs[i];
                if (grammar_.isEpsilon(B)) {
                    continue;
                }

                if (grammar_.isNonTerminal(B)) {
                    for (const std::string& x : trailer) {
                        if (follow_[B].insert(x).second) {
                            changed = true;
                        }
                    }

                    if (first_.at(B).count(GRAMMAR_EPSILON)) {
                        std::set<std::string> beta = first_.at(B);
                        beta.erase(GRAMMAR_EPSILON);
                        trailer.insert(beta.begin(), beta.end());
                    } else {
                        trailer = first_.at(B);
                    }
                } else {
                    trailer = {B};
                }
            }
        }
    }
}

std::set<std::string> GrammarAnalysis::firstOfSequence(
    const std::vector<std::string>& seq) const {
    std::set<std::string> result;
    bool allEpsilon = true;

    for (const std::string& sym : seq) {
        if (grammar_.isEpsilon(sym)) {
            continue;
        }

        std::set<std::string> symFirst;
        if (grammar_.isNonTerminal(sym)) {
            symFirst = first_.at(sym);
        } else {
            symFirst.insert(sym);
        }

        for (const std::string& x : symFirst) {
            if (x != GRAMMAR_EPSILON) {
                result.insert(x);
            }
        }

        if (!symFirst.count(GRAMMAR_EPSILON)) {
            allEpsilon = false;
            break;
        }
    }

    if (allEpsilon) {
        result.insert(GRAMMAR_EPSILON);
    }
    return result;
}

void GrammarAnalysis::buildLL1Table() {
    for (const Production& prod : grammar_.productions) {
        const std::string& A = prod.lhs;
        const auto firstRhs = firstOfSequence(prod.rhs);

        auto addEntry = [&](const std::string& terminal) {
            const auto key = std::make_pair(A, terminal);
            const auto it = ll1Table_.find(key);
            if (it != ll1Table_.end() && it->second != prod.index) {
                std::ostringstream oss;
                oss << "LL(1) conflict at M[" << A << ", " << terminal << "]: productions "
                    << it->second << " and " << prod.index;
                conflicts_.push_back(oss.str());
            }
            ll1Table_[key] = prod.index;
        };

        for (const std::string& t : firstRhs) {
            if (t != GRAMMAR_EPSILON) {
                addEntry(t);
            }
        }

        if (firstRhs.count(GRAMMAR_EPSILON)) {
            for (const std::string& b : follow_.at(A)) {
                addEntry(b);
            }
        }
    }
}

std::string GrammarAnalysis::setToString(const std::set<std::string>& s) {
    std::string out = "{ ";
    for (const std::string& x : s) {
        out += x + " ";
    }
    out += "}";
    return out;
}

void GrammarAnalysis::printFirstFollow(std::ostream& out) const {
    out << "\n=== FIRST and FOLLOW Sets ===\n\n";
    for (const std::string& nt : grammar_.nonTerminals) {
        out << nt << "\n";
        out << "  FIRST  = " << setToString(first_.at(nt)) << "\n";
        out << "  FOLLOW = " << setToString(follow_.at(nt)) << "\n\n";
    }
}

void GrammarAnalysis::printLL1Table(std::ostream& out) const {
    out << "\n=== LL(1) Parsing Table ===\n\n";

    std::vector<std::string> terms(grammar_.terminals.begin(), grammar_.terminals.end());
    std::vector<std::string> nts(grammar_.nonTerminals.begin(), grammar_.nonTerminals.end());

    // Header row
    out << std::setw(28) << std::left << "Non-terminal";
    for (const std::string& t : terms) {
        out << std::setw(14) << std::left << t;
    }
    out << "\n";

    for (const std::string& nt : nts) {
        out << std::setw(28) << std::left << nt;
        for (const std::string& t : terms) {
            const auto it = ll1Table_.find({nt, t});
            if (it != ll1Table_.end()) {
                out << std::setw(14) << std::left << ("P" + std::to_string(it->second));
            } else {
                out << std::setw(14) << std::left << "";
            }
        }
        out << "\n";
    }

    if (!conflicts_.empty()) {
        out << "\nConflicts detected:\n";
        for (const std::string& c : conflicts_) {
            out << "  - " << c << "\n";
        }
    } else {
        out << "\nNo LL(1) conflicts detected.\n";
    }

    out << "\nProductions:\n";
    for (const Production& p : grammar_.productions) {
        out << "  P" << p.index << ": " << p.lhs << " ->";
        for (const std::string& sym : p.rhs) {
            out << " " << sym;
        }
        out << "\n";
    }
}

bool GrammarAnalysis::exportFirstFollow(const std::string& path) const {
    std::ofstream out(path);
    if (!out.is_open()) {
        return false;
    }

    out << "# FIRST and FOLLOW sets\n";
    for (const std::string& nt : grammar_.nonTerminals) {
        out << "FIRST " << nt << " = " << setToString(first_.at(nt)) << "\n";
        out << "FOLLOW " << nt << " = " << setToString(follow_.at(nt)) << "\n";
    }
    return true;
}

bool GrammarAnalysis::exportLL1Table(const std::string& path) const {
    std::ofstream out(path);
    if (!out.is_open()) {
        return false;
    }

    out << "# LL(1) parsing table: nonterminal, terminal, production_index\n";
    for (const auto& entry : ll1Table_) {
        out << entry.first.first << ", " << entry.first.second << ", " << entry.second << "\n";
    }

    if (!conflicts_.empty()) {
        out << "# CONFLICTS\n";
        for (const std::string& c : conflicts_) {
            out << "# " << c << "\n";
        }
    }
    return true;
}
