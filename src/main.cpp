#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include "arena.h"
#include "ast.h"
#include "parser.h"
#include "generation.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Incorrect usage.\n";
        std::cerr << "Correct usage: nexcore <input.nex>\n";
        return EXIT_FAILURE;
    }

    ArenaAllocator arena(1024 * 1024);

    std::string filename = argv[1];

    std::ifstream input(filename);

    if (!input) {
        std::cerr << "Failed to open " << filename << "\n";
        return EXIT_FAILURE;
    }

    std::string source(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>()
    );

    Parser parser(source, arena);
    NodeProgram program = parser.parse();

    Generator generator(std::move(program));

    std::ofstream output("out.asm");

    if (!output) {
        std::cerr << "Failed to create out.asm\n";
        return EXIT_FAILURE;
    }

    output << generator.generate();

    output.close();

    if (system("nasm -f elf64 out.asm") != 0) {
        std::cerr << "NASM failed.\n";
        return EXIT_FAILURE;
    }

    if (system("ld -o out out.o") != 0) {
        std::cerr << "Linker failed.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}