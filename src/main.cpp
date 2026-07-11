#include<iostream>
#include<fstream>
#include<sstream>
#include<optional>
#include<vector>
#include<cctype>
#include <stdexcept>
using namespace std;

enum class TypeOfToken {
    _return,
    int_lit,
    semi,
    identifier
};

struct Token {
    TypeOfToken type;
    optional<string> value{};
};

vector<Token> tokenize(const string &str) {
    //scan characters, build a buffer,  create tokens
    vector<Token> tokens{}; // storing tokens in the string
    string buffer;

    for (int i = 0; i < str.length(); i++) {
        char c = str.at(i); // get current character

        if (isalpha(c)) {
            //checks if a single character is alphabetic or not

            buffer.clear(); //clearing buffer before doing a new keyword/identifier

            //it will continue reading as long as it's an alphabetic character
            //like these identifiers values: "value1", "count2", etc..

            while (i < str.length() && isalnum(str.at(i))) {
                buffer.push_back(str.at(i)); //pushing the character into the buffer
                i++;
            }
            i--; //steps back because for loop increments another i

            if (buffer == "return") {
                tokens.push_back(Token{TypeOfToken::_return, buffer});
            } else {
                tokens.push_back(Token{TypeOfToken::identifier, buffer});
            }
        } else if (isdigit(c)) {
            buffer.clear();

            while (i < str.length() && isdigit(str.at(i))) {
                buffer.push_back(str.at(i));
                i++;
            }
            i--;
            tokens.push_back(Token{TypeOfToken::int_lit, buffer});
        } else if (c == ';') {
            tokens.push_back(Token{TypeOfToken::semi, ";"});
        }
    }

    return tokens;
}

string tokens_to_assembly(vector<Token> &tokens) {
    string buffer = "global _start\nstart:\n";
    for (int i = 0; i < tokens.size(); i++) {
        //used size because its a vector not an array.
        const Token &token = tokens.at(i); // grabbing the token from its reference above.
        switch (token.type) {
            case TypeOfToken::_return: {
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
        cerr << "hyrdo <input.hy>" << endl;
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

    cout<<tokens_to_assembly(tokens)<<endl;

    return EXIT_SUCCESS;
}
