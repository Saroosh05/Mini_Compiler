#ifndef COMPILER_SESSION_H
#define COMPILER_SESSION_H

#include "../ast/ast.h"
#include "../error/error_handler.h"
#include "../grammar_analysis/grammar_analysis.h"
#include "../lexer/token.h"
#include "../symbol_table/symbol_table.h"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

enum class PipelineStageStatus {
    Pending,
    Running,
    Passed,
    Failed,
    Skipped,
    Warning
};

struct PipelineStageInfo {
    std::string name;
    PipelineStageStatus status{PipelineStageStatus::Pending};
    int errorCount{0};
    std::string detail;
};

struct CompilationResult {
    std::string sourcePath;
    std::string sourceText;

    std::vector<Token> tokens;
    bool lexerOk{false};

    GrammarAnalysis grammar;
    bool grammarOk{false};

    bool rdOk{false};
    bool ll1Ok{false};
    bool lrOk{false};
    bool semanticsOk{false};

    std::unique_ptr<AstNode> ast;
    std::vector<SymbolEntry> symbols;
    std::vector<CompilerError> errors;

    std::vector<std::string> rdSummary;
    std::vector<std::string> ll1Trace;
    std::vector<std::string> lrTrace;

    std::string astText;
    int astNodeCount{0};
    int productionCount{0};
    int ll1TableEntryCount{0};
    int ll1ConflictCount{0};

    std::chrono::milliseconds compileTime{0};
    std::vector<PipelineStageInfo> stages;
    bool success{false};
};

struct CompileOptions {
    std::string grammarLl1Path{"src/grammar/grammar_ll1.txt"};
    std::string grammarLrPath{"src/grammar/grammar_original.txt"};
    std::string outputDir;  // writes tokens/first_follow/ll1_table/traces to this folder
    bool runRd{true};
    bool runLl1{true};
    bool runLr{true};
    bool runSemantics{true};
};

// Facade over the existing compiler pipeline. No parsing logic here.
class CompilerSession {
public:
    CompilationResult compile(const std::string& sourcePath,
                              const CompileOptions& opts = {}) const;
    bool saveReport(const CompilationResult& result, const std::string& outPath) const;

    static int countAstNodes(const AstNode* root);
    static std::string astToText(const AstNode* root);
};

#endif
