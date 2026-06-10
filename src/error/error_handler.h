#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <iostream>
#include <ostream>
#include <string>
#include <vector>

enum class ErrorKind { Lexical, Syntactic, Semantic, Warning };

struct CompilerError {
    ErrorKind kind;
    int line;
    int column;
    std::string message;
    std::string expected;
    std::string found;
};

class ErrorHandler {
public:
    void setSilent(bool silent) { silent_ = silent; }
    bool isSilent() const { return silent_; }
    void clear() { errors_.clear(); }

    void report(ErrorKind kind, int line, int column, const std::string& message,
                const std::string& expected = "", const std::string& found = "");
    void printSummary(std::ostream& out = std::cout) const;
    bool hasErrors() const;
    bool hasWarnings() const;
    int count(ErrorKind kind) const;
    const std::vector<CompilerError>& all() const { return errors_; }

private:
    bool silent_{false};
    std::vector<CompilerError> errors_;
};

#endif
