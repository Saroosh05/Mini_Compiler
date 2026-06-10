#ifndef TABLE_WRITER_H
#define TABLE_WRITER_H

#include "../lexer/token.h"

#include <iostream>
#include <string>
#include <vector>

class TableWriter {
public:
    explicit TableWriter(std::ostream& out = std::cout, bool useColor = true);

    void printTitle(const std::string& title);
    void printTokenTable(const std::vector<Token>& tokens);
    void printMatrix(const std::string& title, const std::vector<std::string>& columnHeaders,
                     const std::vector<std::string>& rowLabels,
                     const std::vector<std::vector<std::string>>& cells);
    void flush();

private:
    std::ostream& out_;
    bool useColor_{true};
    std::vector<std::vector<std::string>> rows_;
    std::vector<size_t> widths_;

    void resetTable();
    void addRow(const std::vector<std::string>& cells);
    void renderTable();
    static size_t displayWidth(const std::string& s);
};

#endif
