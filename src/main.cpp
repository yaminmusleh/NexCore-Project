#include<iostream>
#include<fstream>
#include<sstream>
#include<optional>
#include<vector>
#include<cctype>
#include <stdexcept>
#include "./tokenization.h"
#include "./parser.h"
#include "./generation.h"
using namespace std;


int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Incorrect usage. Correct usage is..." << endl;
        cerr << "nexcore <input.nex>" << endl;
        return EXIT_FAILURE;
    }

    fstream file(argv[1], ios::in);

    if (!file) {
        cerr << "Failed to open file." << endl;
        return EXIT_FAILURE;
    }

    stringstream buffer;
    buffer << file.rdbuf();

    string contents = buffer.str();

    Tokenization tokenizer(std::move(contents));
    vector<Token> tokens = tokenizer.tokenize(); // save returned tokens
    cout << "Token count: " << tokens.size() << '\n';

    for (const auto &t: tokens) {
        cout << static_cast<int>(t.type);

        if (t.value)
            cout << " -> " << *t.value;

        cout << '\n';
    }

    ArenaAllocator arena(1024 * 1024);
    Parser parser(std::move(tokens),arena);

    NodeProgram root = parser.parse();


    Generator generator(std::move(root)); // passing the root value for the generator

    {
        fstream file2("out.asm", ios::out); // we want the output to become an assembly separate file
        file2 << generator.generate();
        // insert the tokens (which are the assembly code we got) into the file
    }

    if (system("nasm -f elf64 out.asm") != 0) {
        cerr << "NASM failed.\n";
        return EXIT_FAILURE;
    }

    if (system("ld -o out out.o") != 0) {
        cerr << "Linker failed.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
