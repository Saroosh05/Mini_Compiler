#include "table_writer.h"
#include "color.h"

#include <algorithm>
#include <iomanip>

TableWriter::TableWriter(std::ostream& out, bool useColor)
    : out_(out), useColor_(useColor && Color::isEnabled()) {}

void TableWriter::printTitle(const std::string& title) {
    // Compute box width: title + 4 padding chars
    const size_t w = title.size() + 4;
    const std::string bar(w, '=');
    out_ << '\n';
    if (useColor_) {
        out_ << Color::boldCyan();
    }
    out_ << bar << '\n'
         << "  " << title << "  " << '\n'
         << bar;
    if (useColor_) {
        out_ << Color::reset();
    }
    out_ << '\n';
}

void TableWriter::resetTable() {
    rows_.clear();
    widths_.clear();
}

size_t TableWriter::displayWidth(const std::string& s) {
    return s.size();
}

void TableWriter::addRow(const std::vector<std::string>& cells) {
    if (widths_.empty()) {
        widths_.resize(cells.size(), 0);
    }
    for (size_t i = 0; i < cells.size(); ++i) {
        if (i >= widths_.size()) {
            widths_.push_back(0);
        }
        widths_[i] = std::max(widths_[i], displayWidth(cells[i]));
    }
    rows_.push_back(cells);
}

void TableWriter::renderTable() {
    if (rows_.empty()) {
        return;
    }

    const bool hasColor = useColor_;

    auto printSep = [&]() {
        if (hasColor) out_ << Color::dim();
        out_ << '+';
        for (size_t cw : widths_) {
            out_ << std::string(cw + 2, '-') << '+';
        }
        if (hasColor) out_ << Color::reset();
        out_ << '\n';
    };

    printSep();
    bool isHeader = true;
    for (const auto& row : rows_) {
        if (hasColor) out_ << Color::dim();
        out_ << '|';
        if (hasColor) out_ << Color::reset();
        for (size_t i = 0; i < widths_.size(); ++i) {
            const std::string cell = (i < row.size()) ? row[i] : "";
            out_ << ' ';
            if (isHeader) {
                if (hasColor) out_ << Color::boldCyan();
                out_ << std::setw(static_cast<int>(widths_[i])) << std::left << cell;
                if (hasColor) out_ << Color::reset();
            } else {
                out_ << std::setw(static_cast<int>(widths_[i])) << std::left << cell;
            }
            out_ << ' ';
            if (hasColor) out_ << Color::dim();
            out_ << '|';
            if (hasColor) out_ << Color::reset();
        }
        out_ << '\n';
        if (isHeader) {
            printSep();
            isHeader = false;
        }
    }
    printSep();
    out_ << '\n';
}

void TableWriter::printMatrix(const std::string& title,
                              const std::vector<std::string>& columnHeaders,
                              const std::vector<std::string>& rowLabels,
                              const std::vector<std::vector<std::string>>& cells) {
    printTitle(title);
    resetTable();

    std::vector<std::string> headerRow;
    headerRow.push_back("");
    for (const std::string& h : columnHeaders) {
        headerRow.push_back(h);
    }
    addRow(headerRow);

    for (size_t r = 0; r < rowLabels.size(); ++r) {
        std::vector<std::string> row;
        row.push_back(rowLabels[r]);
        if (r < cells.size()) {
            for (const std::string& c : cells[r]) {
                row.push_back(c);
            }
        }
        addRow(row);
    }
    renderTable();
}

void TableWriter::printTokenTable(const std::vector<Token>& tokens) {
    resetTable();
    addRow({"#", "Line", "Col", "Kind", "Lexeme", "Attribute", "Grammar"});
    int index = 1;
    for (const Token& t : tokens) {
        addRow({std::to_string(index++),
                std::to_string(t.line),
                std::to_string(t.column),
                tokenKindName(t.kind),
                t.lexeme,
                t.attribute,
                tokenGrammarSymbol(t.kind)});
    }
    renderTable();
}

void TableWriter::flush() {
    out_.flush();
}
