# Pascal Subset Mini Compiler

This project contains the lexer, recursive descent parser, LL(1) parser, SLR parser, symbol table, semantic checks, error handling, and optional Qt GUI.

## Requirements

- C++17 compiler: `g++` or `clang++`
- Linux or WSL is recommended
- Qt 6 and CMake are only needed for the GUI

## Build CLI

On Linux or WSL:

```bash
make clean
make
```

On Windows PowerShell, if using WSL:

```powershell
wsl -d Ubuntu -e bash -c "cd /mnt/<drive>/path/to/project && make clean && make"
```

On Windows with a new enough `g++` installed:

```powershell
build.bat
```

## Run CLI

Use these commands after building:

```bash
./mini_compiler test/sample1.pas --lexer
./mini_compiler --first
./mini_compiler --ll1-table
./mini_compiler test/sample1.pas --rd
./mini_compiler test/sample1.pas --ll1
./mini_compiler test/sample1.pas --lr
./mini_compiler test/sample1.pas --symtab
./mini_compiler test/sample1.pas --full
```

To save output in a file:

```bash
./mini_compiler test/sample1.pas --full --no-color --out output/full_compile.txt
```

## Make Output Files

This command rebuilds the main sample outputs:

```bash
make outputs
```

You can use another test file like this:

```bash
make outputs PAS=test/sample2.pas
```

## Build GUI

Install Qt first on Ubuntu or WSL:

```bash
sudo apt install qt6-base-dev cmake
```

Build and run:

```bash
cd gui
mkdir -p build
cd build
cmake ..
make
./compiler_gui
```

## Important Files

- `src/grammar/grammar_ll1.txt`: grammar used by recursive descent and LL(1)
- `src/grammar/grammar_original.txt`: grammar used by SLR parser
- `test/`: sample valid and invalid programs
- `output/`: sample compiler outputs
- `docs/`: project report