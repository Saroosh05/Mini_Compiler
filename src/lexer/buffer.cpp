#include "buffer.h"

#include "../config.h"

#include <iostream>

Buffer::Buffer(const std::string& path) : path_(path) {}

bool Buffer::open() {
    file_.open(path_, std::ios::binary);
    if (!file_.is_open()) {
        return false;
    }
    fillBuffer(0);
    buf_[1][0] = SENTINEL;
    active_ = 0;
    pos_ = 0;
    line_ = 1;
    column_ = 1;
    file_eof_ = false;
    stream_eof_ = false;
    second_filled_ = false;
    return true;
}

void Buffer::fillBuffer(int which) {
    file_.read(buf_[which], BUFFER_SIZE);
    const std::streamsize n = file_.gcount();
    if (n < BUFFER_SIZE) {
        file_eof_ = true;
    }
    buf_[which][static_cast<size_t>(n)] = SENTINEL;
}

char Buffer::currentChar() const {
    return buf_[active_][pos_];
}

void Buffer::switchBuffer() {
    const int next = 1 - active_;
    if (!second_filled_ && next == 1) {
        fillBuffer(1);
        second_filled_ = true;
    }
    active_ = next;
    pos_ = 0;
}

char Buffer::peek() {
    if (stream_eof_) {
        return SENTINEL;
    }

    char c = currentChar();
    if (c != SENTINEL) {
        return c;
    }

    // End of current chunk.
    if (file_eof_ && (active_ == 1 || second_filled_)) {
        stream_eof_ = true;
        return SENTINEL;
    }

    switchBuffer();
    c = currentChar();
    if (c == SENTINEL) {
        stream_eof_ = true;
    }
    return c;
}

char Buffer::peek2() {
    if (stream_eof_) {
        return SENTINEL;
    }
    const int savePos = pos_;
    const int saveActive = active_;
    const int saveLine = line_;
    const int saveCol = column_;

    char c1 = peek();
    if (c1 == SENTINEL) {
        return SENTINEL;
    }
    advance();
    char c2 = peek();

    pos_ = savePos;
    active_ = saveActive;
    line_ = saveLine;
    column_ = saveCol;
    return c2;
}

char Buffer::advance() {
    char c = peek();
    if (stream_eof_) {
        return SENTINEL;
    }

    if (c == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }

    ++pos_;
    return c;
}

bool Buffer::eof() const {
    return stream_eof_;
}

int Buffer::line() const { return line_; }
int Buffer::column() const { return column_; }
