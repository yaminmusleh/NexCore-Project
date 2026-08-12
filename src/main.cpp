#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include "arena.h"
#include "ast.h"
#include "generation.h"
#include "Parser.h"
#include "Scanner.h"

extern ArenaAllocator *arena;
extern NodeProgram program;

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cerr << "Incorrect usage.\n";
        std::cerr << "Correct usage: nexcore <input.nex>\n";
        return EXIT_FAILURE;
    }

    ArenaAllocator myArena(1024 * 1024);
    arena = &myArena;

    FILE *input = fopen(argv[1], "rb");

    if (!input) {
        std::cerr << "Failed to open " << argv[1] << "\n";
        return EXIT_FAILURE;
    }

    Scanner scanner(input);

    Parser parser(&scanner);



    parser.Parse();



    fclose(input);


    Generator generator(std::move(program));


    std::string assembly = generator.generate();

    std::ofstream output("out.asm");

    if (!output) {
        std::cerr << "Failed to create out.asm\n";
        return EXIT_FAILURE;
    }

    output << assembly;
    output.close();

    if (system("nasm -f elf64 out.asm -o out.o") != 0) {
        std::cerr << "NASM failed.\n";
        return EXIT_FAILURE;
    }

    if (system("ld -o out out.o") != 0) {
        std::cerr << "Linker failed.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}