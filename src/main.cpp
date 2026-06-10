#include "ast/ast.h"
#include "error/error_handler.h"
#include "grammar_analysis/grammar_analysis.h"
#include "lexer/lexer.h"
#include "parser/ll1_parser.h"
#include "parser/lr_parser.h"
#include "parser/rd_parser.h"
#include "parser/token_stream.h"
#include "semantic/semantic.h"
#include "symbol_table/symbol_table.h"
#include "util/color.h"
#include "util/table_writer.h"

#include <fstream>
#include <iostream>
#include <string>

namespace {

const char* DEFAULT_GRAMMAR_LL1 = "src/grammar/grammar_ll1.txt";
const char* DEFAULT_GRAMMAR_LR  = "src/grammar/grammar_original.txt";

enum class RunMode {
    Lexer, First, LL1Table, RD, LL1, LR, Symtab, Full
};

void printBanner() {
    std::cout
        << Color::boldCyan()
        << "Pascal Subset Mini-Compiler - CS-471L\n"
        << "Lexer | RD | LL(1) | SLR | Symbol Table | Semantics"
        << Color::reset() << "\n\n";
}

void printUsage(const char* prog) {
    printBanner();
    std::cerr
        << Color::bold() << "Usage:\n" << Color::reset()
        << "  " << Color::cyan() << prog << Color::reset()
        << " <file.pas> " << Color::yellow() << "[--lexer|--rd|--ll1|--lr|--symtab|--full]" << Color::reset()
        << " [--out file] [--no-color]\n"
        << "  " << Color::cyan() << prog << Color::reset()
        << " " << Color::yellow() << "[--first|--ll1-table]" << Color::reset()
        << " [grammar.txt] [--out file] [--no-color]\n\n"
        << Color::bold() << "Modes:\n" << Color::reset()
        << "  --lexer      Print token stream\n"
        << "  --first      Print FIRST/FOLLOW sets\n"
        << "  --ll1-table  Print LL(1) parsing table\n"
        << "  --rd         Recursive descent parse + AST + semantics\n"
        << "  --ll1        LL(1) predictive parse\n"
        << "  --lr         SLR LR parse\n"
        << "  --symtab     Parse + dump symbol table\n"
        << "  --full       Full pipeline: all parsers + traces + outputs\n"
        << "  --no-color   Disable ANSI colors (use when redirecting to file)\n";
}

void step(const std::string& label) {
    std::cout << Color::boldBlue() << "\n[>] " << Color::reset()
              << Color::bold() << label << Color::reset() << "\n";
}

void result(bool ok, const std::string& label) {
    if (ok)
        std::cout << Color::boldGreen() << "  OK  " << label << ": ACCEPT" << Color::reset() << '\n';
    else
        std::cout << Color::boldRed()   << "  NO  " << label << ": REJECT" << Color::reset() << '\n';
}

std::ostream& openOutput(const std::string& path, std::ofstream& file) {
    if (path.empty()) return std::cout;
    file.open(path, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "Cannot open output: " << path << '\n';
        std::exit(1);
    }
    return file;
}

GrammarAnalysis loadLl1Grammar(const std::string& path, ErrorHandler& errors) {
    GrammarAnalysis ga;
    if (!ga.loadGrammar(path)) {
        errors.report(ErrorKind::Syntactic, 0, 0, ga.lastError());
    } else {
        ga.compute();
    }
    return ga;
}

int runLexer(const std::string& source, const std::string& outPath) {
    printBanner();
    step("Lexical Analysis - " + source);
    ErrorHandler errors;
    std::ofstream f;
    std::ostream& out = openOutput(outPath, f);
    Lexer lexer(source);
    lexer.setErrorHandler(&errors);
    if (!lexer.init()) {
        errors.report(ErrorKind::Lexical, 0, 0, "cannot open " + source);
        errors.printSummary(out);
        return 1;
    }
    while (lexer.nextToken()) {}
    const bool plainFile = !outPath.empty();
    TableWriter w(out, !plainFile);
    w.printTitle("Lexical Analysis");
    w.printTokenTable(lexer.tokens());
    if (plainFile) {
        out << "Total tokens: " << lexer.tokens().size() << '\n';
    } else {
        out << Color::dim() << "Total tokens: " << Color::reset()
            << Color::boldWhite() << lexer.tokens().size() << Color::reset() << '\n';
    }
    if (errors.hasErrors()) {
        errors.printSummary(out);
        return 1;
    }
    std::cout << Color::boldGreen() << "  OK  " << lexer.tokens().size()
              << " tokens produced, no errors." << Color::reset() << '\n';
    return 0;
}

int runGrammarOnly(const std::string& grammarPath, const std::string& outPath, bool ll1Table) {
    printBanner();
    step(ll1Table ? "LL(1) Parsing Table" : "FIRST / FOLLOW Sets");
    ErrorHandler errors;
    GrammarAnalysis ga = loadLl1Grammar(grammarPath, errors);
    if (errors.hasErrors()) return 1;

    std::ofstream f;
    std::ostream& out = openOutput(outPath, f);
    ga.printFirstFollow(out);
    ga.exportFirstFollow("src/grammar/first_follow.txt");
    if (ll1Table) {
        ga.printLL1Table(out);
        ga.exportLL1Table("src/grammar/ll1_table.txt");
        if (!ga.conflicts().empty()) {
            std::cout << Color::boldRed() << "  NO  LL(1) conflicts: " << ga.conflicts().size()
                      << Color::reset() << '\n';
            return 1;
        }
        std::cout << Color::boldGreen() << "  OK  LL(1) table built, no conflicts."
                  << Color::reset() << '\n';
    } else {
        std::cout << Color::boldGreen() << "  OK  FIRST/FOLLOW computed."
                  << Color::reset() << '\n';
    }
    return 0;
}

int runParse(const std::string& source, const std::string& outPath, RunMode mode) {
    ErrorHandler errors;
    std::ofstream f;
    std::ostream& out = openOutput(outPath, f);

    step("Lexer - " + source);
    Lexer lexer(source);
    lexer.setErrorHandler(&errors);
    if (!lexer.init()) {
        errors.report(ErrorKind::Lexical, 0, 0, "cannot open " + source);
        errors.printSummary(out);
        return 1;
    }
    while (lexer.nextToken()) {}

    const bool plainFile = !outPath.empty();
    if (mode == RunMode::Full) {
        TableWriter w(out, !plainFile);
        w.printTitle("Lexical Analysis");
        w.printTokenTable(lexer.tokens());
        if (plainFile) {
            out << "Total tokens: " << lexer.tokens().size() << '\n';
        } else {
            out << Color::dim() << "Total tokens: " << Color::reset()
                << Color::boldWhite() << lexer.tokens().size() << Color::reset() << '\n';
        }
    }
    std::cout << Color::boldGreen() << "  OK  " << lexer.tokens().size()
              << " tokens" << Color::reset() << '\n';

    if (errors.hasErrors()) {
        std::cout << Color::boldRed() << "  NO  Lexical errors, parsing skipped." << Color::reset() << '\n';
        errors.printSummary(out);
        return 1;
    }

    step("Grammar Analysis  (FIRST / FOLLOW / LL(1) table)");
    TokenStream ts(lexer.tokens());
    GrammarAnalysis ga = loadLl1Grammar(DEFAULT_GRAMMAR_LL1, errors);
    if (errors.hasErrors()) {
        errors.printSummary(out);
        return 1;
    }

    if (mode == RunMode::Full) {
        ga.exportFirstFollow("output/first_follow.txt");
        ga.exportLL1Table("output/ll1_table.txt");
        out << Color::dim() << "FIRST/FOLLOW -> output/first_follow.txt\n"
            << "LL(1) table  -> output/ll1_table.txt" << Color::reset() << '\n';
        if (!ga.conflicts().empty()) {
            out << Color::boldRed() << "LL(1) conflicts: " << ga.conflicts().size()
                << Color::reset() << '\n';
        }
    }
    std::cout << Color::boldGreen() << "  OK  Grammar analysis complete."
              << Color::reset() << '\n';

    bool ok = false;
    std::unique_ptr<AstNode> astRoot;
    SymbolTableManager symtab(true);

    if (mode == RunMode::RD || mode == RunMode::Full || mode == RunMode::Symtab) {
        step("Recursive Descent Parser");
        RecursiveDescentParser rd(ts, errors, symtab, ga);
        ok = rd.parse();
        astRoot = rd.releaseAst();
        result(ok, "RD parse");
        out << (ok ? "RD parse: ACCEPT\n" : "RD parse: REJECT\n");
        if (astRoot) {
            AstPrinter printer;
            out << "\n" << Color::boldCyan() << "=== Abstract Syntax Tree ==="
                << Color::reset() << '\n';
            printer.print(out, astRoot.get());
        }
    }

    if (mode == RunMode::LL1 || mode == RunMode::Full) {
        step("LL(1) Predictive Parser");
        TokenStream ts2(lexer.tokens());
        LL1Parser ll1(ts2, errors, ga);
        ll1.setTrace(mode == RunMode::Full);
        const bool llOk = ll1.parse();
        result(llOk, "LL(1) parse");
        out << (llOk ? "LL(1) parse: ACCEPT\n" : "LL(1) parse: REJECT\n");
        if (mode == RunMode::Full) {
            std::ofstream traceFile("output/ll1_trace.txt");
            if (traceFile) {
                ll1.printTrace(traceFile);
                std::cout << Color::dim() << "  LL(1) trace -> output/ll1_trace.txt"
                          << Color::reset() << '\n';
            }
        } else {
            ll1.printTrace(out);
        }
        if (mode == RunMode::LL1) ok = llOk;
        else ok = ok && llOk;
    }

    if (mode == RunMode::LR || mode == RunMode::Full) {
        step("SLR LR Parser");
        TokenStream ts3(lexer.tokens());
        LrParser lr(ts3, errors);
        if (!lr.loadGrammar(DEFAULT_GRAMMAR_LR)) {
            errors.printSummary(out);
            return 1;
        }
        lr.setTrace(mode == RunMode::Full);
        lr.buildTables();
        lr.exportTables("output/lr_table.txt");
        const bool lrOk = lr.parse();
        result(lrOk, "LR parse");
        out << (lrOk ? "LR parse: ACCEPT\n" : "LR parse: REJECT\n");
        if (mode == RunMode::Full) {
            std::cout << Color::dim() << "  LR tables -> output/lr_table.txt" << Color::reset() << '\n';
            std::ofstream traceFile("output/lr_trace.txt");
            if (traceFile) {
                lr.printTrace(traceFile);
                std::cout << Color::dim() << "  LR trace  -> output/lr_trace.txt" << Color::reset() << '\n';
            }
        } else {
            lr.printTrace(out);
        }
        ok = (mode == RunMode::LR) ? lrOk : ok && lrOk;
    }

    if (mode == RunMode::Symtab || mode == RunMode::Full) {
        step("Symbol Table");
        symtab.printTable(out);
    }

    if ((mode == RunMode::Full || mode == RunMode::RD) && astRoot) {
        step("Semantic Analysis");
        SemanticAnalyzer sem(symtab, errors);
        const bool semOk = sem.analyze(astRoot.get());
        result(semOk, "Semantics");
    }

    errors.printSummary(out);
    return (ok && !errors.hasErrors()) ? 0 : 1;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    RunMode mode = RunMode::Lexer;
    std::string sourcePath;
    std::string grammarPath = DEFAULT_GRAMMAR_LL1;
    std::string outPath;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if      (arg == "--lexer")    mode = RunMode::Lexer;
        else if (arg == "--first")    mode = RunMode::First;
        else if (arg == "--ll1-table") mode = RunMode::LL1Table;
        else if (arg == "--rd")       mode = RunMode::RD;
        else if (arg == "--ll1")      mode = RunMode::LL1;
        else if (arg == "--lr")       mode = RunMode::LR;
        else if (arg == "--symtab")   mode = RunMode::Symtab;
        else if (arg == "--full")     mode = RunMode::Full;
        else if (arg == "--no-color") Color::disable();
        else if (arg == "--out" && i + 1 < argc) outPath = argv[++i];
        else if (arg.size() >= 4 && arg.compare(arg.size() - 4, 4, ".pas") == 0) sourcePath = arg;
        else if (arg[0] != '-') grammarPath = arg;
        else { printUsage(argv[0]); return 1; }
    }

    if (mode == RunMode::First)    return runGrammarOnly(grammarPath, outPath, false);
    if (mode == RunMode::LL1Table) return runGrammarOnly(grammarPath, outPath, true);

    if (sourcePath.empty()) {
        std::cerr << Color::boldRed() << "Error: " << Color::reset()
                  << "A .pas source file is required for this mode.\n";
        return 1;
    }

    if (mode == RunMode::Lexer) return runLexer(sourcePath, outPath);

    printBanner();
    return runParse(sourcePath, outPath, mode);
}
