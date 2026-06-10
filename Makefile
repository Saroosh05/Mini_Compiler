CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -I src

CORE_SRCS := \
	src/lexer/buffer.cpp \
	src/lexer/token.cpp \
	src/lexer/lexer.cpp \
	src/grammar/grammar_types.cpp \
	src/grammar_loader/grammar_loader.cpp \
	src/grammar_analysis/grammar_analysis.cpp \
	src/parser/token_stream.cpp \
	src/parser/rd_parser.cpp \
	src/parser/ll1_parser.cpp \
	src/parser/lr_parser.cpp \
	src/ast/ast.cpp \
	src/types/type_system.cpp \
	src/symbol_table/symbol_table.cpp \
	src/semantic/semantic.cpp \
	src/error/error_handler.cpp \
	src/error/recovery.cpp \
	src/util/table_writer.cpp \
	src/api/compiler_session.cpp

SRCS := $(CORE_SRCS) src/main.cpp

OBJS := $(SRCS:.cpp=.o)
TARGET := mini_compiler
PAS ?= test/sample1.pas
GRAMMAR_LL1 := src/grammar/grammar_ll1.txt
BASENAME := $(notdir $(basename $(PAS)))
FULL_OUT := output/$(BASENAME)_full.txt
ifeq ($(BASENAME),sample1)
FULL_OUT := output/full_compile.txt
endif

.PHONY: all clean run test lexer first ll1-table rd ll1 lr full outputs zip

all: dirs $(TARGET)

dirs:
	@mkdir -p output 2>/dev/null || mkdir output 2>nul || exit 0

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET) 2>/dev/null || true

run: $(TARGET)
	./$(TARGET) test/sample1.pas --lexer

lexer: $(TARGET)
	./$(TARGET) $(PAS) --lexer --out output/$(BASENAME)_tokens.txt

first: $(TARGET)
	./$(TARGET) --first $(GRAMMAR_LL1) --out output/first_follow_report.txt

ll1-table: $(TARGET)
	./$(TARGET) --ll1-table $(GRAMMAR_LL1) --out output/ll1_table_report.txt

rd: $(TARGET)
	./$(TARGET) $(PAS) --rd --out output/$(BASENAME)_rd.txt

ll1: $(TARGET)
	./$(TARGET) $(PAS) --ll1 --out output/$(BASENAME)_ll1.txt

lr: $(TARGET)
	./$(TARGET) $(PAS) --lr --out output/$(BASENAME)_lr.txt

full: $(TARGET)
	./$(TARGET) $(PAS) --full --out $(FULL_OUT)

outputs: $(TARGET)
	./$(TARGET) $(PAS) --lexer --out output/$(BASENAME)_tokens.txt
	./$(TARGET) --first $(GRAMMAR_LL1) --out output/first_follow_report.txt
	./$(TARGET) --ll1-table $(GRAMMAR_LL1) --out output/ll1_table_report.txt
	./$(TARGET) $(PAS) --rd --out output/$(BASENAME)_rd.txt
	./$(TARGET) $(PAS) --ll1 --out output/$(BASENAME)_ll1.txt
	./$(TARGET) $(PAS) --lr --out output/$(BASENAME)_lr.txt
	./$(TARGET) $(PAS) --full --out $(FULL_OUT)

test: $(TARGET) outputs
	bash scripts/run_parser_tests.sh
	bash scripts/run_semantic_tests.sh