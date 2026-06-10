#ifndef GRAMMAR_LOADER_H
#define GRAMMAR_LOADER_H

#include "../grammar/grammar_types.h"

#include <string>

class GrammarLoader {
public:
    // Load BNF from file. Lines: "A -> X Y" or "A -> epsilon", # comments.
    static bool loadFromFile(const std::string& path, Grammar& out, std::string& error);
};

#endif
