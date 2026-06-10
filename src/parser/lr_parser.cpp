#include "lr_parser.h"

#include "../grammar_loader/grammar_loader.h"
#include "../lexer/token.h"

#include <algorithm>
#include <fstream>
#include <stack>

bool LrParser::Item::operator<(const Item& o) const {
    return prod < o.prod || (prod == o.prod && dot < o.dot);
}

LrParser::LrParser(TokenStream& ts, ErrorHandler& errors) : ts_(ts), errors_(errors) {}

bool LrParser::itemComplete(const Item& it) const {
    const Production& p = grammar_.productions[it.prod];
    for (int i = it.dot; i < static_cast<int>(p.rhs.size()); ++i) {
        if (!grammar_.isEpsilon(p.rhs[i])) {
            return false;
        }
    }
    return true;
}

bool LrParser::loadGrammar(const std::string& path) {
    std::string err;
    Grammar g;
    if (!GrammarLoader::loadFromFile(path, g, err)) {
        errors_.report(ErrorKind::Syntactic, 0, 0, err);
        return false;
    }
    Production aug;
    aug.lhs = "S'";
    aug.rhs = {g.startSymbol};
    g.productions.insert(g.productions.begin(), aug);
    for (size_t i = 0; i < g.productions.size(); ++i) {
        g.productions[i].index = static_cast<int>(i);
    }
    g.startSymbol = "S'";
    g.nonTerminals.insert("S'");
    grammar_ = std::move(g);
    grammar_.terminals.erase(GRAMMAR_EPSILON);
    return true;
}

void LrParser::computeFirst() {
    first_.clear();
    for (const std::string& nt : grammar_.nonTerminals) {
        first_[nt];
    }
    for (const std::string& t : grammar_.terminals) {
        if (t != GRAMMAR_EPSILON) {
            first_[t].insert(t);
        }
    }
    first_[GRAMMAR_END].insert(GRAMMAR_END);

    bool changed = true;
    while (changed) {
        changed = false;
        for (const Production& prod : grammar_.productions) {
            const std::string& A = prod.lhs;
            bool allEpsilon = true;
            for (const std::string& sym : prod.rhs) {
                if (grammar_.isEpsilon(sym)) {
                    continue;
                }
                std::set<std::string> symFirst;
                if (grammar_.isNonTerminal(sym)) {
                    symFirst = first_[sym];
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

std::set<std::string> LrParser::firstOfRhs(const std::vector<std::string>& rhs) const {
    std::set<std::string> result;
    bool allEpsilon = true;
    for (const std::string& sym : rhs) {
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

void LrParser::computeFollow() {
    follow_.clear();
    for (const std::string& nt : grammar_.nonTerminals) {
        follow_[nt];
    }
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

std::set<LrParser::Item> LrParser::closure(const std::set<Item>& items) const {
    std::set<Item> result = items;
    bool changed = true;
    while (changed) {
        changed = false;
        std::set<Item> add;
        for (const Item& it : result) {
            const Production& p = grammar_.productions[it.prod];
            if (it.dot >= static_cast<int>(p.rhs.size())) {
                continue;
            }
            const std::string& sym = p.rhs[it.dot];
            if (!grammar_.isNonTerminal(sym)) {
                continue;
            }
            for (const Production& prod : grammar_.productions) {
                if (prod.lhs != sym) {
                    continue;
                }
                Item ni{prod.index, 0};
                if (!result.count(ni)) {
                    add.insert(ni);
                    changed = true;
                }
            }
        }
        result.insert(add.begin(), add.end());
    }
    return result;
}

std::set<LrParser::Item> LrParser::goTo(const std::set<Item>& items,
                                          const std::string& sym) const {
    std::set<Item> moved;
    for (const Item& it : items) {
        const Production& p = grammar_.productions[it.prod];
        if (it.dot < static_cast<int>(p.rhs.size()) && p.rhs[it.dot] == sym) {
            moved.insert(Item{it.prod, it.dot + 1});
        }
    }
    return closure(moved);
}

void LrParser::buildAutomaton() {
    states_.clear();
    gotoTable_.clear();

    std::set<Item> start;
    start.insert(Item{0, 0});
    start = closure(start);
    states_.push_back(start);

    std::set<std::string> symbols;
    for (const std::string& t : grammar_.terminals) {
        symbols.insert(t);
    }
    for (const std::string& nt : grammar_.nonTerminals) {
        symbols.insert(nt);
    }

    for (size_t i = 0; i < states_.size(); ++i) {
        for (const std::string& sym : symbols) {
            if (sym == GRAMMAR_EPSILON) {
                continue;
            }
            std::set<Item> next = goTo(states_[i], sym);
            if (next.empty()) {
                continue;
            }

            int idx = -1;
            for (size_t j = 0; j < states_.size(); ++j) {
                if (states_[j] == next) {
                    idx = static_cast<int>(j);
                    break;
                }
            }
            if (idx < 0) {
                idx = static_cast<int>(states_.size());
                states_.push_back(next);
            }
            gotoTable_[{static_cast<int>(i), sym}] = idx;
        }
    }
}

void LrParser::buildSLRTable() {
    actionTable_.clear();

    // 1) Shift actions (take precedence over reduce on conflicts)
    for (size_t i = 0; i < states_.size(); ++i) {
        for (const std::string& t : grammar_.terminals) {
            if (t == GRAMMAR_EPSILON) {
                continue;
            }
            const auto gt = gotoTable_.find({static_cast<int>(i), t});
            if (gt != gotoTable_.end()) {
                actionTable_[{static_cast<int>(i), t}] = {ActionKind::Shift, gt->second};
            }
        }
    }

    // 2) Reduce actions only where no shift exists
    for (size_t i = 0; i < states_.size(); ++i) {
        for (const Item& it : states_[i]) {
            const Production& p = grammar_.productions[it.prod];
            if (!itemComplete(it)) {
                continue;
            }

            if (p.lhs == "S'") {
                actionTable_[{static_cast<int>(i), GRAMMAR_END}] = {ActionKind::Accept, 0};
                continue;
            }

            for (const std::string& a : follow_[p.lhs]) {
                const auto key = std::make_pair(static_cast<int>(i), a);
                if (actionTable_.count(key)) {
                    continue;
                }
                actionTable_[key] = {ActionKind::Reduce, p.index};
            }
        }
    }
}

bool LrParser::buildTables() {
    computeFirst();
    computeFollow();
    buildAutomaton();
    buildSLRTable();
    return !states_.empty();
}

std::string LrParser::mapLookahead(const std::string& a, int state) const {
    if (actionTable_.count({state, a})) {
        return a;
    }
    if ((a == "+" || a == "-") && actionTable_.count({state, "addop"})) {
        return "addop";
    }
    if ((a == "*" || a == "/") && actionTable_.count({state, "mulop"})) {
        return "mulop";
    }
    if (actionTable_.count({state, "relop"})) {
        if (a == "=" || a == "<" || a == ">" || a == "<=" || a == ">=" || a == "<>") {
            return "relop";
        }
    }
    return a;
}

bool LrParser::parse() {
    if (states_.empty() && !buildTables()) {
        return false;
    }

    std::stack<int> stateSt;
    std::stack<std::string> symSt;
    stateSt.push(0);

    size_t ip = 0;
    const auto& tokens = ts_.all();

    auto la = [&]() -> std::string {
        if (ip >= tokens.size()) {
            return GRAMMAR_END;
        }
        return terminalGrammarSymbol(tokens[ip]);
    };

    int step = 0;
    while (true) {
        const int s = stateSt.top();
        const std::string raw = la();
        const std::string a = mapLookahead(raw, s);

        const auto it = actionTable_.find({s, a});
        if (it == actionTable_.end()) {
            int line = 0, col = 0;
            if (!tokens.empty()) {
                const size_t idx = std::min(ip, tokens.size() - 1);
                line = tokens[idx].line;
                col = tokens[idx].column;
            }
            errors_.report(ErrorKind::Syntactic, line, col,
                           "LR: no action at state " + std::to_string(s) + " on " + raw);
            return false;
        }

        const Action& act = it->second;
        if (trace_) {
            traceLog_.push_back("Step " + std::to_string(++step) + " state=" +
                                std::to_string(s) + " sym=" + raw + " action=" +
                                (act.kind == ActionKind::Shift   ? "shift"
                                 : act.kind == ActionKind::Reduce ? "reduce"
                                 : act.kind == ActionKind::Accept ? "accept"
                                                                  : "error"));
        }

        if (act.kind == ActionKind::Shift) {
            stateSt.push(act.value);
            symSt.push(a);
            ++ip;
        } else if (act.kind == ActionKind::Reduce) {
            const Production& p = grammar_.productions[act.value];
            int popCount = 0;
            for (const std::string& sym : p.rhs) {
                if (!grammar_.isEpsilon(sym)) {
                    ++popCount;
                }
            }
            for (int k = 0; k < popCount; ++k) {
                if (!stateSt.empty()) {
                    stateSt.pop();
                }
                if (!symSt.empty()) {
                    symSt.pop();
                }
            }
            if (stateSt.empty()) {
                errors_.report(ErrorKind::Syntactic, 0, 0, "LR: empty state stack on reduce");
                return false;
            }
            const int t = stateSt.top();
            const auto gt = gotoTable_.find({t, p.lhs});
            if (gt == gotoTable_.end()) {
                errors_.report(ErrorKind::Syntactic, 0, 0, "LR: missing GOTO for " + p.lhs);
                return false;
            }
            stateSt.push(gt->second);
            symSt.push(p.lhs);
        } else if (act.kind == ActionKind::Accept) {
            return true;
        } else {
            return false;
        }
    }
}

void LrParser::printTrace(std::ostream& out) const {
    for (const std::string& line : traceLog_) {
        out << line << '\n';
    }
}

bool LrParser::exportTables(const std::string& path) const {
    std::ofstream out(path);
    if (!out) {
        return false;
    }
    out << "# SLR ACTION/GOTO\n";
    for (const auto& [key, act] : actionTable_) {
        out << "ACTION " << key.first << " " << key.second << " ";
        if (act.kind == ActionKind::Shift) {
            out << "s" << act.value;
        } else if (act.kind == ActionKind::Reduce) {
            out << "r" << act.value;
        } else {
            out << "acc";
        }
        out << "\n";
    }
    for (const auto& [key, st] : gotoTable_) {
        if (!grammar_.isNonTerminal(key.second)) {
            continue;
        }
        out << "GOTO " << key.first << " " << key.second << " " << st << "\n";
    }
    return true;
}
