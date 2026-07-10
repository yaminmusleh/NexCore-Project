#include<iostream>
#include<fstream>
#include<sstream>
#include<optional>
#include<vector>
#include<cctype>
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
    //scan characters, build a buffer, create tokens
    vector<Token> tokens{};
    string buffer;

    for (int i = 0; i < str.length(); i++) {
        char c = str.at(i);

        if (isalpha(c)) {
            buffer.clear();

            while (i < str.length() && isalnum(str.at(i))) {
                buffer.push_back(str.at(i));
                i++;
            }
            i--;

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

    // print tokens
    for (Token token: tokens) {
        cout << token.value.value() << endl;
    }


    return EXIT_SUCCESS;
}
