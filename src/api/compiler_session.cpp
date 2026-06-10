#include "compiler_session.h"

#include "../ast/ast.h"
#include "../lexer/lexer.h"
#include "../parser/ll1_parser.h"
#include "../parser/lr_parser.h"
#include "../parser/rd_parser.h"
#include "../parser/token_stream.h"
#include "../semantic/semantic.h"
#include "../util/table_writer.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace {

const char* stageStatusLabel(PipelineStageStatus s) {
    switch (s) {
        case PipelineStageStatus::Pending: return "Pending";
        case PipelineStageStatus::Running: return "Running";
        case PipelineStageStatus::Passed: return "Passed";
        case PipelineStageStatus::Failed: return "Failed";
        case PipelineStageStatus::Skipped: return "Skipped";
        case PipelineStageStatus::Warning: return "Warning";
    }
    return "?";
}

GrammarAnalysis loadGrammar(const std::string& path, ErrorHandler& errors) {
    GrammarAnalysis ga;
    if (!ga.loadGrammar(path)) {
        errors.report(ErrorKind::Syntactic, 0, 0, ga.lastError());
    } else {
        ga.compute();
    }
    return ga;
}

std::string readFileText(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        return {};
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void setStage(std::vector<PipelineStageInfo>& stages, const std::string& name,
              PipelineStageStatus status, int errorCount, const std::string& detail) {
    for (PipelineStageInfo& s : stages) {
        if (s.name == name) {
            s.status = status;
            s.errorCount = errorCount;
            s.detail = detail;
            return;
        }
    }
    stages.push_back({name, status, errorCount, detail});
}

int countErrors(const ErrorHandler& errors, ErrorKind kind) {
    return errors.count(kind);
}

bool writeLineFile(const std::string& path, const std::vector<std::string>& lines) {
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out) {
        return false;
    }
    for (const std::string& line : lines) {
        out << line << '\n';
    }
    return true;
}

void exportArtifacts(const CompilationResult& result, const CompileOptions& opts) {
    if (opts.outputDir.empty()) {
        return;
    }

    std::error_code ec;
    std::filesystem::create_directories(opts.outputDir, ec);
    const std::string dir = opts.outputDir;

    if (result.grammarOk) {
        result.grammar.exportFirstFollow(dir + "/first_follow.txt");
        result.grammar.exportLL1Table(dir + "/ll1_table.txt");
    }

    if (!result.tokens.empty()) {
        std::ofstream tokOut(dir + "/tokens.txt", std::ios::out | std::ios::trunc);
        if (tokOut) {
            TableWriter w(tokOut, false);
            w.printTitle("Lexical Analysis");
            w.printTokenTable(result.tokens);
            tokOut << "Total tokens: " << result.tokens.size() << '\n';
        }
    }

    if (!result.ll1Trace.empty()) {
        writeLineFile(dir + "/ll1_trace.txt", result.ll1Trace);
    }
    if (!result.lrTrace.empty()) {
        writeLineFile(dir + "/lr_trace.txt", result.lrTrace);
    }
}

}  // namespace

int CompilerSession::countAstNodes(const AstNode* root) {
    if (!root) {
        return 0;
    }
    int n = 1;
    for (const auto& child : root->children) {
        n += countAstNodes(child.get());
    }
    return n;
}

std::string CompilerSession::astToText(const AstNode* root) {
    if (!root) {
        return "(no AST)\n";
    }
    std::ostringstream out;
    AstPrinter printer;
    printer.print(out, root);
    return out.str();
}

CompilationResult CompilerSession::compile(const std::string& sourcePath,
                                           const CompileOptions& opts) const {
    const auto start = std::chrono::steady_clock::now();

    CompilationResult result;
    result.sourcePath = sourcePath;
    result.sourceText = readFileText(sourcePath);

    result.stages = {
        {"Lexical Analysis", PipelineStageStatus::Pending, 0, ""},
        {"FIRST / FOLLOW", PipelineStageStatus::Pending, 0, ""},
        {"LL(1) Table", PipelineStageStatus::Pending, 0, ""},
        {"Recursive Descent", PipelineStageStatus::Pending, 0, ""},
        {"LL(1) Parser", PipelineStageStatus::Pending, 0, ""},
        {"LR Parser", PipelineStageStatus::Pending, 0, ""},
        {"AST", PipelineStageStatus::Pending, 0, ""},
        {"Symbol Table", PipelineStageStatus::Pending, 0, ""},
        {"Semantic Analysis", PipelineStageStatus::Pending, 0, ""},
    };

    ErrorHandler errors;
    errors.setSilent(true);

    setStage(result.stages, "Lexical Analysis", PipelineStageStatus::Running, 0, "");
    Lexer lexer(sourcePath);
    lexer.setErrorHandler(&errors);
    if (!lexer.init()) {
        errors.report(ErrorKind::Lexical, 0, 0, "cannot open " + sourcePath);
        result.lexerOk = false;
        setStage(result.stages, "Lexical Analysis", PipelineStageStatus::Failed,
                 countErrors(errors, ErrorKind::Lexical), "Cannot open source file");
        result.errors = errors.all();
        result.compileTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);
        return result;
    }
    while (lexer.nextToken()) {}
    result.tokens = lexer.tokens();
    result.lexerOk = !errors.hasErrors();
    setStage(result.stages, "Lexical Analysis", result.lexerOk ? PipelineStageStatus::Passed
                                                                 : PipelineStageStatus::Failed,
             countErrors(errors, ErrorKind::Lexical),
             std::to_string(result.tokens.size()) + " tokens");

    if (!result.lexerOk) {
        for (PipelineStageInfo& s : result.stages) {
            if (s.name != "Lexical Analysis" && s.status == PipelineStageStatus::Pending) {
                s.status = PipelineStageStatus::Skipped;
            }
        }
        result.errors = errors.all();
        result.compileTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);
        return result;
    }

    setStage(result.stages, "FIRST / FOLLOW", PipelineStageStatus::Running, 0, "");
    result.grammar = loadGrammar(opts.grammarLl1Path, errors);
    result.grammarOk = !errors.hasErrors();
    setStage(result.stages, "FIRST / FOLLOW",
             result.grammarOk ? PipelineStageStatus::Passed : PipelineStageStatus::Failed, 0,
             std::to_string(result.grammar.first().size()) + " non-terminals");

    setStage(result.stages, "LL(1) Table", PipelineStageStatus::Running, 0, "");
    result.productionCount = static_cast<int>(result.grammar.grammar().productions.size());
    result.ll1TableEntryCount = static_cast<int>(result.grammar.ll1Table().size());
    result.ll1ConflictCount = static_cast<int>(result.grammar.conflicts().size());
    const auto ll1Status = result.ll1ConflictCount > 0 ? PipelineStageStatus::Warning
                                                       : PipelineStageStatus::Passed;
    setStage(result.stages, "LL(1) Table", ll1Status, result.ll1ConflictCount,
             std::to_string(result.ll1TableEntryCount) + " entries");

    if (!result.grammarOk) {
        result.errors = errors.all();
        result.compileTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);
        return result;
    }

    TokenStream ts(result.tokens);
    SymbolTableManager symtab(false);
    std::unique_ptr<AstNode> astRoot;

    if (opts.runRd) {
        setStage(result.stages, "Recursive Descent", PipelineStageStatus::Running, 0, "");
        RecursiveDescentParser rd(ts, errors, symtab, result.grammar);
        result.rdOk = rd.parse();
        astRoot = rd.releaseAst();
        result.rdSummary = {
            result.rdOk ? "Recursive descent parse: ACCEPT" : "Recursive descent parse: REJECT",
            "Grammar productions available: " + std::to_string(result.productionCount),
        };
        setStage(result.stages, "Recursive Descent",
                 result.rdOk ? PipelineStageStatus::Passed : PipelineStageStatus::Failed,
                 countErrors(errors, ErrorKind::Syntactic), result.rdOk ? "ACCEPT" : "REJECT");
    } else {
        setStage(result.stages, "Recursive Descent", PipelineStageStatus::Skipped, 0, "");
    }

    if (opts.runLl1) {
        setStage(result.stages, "LL(1) Parser", PipelineStageStatus::Running, 0, "");
        TokenStream ts2(result.tokens);
        LL1Parser ll1(ts2, errors, result.grammar);
        ll1.setTrace(true);
        result.ll1Ok = ll1.parse();
        result.ll1Trace = ll1.traceLines();
        setStage(result.stages, "LL(1) Parser",
                 result.ll1Ok ? PipelineStageStatus::Passed : PipelineStageStatus::Failed,
                 countErrors(errors, ErrorKind::Syntactic), result.ll1Ok ? "ACCEPT" : "REJECT");
    } else {
        setStage(result.stages, "LL(1) Parser", PipelineStageStatus::Skipped, 0, "");
    }

    if (opts.runLr) {
        setStage(result.stages, "LR Parser", PipelineStageStatus::Running, 0, "");
        TokenStream ts3(result.tokens);
        LrParser lr(ts3, errors);
        if (lr.loadGrammar(opts.grammarLrPath) && lr.buildTables()) {
            if (!opts.outputDir.empty()) {
                lr.exportTables(opts.outputDir + "/lr_table.txt");
            }
            lr.setTrace(true);
            result.lrOk = lr.parse();
            result.lrTrace = lr.traceLines();
        }
        setStage(result.stages, "LR Parser",
                 result.lrOk ? PipelineStageStatus::Passed : PipelineStageStatus::Failed,
                 countErrors(errors, ErrorKind::Syntactic), result.lrOk ? "ACCEPT" : "REJECT");
    } else {
        setStage(result.stages, "LR Parser", PipelineStageStatus::Skipped, 0, "");
    }

    result.ast = std::move(astRoot);
    result.symbols = symtab.entries();
    if (result.ast) {
        result.astNodeCount = countAstNodes(result.ast.get());
        result.astText = astToText(result.ast.get());
        setStage(result.stages, "AST", PipelineStageStatus::Passed, 0,
                 std::to_string(result.astNodeCount) + " nodes");
    } else {
        setStage(result.stages, "AST", PipelineStageStatus::Failed, 0, "No AST produced");
    }
    setStage(result.stages, "Symbol Table", PipelineStageStatus::Passed, 0,
             std::to_string(result.symbols.size()) + " symbols");

    if (opts.runSemantics && result.ast) {
        setStage(result.stages, "Semantic Analysis", PipelineStageStatus::Running, 0, "");
        SemanticAnalyzer sem(symtab, errors);
        result.semanticsOk = sem.analyze(result.ast.get());
        result.astText = astToText(result.ast.get());
        const int semErr = countErrors(errors, ErrorKind::Semantic);
        setStage(result.stages, "Semantic Analysis",
                 result.semanticsOk ? PipelineStageStatus::Passed : PipelineStageStatus::Failed,
                 semErr, result.semanticsOk ? "No semantic errors" : "Semantic errors found");
    } else {
        setStage(result.stages, "Semantic Analysis", PipelineStageStatus::Skipped, 0, "");
        result.semanticsOk = true;
    }

    result.errors = errors.all();
    result.success = result.lexerOk && result.rdOk && result.ll1Ok && result.lrOk
                     && result.semanticsOk && !errors.hasErrors();
    result.compileTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    if (result.lexerOk) {
        exportArtifacts(result, opts);
    }

    return result;
}

bool CompilerSession::saveReport(const CompilationResult& result,
                                 const std::string& outPath) const {
    std::ofstream out(outPath, std::ios::out | std::ios::trunc);
    if (!out) {
        return false;
    }

    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    out << "Pascal Subset Mini-Compiler - Compilation Report\n";
    out << "Generated: " << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S") << "\n";
    out << "Source: " << result.sourcePath << "\n";
    out << "Verdict: " << (result.success ? "SUCCESS" : "FAILED") << "\n";
    out << "Compile time: " << result.compileTime.count() << " ms\n\n";

    out << "=== Pipeline Summary ===\n";
    for (const PipelineStageInfo& s : result.stages) {
        out << s.name << " | " << stageStatusLabel(s.status) << " | errors=" << s.errorCount
            << " | " << s.detail << "\n";
    }

    out << "\n=== Statistics ===\n";
    out << "Tokens: " << result.tokens.size() << "\n";
    out << "Productions: " << result.productionCount << "\n";
    out << "LL(1) entries: " << result.ll1TableEntryCount << "\n";
    out << "LL(1) conflicts: " << result.ll1ConflictCount << "\n";
    out << "AST nodes: " << result.astNodeCount << "\n";
    out << "Symbols: " << result.symbols.size() << "\n";
    out << "Errors: " << result.errors.size() << "\n\n";

    TableWriter w(out, false);
    w.printTitle("Lexical Analysis");
    w.printTokenTable(result.tokens);

    out << "\n=== FIRST / FOLLOW ===\n";
    result.grammar.printFirstFollow(out);

    out << "\n=== LL(1) Table ===\n";
    result.grammar.printLL1Table(out);

    out << "\n=== Recursive Descent ===\n";
    for (const std::string& line : result.rdSummary) {
        out << line << '\n';
    }

    out << "\n=== LL(1) Trace ===\n";
    for (const std::string& line : result.ll1Trace) {
        out << line << '\n';
    }

    out << "\n=== LR Trace ===\n";
    for (const std::string& line : result.lrTrace) {
        out << line << '\n';
    }

    out << "\n=== Abstract Syntax Tree ===\n" << result.astText;

    out << "\n=== Symbol Table ===\n";
    for (const SymbolEntry& e : result.symbols) {
        out << e.id << " | " << e.name << " | scope=" << e.scopeLevel << " | line=" << e.line
            << " | type=" << e.typeName << '\n';
    }

    auto errorKindName = [](ErrorKind k) {
        switch (k) {
            case ErrorKind::Lexical: return "Lexical";
            case ErrorKind::Syntactic: return "Syntactic";
            case ErrorKind::Semantic: return "Semantic";
            case ErrorKind::Warning: return "Warning";
        }
        return "?";
    };

    out << "\n=== Errors and Warnings ===\n";
    for (const CompilerError& e : result.errors) {
        out << '[' << errorKindName(e.kind) << "] " << e.line << ':' << e.column << " - "
            << e.message;
        if (!e.expected.empty()) {
            out << " (expected: " << e.expected << ", got: " << e.found << ')';
        }
        out << '\n';
    }

    return true;
}
