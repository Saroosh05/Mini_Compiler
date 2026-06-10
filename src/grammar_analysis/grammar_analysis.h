#ifndef GRAMMAR_ANALYSIS_H
#define GRAMMAR_ANALYSIS_H

#include "../grammar/grammar_types.h"

#include <map>
#include <set>
#include <string>
#include <vector>

// FIRST/FOLLOW and LL(1) parsing table construction.
class GrammarAnalysis {
public:
    bool loadGrammar(const std::string& path);
    void compute();

    const Grammar& grammar() const { return grammar_; }
    const std::map<std::string, std::set<std::string>>& first() const { return first_; }
    const std::map<std::string, std::set<std::string>>& follow() const { return follow_; }

    // (non-terminal, terminal) -> production index; absent if empty cell
    const std::map<std::pair<std::string, std::string>, int>& ll1Table() const {
        return ll1Table_;
    }
    const std::vector<std::string>& conflicts() const { return conflicts_; }

    bool exportFirstFollow(const std::string& path) const;
    bool exportLL1Table(const std::string& path) const;

    void printFirstFollow(std::ostream& out) const;
    void printLL1Table(std::ostream& out) const;

    std::string lastError() const { return lastError_; }

private:
    Grammar grammar_;
    std::map<std::string, std::set<std::string>> first_;
    std::map<std::string, std::set<std::string>> follow_;
    std::map<std::pair<std::string, std::string>, int> ll1Table_;
    std::vector<std::string> conflicts_;
    std::string lastError_;

    void computeFirst();
    void computeFollow();
    std::set<std::string> firstOfSequence(const std::vector<std::string>& seq) const;
    void buildLL1Table();
    static std::string setToString(const std::set<std::string>& s);
};

#endif
