#ifndef LR_PARSER_H
#define LR_PARSER_H

#include "../error/error_handler.h"
#include "../grammar/grammar_types.h"
#include "token_stream.h"

#include <map>
#include <ostream>
#include <set>
#include <string>
#include <vector>

class LrParser {
public:
    explicit LrParser(TokenStream& ts, ErrorHandler& errors);

    bool loadGrammar(const std::string& path);
    bool buildTables();
    bool parse();
    void setTrace(bool on) { trace_ = on; }
    void printTrace(std::ostream& out) const;
    bool exportTables(const std::string& path) const;
    const std::vector<std::string>& traceLines() const { return traceLog_; }

private:
    struct Item {
        int prod{0};
        int dot{0};
        bool operator<(const Item& o) const;
        bool operator==(const Item& o) const { return prod == o.prod && dot == o.dot; }
    };

    enum class ActionKind { Shift, Reduce, Accept, Error };
    struct Action {
        ActionKind kind{ActionKind::Error};
        int value{0};
    };

    TokenStream& ts_;
    ErrorHandler& errors_;
    Grammar grammar_;
    bool trace_{false};

    std::vector<std::set<Item>> states_;
    std::map<std::pair<int, std::string>, int> gotoTable_;
    std::map<std::pair<int, std::string>, Action> actionTable_;
    std::map<std::string, std::set<std::string>> first_;
    std::map<std::string, std::set<std::string>> follow_;
    std::vector<std::string> traceLog_;

    std::set<Item> closure(const std::set<Item>& items) const;
    std::set<Item> goTo(const std::set<Item>& items, const std::string& sym) const;
    void computeFirst();
    void computeFollow();
    void buildAutomaton();
    void buildSLRTable();
    std::set<std::string> firstOfRhs(const std::vector<std::string>& rhs) const;
    std::string mapLookahead(const std::string& a, int state) const;
    bool itemComplete(const Item& it) const;
};

#endif
