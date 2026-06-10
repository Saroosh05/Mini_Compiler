#include "ll1_parser.h"

#include <stack>

LL1Parser::LL1Parser(TokenStream& ts, ErrorHandler& errors, const GrammarAnalysis& grammar)
    : ts_(ts), errors_(errors), grammar_(grammar) {}

bool LL1Parser::parse() {
    if (!grammar_.conflicts().empty()) {
        errors_.report(ErrorKind::Syntactic, 0, 0, "LL(1) grammar has conflicts");
        return false;
    }

    std::stack<std::string> st;
    st.push("$");
    st.push(grammar_.grammar().startSymbol);

    size_t ip = 0;
    const auto& tokens = ts_.all();

    auto currentTerminal = [&]() -> std::string {
        if (ip >= tokens.size()) return "$";
        return terminalGrammarSymbol(tokens[ip]);
    };

    while (!st.empty()) {
        const std::string X = st.top();
        st.pop();
        const std::string a = currentTerminal();

        if (trace_) {
            traceLog_.push_back("Stack top: " + X + ", input: " + a);
        }

        if (!grammar_.grammar().isNonTerminal(X)) {
            const bool match =
                (X == a) || (X == "addop" && (a == "+" || a == "-")) ||
                (X == "mulop" && (a == "*" || a == "/")) ||
                (X == "relop" && tokens[ip].kind == TokenKind::RELOP);
            if (match) {
                if (X != "$") ++ip;
                continue;
            }
            const Token& t = tokens[std::min(ip, tokens.size() - 1)];
            errors_.report(ErrorKind::Syntactic, t.line, t.column, "LL(1) mismatch at '" + X + "'",
                           X, a);
            return false;
        }

        const auto& table = grammar_.ll1Table();
        auto lookup = [&](const std::string& nt, std::string term) {
            auto it = table.find({nt, term});
            if (it != table.end()) return it;
            if ((term == "+" || term == "-") && table.count({nt, "addop"})) {
                return table.find({nt, "addop"});
            }
            if ((term == "*" || term == "/") && table.count({nt, "mulop"})) {
                return table.find({nt, "mulop"});
            }
            return table.end();
        };
        const auto it = lookup(X, a);
        if (it == table.end()) {
            const Token& t = tokens[std::min(ip, tokens.size() - 1)];
            errors_.report(ErrorKind::Syntactic, t.line, t.column,
                           "no LL(1) entry for M[" + X + ", " + a + "]");
            return false;
        }

        const Production& prod = grammar_.grammar().productions.at(it->second);
        if (trace_) {
            traceLog_.push_back("Expand P" + std::to_string(prod.index) + ": " + prod.lhs + " -> ...");
        }

        for (int i = static_cast<int>(prod.rhs.size()) - 1; i >= 0; --i) {
            if (prod.rhs[i] == GRAMMAR_EPSILON) continue;
            st.push(prod.rhs[i]);
        }
    }

    return true;
}

void LL1Parser::printTrace(std::ostream& out) const {
    for (const std::string& line : traceLog_) {
        out << line << '\n';
    }
}
