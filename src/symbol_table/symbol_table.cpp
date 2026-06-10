#include "symbol_table.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>

SymbolTableManager::SymbolTableManager(bool trace) : trace_(trace) {}

int SymbolTableManager::hash(const std::string& name) {
    unsigned long h = 5381;
    for (char c : name) {
        h = ((h << 5) + h) + static_cast<unsigned char>(c);
    }
    return static_cast<int>(h % TABLE_SIZE);
}

void SymbolTableManager::beginScope() {
    auto scope = std::make_unique<SymbolTableScope>();
    scope->parent = current_;
    scope->scopeLevel = current_ ? current_->scopeLevel + 1 : 0;
    scope->buckets.assign(TABLE_SIZE, nullptr);
    current_ = scope.get();
    scopeStorage_.push_back(std::move(scope));
}

void SymbolTableManager::endScope() {
    if (!current_) {
        return;
    }
    current_ = current_->parent;
}

bool SymbolTableManager::insert(const std::string& name, SymbolKind kind,
                                const std::string& typeName, int line, int paramCount) {
    if (!current_) {
        beginScope();
    }
    if (findInScope(current_, name)) {
        return false;
    }

    SymbolEntry entry;
    entry.id = nextId_++;
    entry.name = name;
    entry.kind = kind;
    entry.typeName = typeName;
    entry.scopeLevel = current_->scopeLevel;
    entry.line = line;
    entry.paramCount = paramCount;

    storage_.push_back(entry);
    SymbolEntry* ptr = &storage_.back();

    const int slot = hash(name);
    ptr->next = current_->buckets[slot];
    current_->buckets[slot] = ptr;
    return true;
}

SymbolEntry* SymbolTableManager::findInScope(SymbolTableScope* scope,
                                             const std::string& name) const {
    if (!scope) {
        return nullptr;
    }
    const int slot = hash(name);
    SymbolEntry* e = scope->buckets[slot];
    while (e) {
        if (e->name == name) {
            return e;
        }
        e = e->next;
    }
    return nullptr;
}

std::optional<SymbolEntry> SymbolTableManager::lookup(const std::string& name) const {
    const SymbolEntry* best = nullptr;
    for (const SymbolEntry& entry : storage_) {
        if (entry.name != name || entry.scopeLevel > lookupDepth_) {
            continue;
        }
        if (!best || entry.scopeLevel > best->scopeLevel) {
            best = &entry;
        }
    }
    if (best) {
        return *best;
    }
    return std::nullopt;
}

void SymbolTableManager::setParamCount(const std::string& name, int count) {
    for (SymbolEntry& entry : storage_) {
        if (entry.name == name) {
            entry.paramCount = count;
            return;
        }
    }
}

void SymbolTableManager::setParamTypes(const std::string& name,
                                       const std::vector<std::string>& types) {
    for (SymbolEntry& entry : storage_) {
        if (entry.name != name) {
            continue;
        }
        if (entry.kind != SymbolKind::Function && entry.kind != SymbolKind::Procedure) {
            continue;
        }
        entry.paramTypes = types;
        entry.paramCount = static_cast<int>(types.size());
        return;
    }
}

bool SymbolTableManager::remove(const std::string& name) {
    if (!current_) {
        return false;
    }
    const int slot = hash(name);
    SymbolEntry* prev = nullptr;
    SymbolEntry* e = current_->buckets[slot];
    while (e) {
        if (e->name == name) {
            if (prev) {
                prev->next = e->next;
            } else {
                current_->buckets[slot] = e->next;
            }
            return true;
        }
        prev = e;
        e = e->next;
    }
    return false;
}

void SymbolTableManager::reportDuplicate(const std::string& name, int line) {
    errors_.push_back("duplicate declaration '" + name + "' at line " + std::to_string(line));
}

void SymbolTableManager::reportUndeclared(const std::string& name, int line) {
    errors_.push_back("undeclared identifier '" + name + "' at line " + std::to_string(line));
}

const char* SymbolTableManager::kindName(SymbolKind k) {
    switch (k) {
        case SymbolKind::Variable: return "variable";
        case SymbolKind::Constant: return "constant";
        case SymbolKind::Function: return "function";
        case SymbolKind::Procedure: return "procedure";
        case SymbolKind::Parameter: return "parameter";
        case SymbolKind::Array: return "array";
        case SymbolKind::Program: return "program";
    }
    return "?";
}

std::vector<SymbolEntry*> SymbolTableManager::collectAll() const {
    std::vector<SymbolEntry*> all;
    for (const SymbolEntry& entry : storage_) {
        all.push_back(const_cast<SymbolEntry*>(&entry));
    }
    std::sort(all.begin(), all.end(),
              [](SymbolEntry* a, SymbolEntry* b) { return a->id < b->id; });
    return all;
}

std::vector<SymbolEntry> SymbolTableManager::entries() const {
    std::vector<SymbolEntry> result;
    for (SymbolEntry* ptr : collectAll()) {
        SymbolEntry copy = *ptr;
        copy.next = nullptr;
        result.push_back(std::move(copy));
    }
    return result;
}

void SymbolTableManager::printTable(std::ostream& out) const {
    auto rows = collectAll();
    if (rows.empty()) {
        out << "(symbol table empty)\n";
        return;
    }

    out << "\n=== Symbol Table ===\n";
    out << std::setw(5) << "ID" << " | " << std::setw(12) << "Name" << " | " << std::setw(10)
        << "Kind" << " | " << std::setw(8) << "Type" << " | " << std::setw(6) << "Scope"
        << " | " << std::setw(5) << "Line" << '\n';
    out << std::string(60, '-') << '\n';
    for (const SymbolEntry* e : rows) {
        out << std::setw(5) << e->id << " | " << std::setw(12) << e->name << " | "
            << std::setw(10) << kindName(e->kind) << " | " << std::setw(8) << e->typeName
            << " | " << std::setw(6) << e->scopeLevel << " | " << std::setw(5) << e->line
            << '\n';
    }
}
