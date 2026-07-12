#include<iostream>
#include<fstream>
#include<sstream>
#include<optional>
#include<vector>
#include<cctype>
#include <stdexcept>
#include "./tokenization.h"
using namespace std;



vector<Token> tokenize(const string &str) {

}

string tokens_to_assembly(vector<Token> &tokens) {
    string buffer = "global _start\n_start:\n";
    for (int i = 0; i < tokens.size(); i++) {
        //used size because it's a vector not an array.
        const Token &token = tokens.at(i); // grabbing the token from its reference above.
        switch (token.type) {
            case TypeOfToken::exit: {
                if (i + 1 >= tokens.size()) { // to check if there was a token after return
                    throw runtime_error("Expected integer after return");
                }
                const Token &token = tokens.at(i + 1); // to get the next token.
                if (token.type != TypeOfToken::int_lit) {
                    throw runtime_error("Expected integer after return value");
                }
                buffer += "    mov rax, 60\n";
                buffer += "    mov rdi, ";
                buffer += token.value.value();
                buffer += "\n";
                buffer += "    syscall\n";

                i++;

                break;
            }
            case TypeOfToken::int_lit:
                break;
            case TypeOfToken::semi:
                break;
            case TypeOfToken::identifier:
                break;
        }
    }
    return buffer;
}

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

    vector<Token> tokens = tokenize(contents); // save returned tokens

    {
        fstream file2("out.asm", ios::out); // we want the output to become an assembly separate file
        file2 << tokens_to_assembly(tokens)<<endl; // insert the tokens (which are the assembly code we got) into the file
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
