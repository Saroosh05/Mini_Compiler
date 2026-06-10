#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <vector>

enum class SymbolKind {
    Variable,
    Constant,
    Function,
    Procedure,
    Parameter,
    Array,
    Program
};

struct SymbolEntry {
    int id{0};
    std::string name;
    SymbolKind kind{SymbolKind::Variable};
    std::string typeName;
    int scopeLevel{0};
    int line{0};
    int paramCount{0};
    std::vector<std::string> paramTypes;
    SymbolEntry* next{nullptr};
};

struct SymbolTableScope {
    int scopeLevel{0};
    SymbolTableScope* parent{nullptr};
    std::vector<SymbolEntry*> buckets;
};

class SymbolTableManager {
public:
    static constexpr int TABLE_SIZE = 211;

    explicit SymbolTableManager(bool trace = false);

    void beginScope();
    void endScope();

    bool insert(const std::string& name, SymbolKind kind, const std::string& typeName, int line,
                int paramCount = 0);
    void setParamCount(const std::string& name, int count);
    void setParamTypes(const std::string& name, const std::vector<std::string>& types);
    std::optional<SymbolEntry> lookup(const std::string& name) const;
    void setLookupDepth(int depth) { lookupDepth_ = depth; }
    bool remove(const std::string& name);

    void reportDuplicate(const std::string& name, int line);
    void reportUndeclared(const std::string& name, int line);

    void printTable(std::ostream& out) const;
    std::vector<SymbolEntry> entries() const;
    const std::vector<std::string>& errors() const { return errors_; }

private:
    bool trace_{false};
    int nextId_{1};
    int lookupDepth_{255};
    SymbolTableScope* current_{nullptr};
    std::vector<std::unique_ptr<SymbolTableScope>> scopeStorage_;
    std::deque<SymbolEntry> storage_;
    std::vector<std::string> errors_;

    static int hash(const std::string& name);
    SymbolEntry* findInScope(SymbolTableScope* scope, const std::string& name) const;
    std::vector<SymbolEntry*> collectAll() const;
    static const char* kindName(SymbolKind k);
};

#endif
