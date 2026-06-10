#include "error_handler.h"

#include "../util/color.h"
#include <iostream>

static const char* kindLabel(ErrorKind k) {
    switch (k) {
        case ErrorKind::Lexical:   return "Lexical";
        case ErrorKind::Syntactic: return "Syntactic";
        case ErrorKind::Semantic:  return "Semantic";
        case ErrorKind::Warning:   return "Warning";
    }
    return "?";
}

static const char* kindColor(ErrorKind k) {
    switch (k) {
        case ErrorKind::Lexical:   return Color::boldRed();
        case ErrorKind::Syntactic: return Color::boldRed();
        case ErrorKind::Semantic:  return Color::boldYellow();
        case ErrorKind::Warning:   return Color::yellow();
    }
    return Color::reset();
}

void ErrorHandler::report(ErrorKind kind, int line, int column, const std::string& message,
                          const std::string& expected, const std::string& found) {
    errors_.push_back({kind, line, column, message, expected, found});
    if (silent_) {
        return;
    }
    std::cerr << kindColor(kind) << '[' << kindLabel(kind) << ']' << Color::reset()
              << Color::dim() << " at " << Color::reset()
              << Color::boldWhite() << line << ":" << column << Color::reset()
              << " - " << message;
    if (!expected.empty()) {
        std::cerr << Color::dim() << " (expected: " << Color::reset()
                  << Color::cyan() << expected << Color::reset()
                  << Color::dim() << ", got: " << Color::reset()
                  << Color::yellow() << found << Color::reset() << ")";
    }
    std::cerr << '\n';
}

int ErrorHandler::count(ErrorKind kind) const {
    int n = 0;
    for (const CompilerError& e : errors_) {
        if (e.kind == kind) {
            ++n;
        }
    }
    return n;
}

bool ErrorHandler::hasWarnings() const {
    return count(ErrorKind::Warning) > 0;
}

void ErrorHandler::printSummary(std::ostream& out) const {
    const int lex  = count(ErrorKind::Lexical);
    const int syn  = count(ErrorKind::Syntactic);
    const int sem  = count(ErrorKind::Semantic);
    const int warn = count(ErrorKind::Warning);
    const int total = static_cast<int>(errors_.size());

    out << '\n'
        << Color::boldCyan() << "=== Compilation Summary ==="
        << Color::reset() << '\n';

    auto countLine = [&](const char* label, int n, const char* col) {
        out << "  " << label << "  ";
        if (n > 0)
            out << col << n << Color::reset();
        else
            out << Color::dim() << "0" << Color::reset();
        out << '\n';
    };

    countLine("Lexical  errors  :", lex,  Color::boldRed());
    countLine("Syntax   errors  :", syn,  Color::boldRed());
    countLine("Semantic errors  :", sem,  Color::boldYellow());
    countLine("Warnings         :", warn, Color::yellow());

    out << Color::dim() << "  " << std::string(30, '-') << Color::reset() << '\n';
    out << "  Total            :  ";
    if (total == 0)
        out << Color::boldGreen() << "0 - clean" << Color::reset() << '\n';
    else
        out << Color::boldRed() << total << Color::reset() << '\n';

    if (!errors_.empty()) {
        out << '\n' << Color::bold() << "  Details:" << Color::reset() << '\n';
        for (const CompilerError& e : errors_) {
            out << "  " << kindColor(e.kind) << '[' << kindLabel(e.kind) << ']' << Color::reset()
                << ' ' << Color::boldWhite() << e.line << ':' << e.column << Color::reset()
                << " - " << e.message;
            if (!e.expected.empty()) {
                out << Color::dim() << " (expected: " << Color::reset()
                    << Color::cyan() << e.expected << Color::reset()
                    << Color::dim() << ", got: " << Color::reset()
                    << Color::yellow() << e.found << Color::reset() << ')';
            }
            out << '\n';
        }
    }
    out << '\n';
}

bool ErrorHandler::hasErrors() const {
    for (const CompilerError& e : errors_) {
        if (e.kind != ErrorKind::Warning) {
            return true;
        }
    }
    return false;
}
