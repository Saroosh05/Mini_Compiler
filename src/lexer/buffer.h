#ifndef BUFFER_H
#define BUFFER_H

#include "../config.h"

#include <fstream>
#include <string>

// Synchronous double buffer (Compiler Construction Lab 2).
// Each half holds BUFFER_SIZE chars plus a sentinel '\0' marking end-of-chunk.
class Buffer {
public:
    explicit Buffer(const std::string& path);

    bool open();
    char peek();
    char peek2();
    char advance();
    bool eof() const;
    int line() const;
    int column() const;

private:
    static constexpr char SENTINEL = '\0';

    std::string path_;
    std::ifstream file_;
    char buf_[2][BUFFER_SIZE + 1];
    int active_{0};
    int pos_{0};
    int line_{1};
    int column_{1};
    bool file_eof_{false};
    bool stream_eof_{false};
    bool second_filled_{false};

    void fillBuffer(int which);
    char currentChar() const;
    void switchBuffer();
};

#endif
